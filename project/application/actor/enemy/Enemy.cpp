#define NOMINMAX
#include "Enemy.h"
#include "ModelManager.h"
#include <algorithm>
#include <cstdlib> 
#include <AABB.h>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void Enemy::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// 敵の読み込み
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);

	// 触手の読み込み
	tentacle_ = std::make_unique<TKM::Object3d>();
	tentacle_->Initialize(common, dxCommon);

	// カメラ設定（既存に合わせる）
	if (camera_) {
		object_->SetCamera(camera_);
		tentacle_->SetCamera(camera_);
	}

	// 親子付け：触手を傘の子にする
	tentacle_->SetParent(object_.get());

	// 触手のローカル（傘からの相対）初期値
	tentacle_->SetTranslate(tentacleLocalPos_);
	tentacle_->SetRotate(tentacleLocalRot_);
	tentacle_->SetScale(tentacleLocalScale_);

	// 当たり判定用スケールの初期値
	baseScale_ = object_->GetScale();
	startX_ = object_->GetTranslate().x;
}

void Enemy::Update(float dt) {

	// 60fps基準の値をそのまま使えるようにする係数
	// dt=1/60 のとき factor=1.0 になる
	const float factor_ = dt * 60.0f;

	// =========================================================
	// Data-driven：死亡リアクション / 行動 の関数テーブル
	// =========================================================

	// 死亡リアクション用のコンテキスト構造体
	struct DeathCtx {
		float dt_; // 前フレームからの経過時間（秒）
		float t_; // 死亡リアクション開始からの経過時間
		Vector3 pos_; // 現在の位置
		Vector3 rot_; // 現在の回転
		Vector3 scale_; // 現在のスケール
	};
	// 行動用のコンテキスト構造体
	struct MoveCtx {
		float dt_; // 前フレームからの経過時間（秒）
		float factor_; // 60fps基準の値をそのまま使えるようにする係数
		Vector3 pos_; // 現在の位置
	};
	// ローカル関数をまとめる構造体
	struct Local {

		// 死亡リアクション関数の例：被弾方向に吹き飛ぶ + 回転 + 縮む
		static void Death_BlowAway(Enemy* self, DeathCtx& c) {
			float speed_ = 1.0f - c.t_; // 時間経過で減速
			float s_ = 1.0f - c.t_; // 時間経過で縮む
			// 被弾方向に吹き飛ぶ
			c.pos_ += self->deathVelocity_ * speed_ * c.dt_;
			// 回転も加速していく
			c.rot_.x += self->deathRotateSpeed_.x * c.dt_;
			c.rot_.y += self->deathRotateSpeed_.y * c.dt_;
			c.rot_.z += self->deathRotateSpeed_.z * c.dt_;
			// スケールは均等に縮む
			c.scale_ = { self->baseScale_.x * s_, self->baseScale_.y * s_, self->baseScale_.z * s_ };
		}
		// 死亡リアクション関数の例：空中でふわっと浮かび上がる + 回転 + 縮む
		static void Death_RiseAbsorb(Enemy* self, DeathCtx& c) {
			float s_ = 1.0f - c.t_; // 時間経過で縮む
			// 被弾方向に吹き飛ぶ（Y軸は上向きに固定）
			c.pos_ += self->deathVelocity_ * c.dt_;
			// Y軸は上向きに固定してふわっと浮かび上がる
			c.rot_.y += self->deathRotateSpeed_.y * c.dt_;
			// スケールはX,Zは縮むがYはあまり縮まない
			c.scale_ = {
				self->baseScale_.x* s_ * 0.5f, // X軸は早めに縮む
				self->baseScale_.y* (1.0f - c.t_ * 0.2f), // Y軸はあまり縮まない
				self->baseScale_.z* s_ * 0.5f // Z軸は早めに縮む
			};
		}
		// 死亡リアクション関数の例：地面に倒れ込むように崩れる
		static void Death_Collapse(Enemy* self, DeathCtx& c) {
			float s_ = 1.0f - c.t_; // 時間経過で縮む
			// 被弾方向に吹き飛ぶ（Y軸は地面に向かって固定）
			c.pos_ += self->deathVelocity_ * c.dt_;
			// Y軸は地面に向かって固定して倒れ込む
			c.rot_.x += self->deathRotateSpeed_.x * c.dt_;
			// スケールはX,Zはあまり縮まないがYは大きく縮む
			c.scale_ = {
				self->baseScale_.x, // X軸はあまり縮まない
				self->baseScale_.y* s_ * 0.2f, // Y軸は大きく縮む
				self->baseScale_.z // Z軸はあまり縮まない
			};
		}
		// 死亡リアクション関数の例：ボスの最終死亡リアクション（空中に打ち上げられて爆発）
		static void Death_BossFinal(Enemy* self, DeathCtx& c) {
			const float launchStartT_ = 0.5f; // 打ち上げ開始までの時間

			// 打ち上げ開始前は、被弾方向に吹き飛びつつ震える
			if (c.t_ < launchStartT_) {
				float shakeAmp_ = 0.25f; // 震えの振幅
				float shakeFreq_ = 18.0f; // 震えの周波数
				float pulse_ = 1.0f + 0.10f * sinf(self->deathTimer_ * 10.0f); // 打ち上げ前の脈動

				// 被弾方向に吹き飛ぶ
				c.pos_.x += sinf(self->deathTimer_ * shakeFreq_) * shakeAmp_;
				c.pos_.y += cosf(self->deathTimer_ * shakeFreq_ * 0.7f) * shakeAmp_ * 0.6f;
				// 打ち上げ前の脈動
				c.scale_ = {
					self->baseScale_.x * pulse_,
					self->baseScale_.y * pulse_,
					self->baseScale_.z * pulse_,
				};

				TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();

				// 打ち上げ開始前はまばらに爆散する
				if (std::rand() % 3 != 0) {
					Vector3 center_ = self->GetWorldPosition(); // 爆散の中心は敵の現在位置
					Vector3 off_ = { // -0.5..0.5 のランダムオフセットを当たり判定スケールに応じて生成
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.x,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.y,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.z
					};
					Vector3 emitPos_ = center_ + off_ * 0.5f; // 爆散の位置は中心から少しランダムにオフセット
					pm_->Emit("bossDeath_bomb", emitPos_, 1); // 爆散のパーティクルを1つ放出
				}

			} else { // 打ち上げ開始後は空中に打ち上げられて回転しつつ縮む

				// 打ち上げ開始時に一度だけ、打ち上げ開始位置を記録して爆発エフェクトを放出
				if (!self->bossFinalLaunchStarted_) {
					self->bossFinalLaunchStarted_ = true; // 打ち上げ開始フラグを立てる
					self->bossFinalLaunchStartPos_ = c.pos_; // 打ち上げ開始位置を記録

					TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
					Vector3 center_ = self->GetWorldPosition(); // 爆発の中心は敵の現在位置

					// 打ち上げ開始時の爆発エフェクトを多めに放出
					pm_->Emit("bossDeath_ring", center_, 2); // リングエフェクトを2つ放出
					pm_->Emit("bossDeath_bomb", center_, 10); // 爆散エフェクトを10個放出
					pm_->Emit("bossDeath_smoke", center_, 24); // 煙エフェクトを24個放出
				}
				// 打ち上げ開始からの経過時間に応じて、0..1の値を計算
				float u_ = (c.t_ - launchStartT_) / (1.0f - launchStartT_);
				if (u_ < 0.0f) u_ = 0.0f; // 念のため0未満は切り捨て
				if (u_ > 1.0f) u_ = 1.0f; // 念のため1より大きいのも切り捨て
				// 打ち上げの動きは、時間経過に応じて加速していくように、uを3乗してイージングする
				float k_ = u_ * u_ * u_;

				Vector3 upDir_ = { 0.0f, 1.0f, 0.0f }; // 打ち上げの方向は上向き固定
				Vector3 forwardDir_ = { 0.0f, 0.0f, 1.0f }; // 打ち上げの前方向はZ軸正方向固定
				float s_ = 1.0f - 0.3f * k_; // 時間経過に応じて0.7まで縮むようにする
				float upDist_ = 15.0f; // 打ち上げの初速は、時間経過に応じて加速していくように、kを掛ける
				float depthDist_ = 40.0f; // 打ち上げの前方向の動きも加えると、より派手になるので、同様にkを掛ける
				// 打ち上げ開始位置から、上方向と前方向に距離を加算していく
				c.pos_ = self->bossFinalLaunchStartPos_
					+ upDir_ * (upDist_ * k_)
					+ forwardDir_ * (depthDist_ * k_);
				// 打ち上げと同時に回転も加速していく
				c.rot_.x += 2.5f * c.dt_;
				c.rot_.y += 3.0f * c.dt_;
				c.rot_.z += 1.5f * c.dt_;
				// 最小スケールは0.1にする
				if (s_ < 0.1f) s_ = 0.1f;
				// スケールは均等に縮む
				c.scale_ = {
					self->baseScale_.x * s_,
					self->baseScale_.y * s_,
					self->baseScale_.z * s_,
				};
			}
		}

		// 行動関数の例：まっすぐ移動して、指定Zで止まる
		static void Move_StraightStop(Enemy* self, MoveCtx& c) {
			// まっすぐ移動
			if (!self->stopMove_) {
				c.pos_ += self->velocity_ * c.factor_;
				if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; self->stopMove_ = true; }
			}
		}
		// 行動関数の例：X軸方向にサイン波移動しながら、指定Zで止まる
		static void Move_SineX(Enemy* self, MoveCtx& c) {
			self->t_ += 0.05f * c.factor_; // サイン波の位相を時間経過に応じて進める
			c.pos_.z += self->velocity_.z * c.factor_; // Z方向にはまっすぐ移動
			c.pos_.x = self->startX_ + std::sinf(self->sinePhase_ + self->t_ * self->sineFreq_) * self->sineAmpX_; // X方向はサイン波移動
			// Zが指定値を越えたら止まる
			if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; }
		}
		// 行動関数の例：X軸方向に往復移動しながら、指定Zで止まる
		static void Move_StrafeLtoR(Enemy* self, MoveCtx& c) {
			c.pos_.z += self->velocity_.z * c.factor_; // Z方向にはまっすぐ移動
			self->strafePosX_ += self->strafeSpeed_ * self->strafeDir_ * c.factor_; // X方向は往復移動
			// Xが指定範囲を越えたら反転する
			if (self->strafePosX_ > self->strafeRight_) { self->strafePosX_ = self->strafeRight_; self->strafeDir_ = -1; }
			// Zが指定値を越えたら止まる
			if (self->strafePosX_ < self->strafeLeft_) { self->strafePosX_ = self->strafeLeft_;  self->strafeDir_ = +1; }
			c.pos_.x = self->strafePosX_; // X位置を更新
			// Zが指定値を越えたら止まる
			if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; }
		}
		// 行動関数の例：プレイヤーを追いかけながら、指定Zで止まる
		static void Move_ChasePlayer(Enemy* self, MoveCtx& c) {
			c.pos_.z += self->velocity_.z * c.factor_; // Z方向にはまっすぐ移動
			// プレイヤーの位置を取得できる場合は、プレイヤーを追いかける
			if (self->playerGetter_) {
				Vector3 toP_ = self->playerGetter_() - c.pos_; // プレイヤーへのベクトル
				Vector3 desire_ = { toP_.x, toP_.y, 0.0f }; // Z方向は無視して、X,Y方向のベクトルだけで追いかける
				float len_ = MyMath::Length(desire_); // プレイヤーへの距離
				// ある程度距離がある場合だけ追いかける（近すぎると振動してしまうのを防止）
				if (len_ > 0.001f) {
					Vector3 dir = MyMath::Normalize(desire_); // プレイヤーへの方向ベクトル
					// プレイヤーへの方向に移動する
					c.pos_.x += dir.x * self->chaseSpeed_ * c.factor_;
					c.pos_.y += dir.y * self->chaseSpeed_ * c.factor_;
				}
			}
			// Zが指定値を越えたら止まる
			if (c.pos_.z <= self->stopZ_) { c.pos_.z = self->stopZ_; }
		}
		// 行動関数の例：空からプレイヤーに向かって急降下する
		static void Move_PounceFromAbove(Enemy* self, MoveCtx& c) {
			if (!self->pounceStarted_) { return; }

			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
			// 急降下開始前は、空中でホバリングしている状態
			if (!self->pounceDiving_) {
				self->pounceTime_ += c.dt_; // 急降下開始からの経過時間を更新
				float t_ = self->pounceTime_ / self->pounceDuration_; // 急降下開始からの経過時間を、急降下の全体時間で割って、0..1の値にする
				if (t_ > 1.0f) t_ = 1.0f; // 念のため1より大きいのも切り捨て
				// 急降下の動きは、時間経過に応じて加速していくように、イージングする
				auto EaseOutQuad_ = [](float x) {
					return 1.0f - (1.0f - x) * (1.0f - x); // イージング関数（EaseOutQuad）
					};
				float u_ = EaseOutQuad_(t_); // 急降下開始からの経過時間に応じて、0..1の値をイージングして計算

				// 急降下の軌道は、急降下開始位置から急降下の頂点を経由して、急降下の目標位置に向かう放物線を想定して、2段階の線形補間で計算する
				Vector3 pos1_ = MyMath::Vector3Lerp(self->pounceStart_, self->pounceApex_, u_);
				Vector3 pos2_ = MyMath::Vector3Lerp(self->pounceApex_, self->pounceTarget_, u_);
				Vector3 newPos_ = MyMath::Vector3Lerp(pos1_, pos2_, u_);
				// 計算した新しい位置を適用
				c.pos_ = newPos_;

				// 急降下の軌道に沿って、定期的にエフェクトを放出
				{
					Vector3 emitPos_ = c.pos_;
					pm_->Emit("enemyPounceTrail", emitPos_, 2);
					pm_->Emit("enemyPounceSpark", emitPos_, 3);
				}

				// 急降下の全体時間が経過したら、急降下の軌道に沿った移動をやめて、プレイヤーに向かって急降下する状態に切り替える
				if (t_ >= 1.0f) {
					Vector3 dir_ = self->pounceTarget_ - self->pounceStart_; // 急降下の開始位置から目標位置へのベクトルを計算
					float len_ = MyMath::Length(dir_); // ベクトルの長さを計算

					// ベクトルの長さがある程度ある場合は、正規化して方向ベクトルにする。あまりに短い場合は、下方向を向くようにする（急降下の開始位置と目標位置がほぼ同じ場合への対処）
					if (len_ > 0.001f) {
						dir_ = MyMath::Normalize(dir_);
					} else { // ベクトルの長さがほとんどない場合は、下方向を向くようにする
						dir_ = { 0.0f, -0.1f, -1.0f };
					}

					dir_.y -= 0.2f; // 急降下の軌道から少し下向きにすることで、より急降下っぽい動きになる
					dir_ = MyMath::Normalize(dir_); // 方向ベクトルを正規化して、移動の方向だけを残す
					// 急降下の速度を設定
					float diveSpeed_ = 0.7f;
					// 急降下の方向に速度を設定して、急降下する状態に切り替える
					self->velocity_ = dir_ * diveSpeed_;
					// 急降下する状態に切り替えるフラグを立てる
					self->pounceDiving_ = true;
				}

			} else {
				// 急降下する状態：プレイヤーに向かって急降下している状態
				c.pos_ += self->velocity_ * c.factor_;
				// 急降下の軌道に沿って、定期的にエフェクトを放出
				Vector3 emitPos_ = c.pos_;
				pm_->Emit("enemyPounceTrail", emitPos_, 2); // こちらは急降下の軌道に沿った煙のエフェクト
				pm_->Emit("enemyPounceSpark", emitPos_, 2); // こちらは急降下の軌道に沿った火花のエフェクト
			}
		}
		// 行動関数の例：Roam（徘徊） - 一定範囲内でランダムに移動する
		static void Move_FreeRoam(Enemy* self, MoveCtx& c) {
			// 0..1のランダム値を生成する関数
			auto random01_ = []() {
				return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
				};

			float distToTarget_ = MyMath::Length(self->roamTarget_ - c.pos_); // 現在位置と徘徊の目標位置との距離

			// 目標位置が設定されていないか、目標位置に近づきすぎたら、新しい目標位置をランダムに設定する
			if (!self->hasRoamTarget_ || distToTarget_ < 0.5f) {
				self->hasRoamTarget_ = true; // 目標位置が設定されたフラグを立てる
				// 目標位置を、徘徊の範囲を表すローミングエリアの最小値と最大値の間でランダムに生成する
				Vector3 target_;
				target_.x = self->roamMin_.x + (self->roamMax_.x - self->roamMin_.x) * random01_();
				target_.y = self->roamMin_.y + (self->roamMax_.y - self->roamMin_.y) * random01_();
				target_.z = self->roamMin_.z + (self->roamMax_.z - self->roamMin_.z) * random01_();

				// 怒り状態のときは、プレイヤーの位置に向かって少し目標位置を引き寄せる。これにより、怒り状態のときはプレイヤーに向かって徘徊するようになる
				if (self->isAngry_ && self->playerGetter_) {
					Vector3 p = self->playerGetter_(); // プレイヤーの位置を取得
					target_.x = (target_.x * 0.4f) + (p.x * 0.6f); // 目標位置をプレイヤーの位置に向かって引き寄せる（0.4は元の目標位置の重み、0.6はプレイヤーの位置の重み。これらを調整することで、怒り状態のときの徘徊の傾向を変えられる）
					target_.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, target_.x)); // 目標位置がローミングエリアの範囲内に収まるようにする
				}
				// 新しい目標位置を設定する
				self->roamTarget_ = target_;
			}

			Vector3 toT_ = self->roamTarget_ - c.pos_; // 現在位置から目標位置へのベクトル
			float len_ = MyMath::Length(toT_); // 現在位置から目標位置への距離

			// ある程度距離がある場合だけ移動する（近すぎると振動してしまうのを防止）
			if (len_ > 0.001f) {
				Vector3 dir_ = toT_ / len_; // 現在位置から目標位置への方向ベクトル（正規化されたベクトル）
				float speed_ = self->isAngry_ ? self->roamSpeedAngry_ : self->roamSpeedNormal_; // 怒り状態のときは速く移動する
				c.pos_ += dir_ * speed_ * c.factor_; // 目標位置に向かって移動する
			}
			// 目標位置に向かって移動した後、念のため位置がローミングエリアの範囲内に収まるようにする
			c.pos_.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, c.pos_.x));
			c.pos_.y = std::max(self->roamMin_.y, std::min(self->roamMax_.y, c.pos_.y));
			c.pos_.z = std::max(self->roamMin_.z, std::min(self->roamMax_.z, c.pos_.z));
		}
	};

	// 死亡リアクションテーブル（enum順：BlowAway, RiseAbsorb, Collapse, BossFinal）
	using DeathFn = void(*)(Enemy*, DeathCtx&);
	// 死亡リアクションテーブルは、EnemyDeathReactionのenum値をインデックスにして、対応する関数を呼び出せるようにする
	static const DeathFn kDeathTable_[] = {
		&Local::Death_BlowAway,
		&Local::Death_RiseAbsorb,
		&Local::Death_Collapse,
		&Local::Death_BossFinal,
	};

	// 行動テーブル（enum順：StraightStop, SineX, StrafeLtoR, ChasePlayer, PounceFromAbove, FreeRoam）
	using MoveFn = void(*)(Enemy*, MoveCtx&);
	// 行動テーブルは、EnemyBehaviorのenum値をインデックスにして、対応する関数を呼び出せるようにする
	static const MoveFn kMoveTable_[] = {
		&Local::Move_StraightStop,
		&Local::Move_SineX,
		&Local::Move_StrafeLtoR,
		&Local::Move_ChasePlayer,
		&Local::Move_PounceFromAbove,
		&Local::Move_FreeRoam,
	};

	// =========================================================
	// 死亡演出（Data-driven）
	// =========================================================

	// 死亡中の更新
	if (isDying_) {
		deathTimer_ += dt; // 死亡リアクション開始からの経過時間を更新
		float t_ = std::min(deathTimer_ / deathDuration_, 1.0f); // 死亡リアクション開始からの経過時間を、死亡リアクションの全体時間で割って、0..1の値にする
		// 死亡リアクションのコンテキストを作成して、関数テーブルから呼び出す
		DeathCtx c_{};
		c_.dt_ = dt;
		c_.t_ = t_;
		c_.pos_ = object_->GetTranslate();
		c_.rot_ = object_->GetRotate();
		c_.scale_ = baseScale_;
		// 死亡リアクションの関数を呼び出す
		const int di_ = static_cast<int>(deathReaction_);
		// 念のため、deathReaction_がテーブルの範囲内の値であるかをチェックしてから呼び出す
		if (0 <= di_ && di_ < static_cast<int>(std::size(kDeathTable_))) {
			kDeathTable_[di_](this, c_);
		}
		// コンテキストの更新結果を敵オブジェクトに適用する
		object_->SetTranslate(c_.pos_);
		object_->SetRotate(c_.rot_);
		object_->SetScale(c_.scale_);

		// 死亡リアクションの経過時間に応じて、敵の透明度を変化させる
		if (deathReaction_ == EnemyDeathReaction::BossFinal) {
			// ボスの最終死亡リアクションは、前半は透明度1.0のままで、後半で徐々に透明になるようにする
			if (t_ < 0.7f) {
				deathAlpha_ = 1.0f;
			} else { // 0.7秒以降は徐々に透明になる
				float u_ = (t_ - 0.7f) / 0.3f; // 0.7秒から1.0秒の間で、0..1の値を計算
				// 念のため0未満は切り捨て、1より大きいのも切り捨て
				if (u_ > 1.0f) u_ = 1.0f;
				// 透明度は、uをそのまま使うのではなく、イージングして変化させると、より自然な感じになる。ここでは、uを2乗してイージングする
				deathAlpha_ = 1.0f - u_;
			}
		} else { // その他の死亡リアクションは、経過時間に応じて線形に透明になるようにする
			deathAlpha_ = 1.0f - t_; // 経過時間が0のときは透明度1.0、経過時間が死亡リアクションの全体時間以上のときは透明度0.0になるようにする
		}
		// 敵オブジェクトの色に透明度を適用する（RGBはそのままで、AにdeathAlpha_を設定する）
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });
		// 死亡中は移動や攻撃などの行動はしないので、ここで更新を終える
		object_->Update();

		// 死亡リアクションの経過時間が死亡リアクションの全体時間を超えたら、完全に死亡した状態になる
		if (deathTimer_ >= deathDuration_) {
			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();
			Vector3 emitPos_ = GetWorldPosition(); // 敵の現在位置をパーティクルの発生位置とする

			// 死亡リアクションに応じたパーティクルを発生させる
			if (deathReaction_ == EnemyDeathReaction::BlowAway) { // 吹き飛びの死亡
				pm_->Emit("enemyDeath_core", emitPos_, 1);
				pm_->Emit("enemyDeath_shard", emitPos_, 20);
				pm_->Emit("enemyDeath_smoke", emitPos_, 4);
			} else if (deathReaction_ == EnemyDeathReaction::RiseAbsorb) { // 浮き上がり吸収の死亡
				pm_->Emit("enemyDeath_core", emitPos_, 1);
				pm_->Emit("enemyDeath_shard", emitPos_, 14);
				pm_->Emit("enemyDeath_smoke", emitPos_, 6);
			} else if (deathReaction_ == EnemyDeathReaction::Collapse) { // 崩れ落ちの死亡
				pm_->Emit("enemyDeath_shard", emitPos_, 10);
				pm_->Emit("enemyDeath_smoke", emitPos_, 3);
			} else if (deathReaction_ == EnemyDeathReaction::BossFinal) { // ボスの最終死亡
				if (!bossFinalBigBurstDone_) {
					pm_->Emit("bossClear_core", emitPos_, 1);
					pm_->Emit("bossClear_ring", emitPos_, 3);
					pm_->Emit("bossClear_spark", emitPos_, 80);
					pm_->Emit("bossClear_debris", emitPos_, 60);
				}
			}
			// 死亡フラグを立てる
			isDead_ = true;
		}

		// 死亡中も触手を更新して、親の動きに追従させる
		if (tentacle_) {
			tentacle_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ }); // 触手も同じアルファで更新

			// 死亡中も触手を回転させたい場合はここで回転させる（例：ゆっくり回転させる）
			if (type_ != EnemyType::Boss) {
				tentacleLocalRot_.y += 0.1f * factor_;
			}
			// 死亡中も取り付け位置を毎フレ反映したいなら
			tentacle_->SetTranslate(tentacleLocalPos_);
			tentacle_->SetRotate(tentacleLocalRot_);
			tentacle_->SetScale(tentacleLocalScale_);
			// 死亡中も更新して、親の動きに追従させる
			tentacle_->Update();
		}

		return;
	}

	// =========================================================
	// 怒りタイマー更新
	// =========================================================
	if (isAngry_) {
		angryTimer_ += dt; // 怒り状態の経過時間を更新

		// 怒り状態の経過時間が怒り状態の持続時間を超えたら、怒り状態を解除する
		if (angryTimer_ >= angryDuration_) {
			isAngry_ = false;
		}
	}

	// =========================================================
	// 行動（Data-driven）
	// =========================================================

	// 移動のコンテキストを作成して、関数テーブルから呼び出す
	MoveCtx m_{};
	m_.dt_ = dt;
	m_.factor_ = factor_;
	m_.pos_ = object_->GetTranslate();

	// 移動の関数を呼び出す
	if (!freezeMove_) {
		const int bi_ = static_cast<int>(behavior_); // 行動パターンを表すbehavior_を整数にキャストして、テーブルのインデックスとして使う

		// 念のため、behavior_がテーブルの範囲内の値であるかをチェックしてから呼び出す
		if (0 <= bi_ && bi_ < static_cast<int>(std::size(kMoveTable_))) {
			kMoveTable_[bi_](this, m_); // 行動関数を呼び出す
		}
	}
	// 行動関数の更新結果を敵オブジェクトに適用する
	object_->SetTranslate(m_.pos_);

	// 敵が画面奥に逃げたら、逃げフラグを立てて死亡させる
	if (!isDying_) {
		// 逃げるのは、ボス以外の通常の敵だけにする
		if (m_.pos_.z < -30.0f) {
			escaped_ = true; // 逃げフラグを立てる
			isDead_ = true; // 死亡フラグを立てる
		}
	}

	/// 当たり判定可視化=================================================
#ifdef USE_IMGUI
	// AABB 表示（そのまま）
	{
		Vector3 center_ = GetWorldPosition();
		Vector3 size_ = colliderScale_;

		auto* lr_ = TKM::LineRenderer::GetInstance();

		TKM::LineRenderer::Color normal_{ 0.0f, 1.0f, 0.0f, 1.0f };
		TKM::LineRenderer::Color hit_{ 1.0f, 0.0f, 0.0f, 1.0f };

		if (reticle_) {
			Vector3 rayOrigin_;
			if (playerGetter_) {
				rayOrigin_ = playerGetter_();
			} else {
				rayOrigin_ = reticle_->GetCenterWorldPos();
			}

			Vector3 rayDir_ = reticle_->GetAimDirection();
			lr_->AddAABBWithRayHighlight(center_, size_, rayOrigin_, rayDir_, normal_, hit_);
		} else {
			lr_->AddAABB(center_, size_, normal_);
		}
	}
#endif
	/// ==============================================================

	// ロック脈動（そのまま）
	if (isLocked_ && lockPulseEnabled_) {
		pulseT_ += 0.12f * factor_;
		float s_ = 1.0f + 0.15f * sinf(pulseT_);
		object_->SetScale({ baseScale_.x * s_, baseScale_.y * s_, baseScale_.z * s_ });
	} else {
		object_->SetScale(baseScale_);
	}

	// 射撃（そのまま）
	if (canShoot_ && !isDying_) {
		shootTimer_ += factor_; // フレーム加算→dt換算
		if (shootTimer_ >= shootInterval_) {
			shootTimer_ = 0.0f;
		}
	}

	object_->Update();

	// 触手も同じアルファで更新
	tentacle_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

	// --- テンタクル回転（Y軸くるくる） ---
	// ※ボスは回転させない
	if (type_ != EnemyType::Boss) {
		tentacleLocalRot_.y += 0.1f * factor_; // 回転速度調整
	}

	// 取り付け位置を毎フレ反映したいなら（調整中なら便利）
	tentacle_->SetTranslate(tentacleLocalPos_);
	tentacle_->SetRotate(tentacleLocalRot_);
	tentacle_->SetScale(tentacleLocalScale_);
	tentacle_->Update();
}

void Enemy::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_) return;
	object_->Draw(dxCommon); // 傘
	if (!tentacle_) return;
	tentacle_->Draw(dxCommon); // 触手
}

void Enemy::SetHP(int hp) {
	hp_ = hp;
	maxHP_ = hp;
}

void Enemy::SetModel(const std::string& modelName) {
	if (object_) object_->SetModel(modelName);
}

void Enemy::SetScale(const Vector3& scale) {
	baseScale_ = scale; // 元のスケールを更新
	if (object_) object_->SetScale(scale); // Object3d にも反映
}

void Enemy::SetRotate(const Vector3& rot) {
	if (!object_) { return; }
	object_->SetRotate(rot);
}

void Enemy::SetCamera(TKM::Camera* camera) {
	camera_ = camera;
	if (object_) { object_->SetCamera(camera); } // カメラ設定（既存に合わせる）
	if (tentacle_) { tentacle_->SetCamera(camera); } // カメラ設定（既存に合わせる）
}
void Enemy::SetPosition(const Vector3& pos) {
	if (object_) { object_->SetTranslate(pos); } // 位置設定
}
void Enemy::SetParentScene(TKM::BaseScene* scene) {
	parentScene_ = scene; // 親シーン設定
	if (object_) { object_->SetParentScene(scene); } // シーン設定（既存に合わせる）
	if (tentacle_) { tentacle_->SetParentScene(scene); } // シーン設定（既存に合わせる）
}
void Enemy::SetLocked(bool v) {
	isLocked_ = v; if (!v) pulseT_ = 0.0f; // ロック解除で脈動リセット
}
void Enemy::SetColliderScale(const Vector3& s) {
	colliderScale_ = s; // 当たり判定スケール設定（必要ならAABBも更新）
}
void Enemy::SetBehavior(EnemyBehavior b) {
	behavior_ = b; // 行動パターン設定
}
void Enemy::SetVelocity(const Vector3& v) {
	velocity_ = v; // 移動速度設定
}
void Enemy::SetStopZ(float z) {
	stopZ_ = z; // 停止Z位置設定
}
void Enemy::SetSineParams(float ampX, float freq) {
	sineAmpX_ = ampX; sineFreq_ = freq; // サイン波移動のパラメータ設定
}
void Enemy::SetStrafeX(float left, float right, float speed) {
	strafeLeft_ = left; strafeRight_ = right; strafeSpeed_ = speed; // 左右移動の範囲と速度設定
	if (strafePosX_ == 0.0f) strafePosX_ = left; // 初期位置が0のままなら左端からスタート
}
void Enemy::SetCanShoot(bool v, float interval) {
	canShoot_ = v; shootInterval_ = interval; // 射撃可能にしたとき、タイマーをリセットしてすぐ撃てるようにする
}
void Enemy::SetPlayer(std::function<Vector3()> getter) {
	playerGetter_ = std::move(getter); // プレイヤー位置取得関数設定
}
void Enemy::SetSinePhase(float rad) {
	sinePhase_ = rad; // サイン波移動の位相設定
}
void Enemy::SetReticle(Reticle* r) {
	reticle_ = r; // レティクル設定
}
void Enemy::SetPounceParameters(const Vector3& start, const Vector3& apex, const Vector3& target, float duration) {
	pounceStart_ = start; // ジャンプ開始位置設定
	pounceApex_ = apex; // ジャンプ頂点位置設定
	pounceTarget_ = target; // ジャンプ着地位置設定
	pounceDuration_ = duration; // ジャンプ時間設定
	pounceTime_ = 0.0f; // ジャンプタイマーリセット
	pounceStarted_ = true; // ジャンプ開始フラグを立てる
	pounceDiving_ = false; // ジャンプ開始フラグとダイブ開始フラグをリセット
}
void Enemy::SetType(EnemyType t) {
	type_ = t; // 敵の種類設定
	lockPulseEnabled_ = (type_ != EnemyType::Boss); // ボスはロック脈動無効
	if (!lockPulseEnabled_) { pulseT_ = 0.0f; } // ロック脈動無効なら脈動タイマーもリセット
}
void Enemy::SetFreeRoamArea(const Vector3& min, const Vector3& max, float normalSpeed, float angrySpeed) {

}
void Enemy::SetAngry(float duration) {
	isAngry_ = true; // 怒り状態にする
	angryDuration_ = duration; // 怒り持続時間設定
	angryTimer_ = 0.0f; // 怒りタイマーリセット
}
void Enemy::SetFreezeMove(bool v) {
	freezeMove_ = v; // 移動凍結設定
}
void Enemy::SetCurrentHP(int hp) {
	if (hp < 0) { hp = 0; } // HPが0未満にならないようにする
	if (hp > maxHP_) { hp = maxHP_; } // HPが最大HPを超えないようにする
	hp_ = hp; // 現在HP設定
}
void Enemy::SetTentacleModel(const std::string& modelName) {
	if (tentacle_) tentacle_->SetModel(modelName); // モデル設定
}
void Enemy::SetTentacleLocal(const Vector3& pos, const Vector3& rot, const Vector3& scale) {
	tentacleLocalPos_ = pos; // ローカル位置設定
	tentacleLocalRot_ = rot; // ローカル回転設定
	tentacleLocalScale_ = scale; // ローカルスケール設定
}
void Enemy::SetRoamArea(const Vector3& min, const Vector3& max) {
	roamMin_ = min;
	roamMax_ = max;
	hasRoamTarget_ = false; // 目標を作り直す
}
void Enemy::SetRoamSpeed(float normal, float angry) {
	roamSpeedNormal_ = normal;
	roamSpeedAngry_ = angry;
}
Vector3 Enemy::GetWorldPosition() const {
	if (!object_) {
		return { 0.0f, 0.0f, 0.0f }; // オブジェクトがない場合は原点を返す
	}
	return object_->GetTranslate(); // ワールド位置を返す
}

void Enemy::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	ImGui::Begin("Enemy");

	Vector3 pos_ = object_->GetTranslate();
	Vector3 rot_ = object_->GetRotate();
	Vector3 scale_ = object_->GetScale();

	if (ImGui::DragFloat3("位置", &pos_.x, 0.01f)) {
		object_->SetTranslate(pos_);
	}
	if (ImGui::DragFloat3("回転", &rot_.x, 0.01f)) {
		object_->SetRotate(rot_);
	}
	if (ImGui::DragFloat3("拡縮", &scale_.x, 0.01f)) {
		SetScale(scale_);   // モデルと当たり判定両方に反映される
	}

	// 当たり判定スケール編集
	Vector3 col_ = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ", &col_.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col_);
	}

	ImGui::Text("HP: %d / %d", hp_, maxHP_);
	ImGui::Text("生死: %s", isDead_ ? "死" : "生");

	ImGui::End();
#endif
}

void Enemy::OnHitWithDamage(int damage) {
	// すでに死んでる or 死亡演出中なら無視
	if (isDead_ || isDying_) {
		return;
	}

	hp_ -= damage;
	if (hp_ <= 0) {
		hp_ = 0;
		// ボスなら専用死亡演出、それ以外は従来通り
		if (type_ == EnemyType::Boss) {
			StartBossDeathReaction({ 0.0f, 0.0f, 1.0f });
		} else {
			StartDeathReaction({ 0.0f, 0.0f, 1.0f });
		}
	}
}

void Enemy::StartDeathReaction(const Vector3& hitDir) {
	if (isDying_) {
		return;
	}

	// ボスなら共通処理は使わず専用リアクションへ
	if (type_ == EnemyType::Boss) {
		StartBossDeathReaction(hitDir); // ボス専用死亡リアクション
		return;
	}

	freezeMove_ = true; // 死んだ瞬間に動き停止
	defeated_ = true; // 敵撃破フラグをtrueに
	isDying_ = true; // 死亡演出中フラグを立てるtrueに
	deathTimer_ = 0.0f; // タイマーリセット
	deathAlpha_ = 1.0f; // アルファ初期値

	// 0,1,2 のどれかをランダムに選ぶ
	int r_ = std::rand() % 3;

	// 共通で使うノックバック方向
	Vector3 dir_ = hitDir;
	if (MyMath::Length(dir_) < 0.001f) {
		dir_ = { 0.0f, 0.0f, 1.0f };
	}
	dir_ = MyMath::Normalize(dir_);

	// ランダムに選んだ値に応じて、死亡リアクションのパラメータを設定するラムダ関数を呼び出す
	auto Pick0_BlowAway_ = [&]() {
		deathReaction_ = EnemyDeathReaction::BlowAway;
		deathDuration_ = 3.0f;
		deathVelocity_ = dir_ * 4.0f;
		deathRotateSpeed_ = { 1.5f, 2.0f, 0.8f };
		};
	// RiseAbsorb は、上に浮き上がりながら回転して、最後にゆっくり消える感じのリアクション。dir_はあまり関係ないけど、少しだけ前方に飛ばす方向を入れてもいいかも
	auto Pick1_RiseAbsorb_ = [&]() {
		deathReaction_ = EnemyDeathReaction::RiseAbsorb;
		deathDuration_ = 1.2f;
		deathVelocity_ = { 0.0f, 3.0f, 0.0f };
		deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
		};
	// Collapse は、ほとんど動かずに地面に崩れ落ちる感じのリアクション。dir_はあまり関係ないけど、少しだけ前方に飛ばす方向を入れてもいいかも
	auto Pick2_Collapse_ = [&]() {
		deathReaction_ = EnemyDeathReaction::Collapse;
		deathDuration_ = 0.9f;
		deathVelocity_ = { dir_.x * 1.5f, -3.0f, dir_.z * 1.5f };
		deathRotateSpeed_ = { 3.0f, 0.5f, 0.0f };
		};
	// ランダムに選んだ値に応じて、死亡リアクションのパラメータを設定するラムダ関数を呼び出す
	if (r_ == 0) { Pick0_BlowAway_(); } else if (r_ == 1) { Pick1_RiseAbsorb_(); } else { Pick2_Collapse_(); }
}

void Enemy::SyncTransform() {
	if (!object_) return;
	if (!tentacle_) return;
	object_->Update();  // 行列と定数バッファだけ更新
	tentacle_->Update(); // 行列と定数バッファだけ更新
}

void Enemy::StartBossDeathReaction(const Vector3& hitDir) {
	if (isDying_) { // すでに死亡演出中なら無視
		return;
	}

	freezeMove_ = true; // 死んだ瞬間に動き停止
	defeated_ = true;   // 敵撃破フラグをtrueに
	isDying_ = true;    // 死亡演出中フラグを立てる
	deathTimer_ = 0.0f; // タイマーリセット
	deathAlpha_ = 1.0f; // アルファ初期値

	// ボス専用リアクション
	deathReaction_ = EnemyDeathReaction::BossFinal;

	// ボスはしっかり見せたいので少し長め
	deathDuration_ = 5.0f;

	// 位置＆回転は BossFinal ブロック側で制御するのでここでは 0
	deathVelocity_ = { 0.0f, 0.0f, 0.0f };
	deathRotateSpeed_ = { 0.0f, 0.0f, 0.0f };

	// BossFinal 用（ぶっ飛び演出の開始をリセット）
	bossFinalLaunchStarted_ = false;
	bossFinalLaunchStartPos_ = object_->GetTranslate();

	// BossFinal 用一時変数リセット
	bossFinalLaunchStarted_ = false;
	bossFinalBigBurstDone_ = false;
	bossFinalCameraInited_ = false;
}