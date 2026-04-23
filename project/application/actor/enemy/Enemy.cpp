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
	// 敵本体の3Dオブジェクトを生成する
	object_ = std::make_unique<TKM::Object3d>();

	// 敵本体の描画に必要な共通情報とDirectX情報を渡して初期化する
	object_->Initialize(common, dxCommon);

	// 触手用の3Dオブジェクトを生成する
	tentacle_ = std::make_unique<TKM::Object3d>();

	// 触手の描画に必要な共通情報とDirectX情報を渡して初期化する
	tentacle_->Initialize(common, dxCommon);

	// すでにカメラが設定済みなら、本体と触手の両方に同じカメラを適用する
	if (camera_) {
		object_->SetCamera(camera_);
		tentacle_->SetCamera(camera_);
	}

	// 触手を敵本体の子にして、本体の移動や回転に追従させる
	tentacle_->SetParent(object_.get());

	// 触手のローカル座標・ローカル回転・ローカル拡縮を初期値で反映する
	tentacle_->SetTranslate(tentacleLocalPos_);
	tentacle_->SetRotate(tentacleLocalRot_);
	tentacle_->SetScale(tentacleLocalScale_);

	// 当たり判定やロック脈動で使う基準スケールを保存する
	baseScale_ = object_->GetScale();

	// サイン移動の基準位置として、初期X座標を保存しておく
	startX_ = object_->GetTranslate().x;
}

void Enemy::Update(float dt) {

	// 60fps基準で作った速度や加算値を、そのまま流用できるようにする補正値
	// dt = 1/60 のとき factor_ = 1.0 になる
	const float factor_ = dt * 60.0f;

	//=========================================================
	// Data-driven：死亡リアクション / 行動 の関数テーブル
	//=========================================================

	// 死亡演出の更新に必要な情報をまとめるコンテキスト
	struct DeathCtx {
		float dt_;      // 前フレームからの経過時間
		float t_;       // 死亡演出の進行率（0.0f ～ 1.0f）
		Vector3 pos_;   // 現在位置
		Vector3 rot_;   // 現在回転
		Vector3 scale_; // 現在スケール
	};

	// 通常行動の更新に必要な情報をまとめるコンテキスト
	struct MoveCtx {
		float dt_;      // 前フレームからの経過時間
		float factor_;  // 60fps基準補正値
		Vector3 pos_;   // 現在位置
	};

	// 各行動・各死亡演出の処理本体をまとめるローカル構造体
	struct Local {

		// 被弾方向へ吹き飛びながら回転し、最後に縮んで消える死亡演出
		static void Death_BlowAway(Enemy* self, DeathCtx& c) {
			// 時間経過に応じて移動速度を弱める
			float speed_ = 1.0f - c.t_;

			// 時間経過に応じて縮小率を下げる
			float s_ = 1.0f - c.t_;

			// 被弾方向に吹き飛ばす
			c.pos_ += self->deathVelocity_ * speed_ * c.dt_;

			// 各軸の回転速度を加算して、吹き飛び中に回転させる
			c.rot_.x += self->deathRotateSpeed_.x * c.dt_;
			c.rot_.y += self->deathRotateSpeed_.y * c.dt_;
			c.rot_.z += self->deathRotateSpeed_.z * c.dt_;

			// XYZを均等に縮めていく
			c.scale_ = {
				self->baseScale_.x * s_,
				self->baseScale_.y * s_,
				self->baseScale_.z * s_
			};
		}

		// 上へ吸い込まれるように浮き上がりながら回転し、細く消えていく死亡演出
		static void Death_RiseAbsorb(Enemy* self, DeathCtx& c) {
			// 時間経過に応じて縮小率を下げる
			float s_ = 1.0f - c.t_;

			// 上方向中心の速度で移動させる
			c.pos_ += self->deathVelocity_ * c.dt_;

			// Y軸だけ回して、吸い込まれるような印象を出す
			c.rot_.y += self->deathRotateSpeed_.y * c.dt_;

			// X,Zは細く縮め、Yは少しだけ残して縦方向に見せる
			c.scale_ = {
				self->baseScale_.x * s_ * 0.5f,
				self->baseScale_.y * (1.0f - c.t_ * 0.2f),
				self->baseScale_.z * s_ * 0.5f
			};
		}

		// 地面に倒れ込むように崩れながら消える死亡演出
		static void Death_Collapse(Enemy* self, DeathCtx& c) {
			// 時間経過に応じて縮小率を下げる
			float s_ = 1.0f - c.t_;

			// やや下方向を含んだ速度で落とす
			c.pos_ += self->deathVelocity_ * c.dt_;

			// 前に倒れるような見た目にするためX軸回転を進める
			c.rot_.x += self->deathRotateSpeed_.x * c.dt_;

			// Yだけ大きく潰して、倒れ込む感じを出す
			c.scale_ = {
				self->baseScale_.x,
				self->baseScale_.y * s_ * 0.2f,
				self->baseScale_.z
			};
		}

		// ボス専用の最終死亡演出
		// 前半は震え＋脈動、後半は打ち上げ＋爆発系の見せ場に切り替える
		static void Death_BossFinal(Enemy* self, DeathCtx& c) {
			// 演出の前半と後半を切り替える境目時間
			const float launchStartT_ = 0.5f;

			// 打ち上げ前の演出
			if (c.t_ < launchStartT_) {
				// 震えの大きさ
				float shakeAmp_ = 0.25f;

				// 震えの速さ
				float shakeFreq_ = 18.0f;

				// ボスが膨張・収縮しているように見せる脈動倍率
				float pulse_ = 1.0f + 0.10f * sinf(self->deathTimer_ * 10.0f);

				// X方向に細かく揺らして不安定感を出す
				c.pos_.x += sinf(self->deathTimer_ * shakeFreq_) * shakeAmp_;

				// Y方向にも少し揺らして完全な横揺れにならないようにする
				c.pos_.y += cosf(self->deathTimer_ * shakeFreq_ * 0.7f) * shakeAmp_ * 0.6f;

				// 本体全体を脈動させる
				c.scale_ = {
					self->baseScale_.x * pulse_,
					self->baseScale_.y * pulse_,
					self->baseScale_.z * pulse_,
				};

				TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();

				// 毎フレ大量に出しすぎないように、確率で爆散パーティクルを出す
				if (std::rand() % 3 != 0) {
					// 爆散の中心を現在の敵位置にする
					Vector3 center_ = self->GetWorldPosition();

					// 当たり判定サイズの範囲内でランダムなオフセットを作る
					Vector3 off_ = {
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.x,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.y,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * self->colliderScale_.z
					};

					// 中心から少しだけ散らした位置に出す
					Vector3 emitPos_ = center_ + off_ * 0.5f;

					// 爆散系パーティクルを1つ放出する
					pm_->Emit("bossDeath_bomb", emitPos_, 1);
				}

			} else {
				// 打ち上げ開始時の一回だけ実行する処理
				if (!self->bossFinalLaunchStarted_) {
					// 二重実行を防ぐため、開始済みフラグを立てる
					self->bossFinalLaunchStarted_ = true;

					// 打ち上げ開始位置を保存し、この座標から後半演出を組み立てる
					self->bossFinalLaunchStartPos_ = c.pos_;

					TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();

					// 爆発の中心はその瞬間の敵位置にする
					Vector3 center_ = self->GetWorldPosition();

					// 打ち上げ直前の派手な爆発をまとめて出す
					pm_->Emit("bossDeath_ring", center_, 2);
					pm_->Emit("bossDeath_bomb", center_, 10);
					pm_->Emit("bossDeath_smoke", center_, 24);
				}

				// 打ち上げ開始後の進行率を 0.0f ～ 1.0f に正規化する
				float u_ = (c.t_ - launchStartT_) / (1.0f - launchStartT_);
				if (u_ < 0.0f) u_ = 0.0f;
				if (u_ > 1.0f) u_ = 1.0f;

				// 立ち上がりを後ろに寄せて、終盤で一気に伸びるようにする
				float k_ = u_ * u_ * u_;

				// 上方向に打ち上げるための固定方向
				Vector3 upDir_ = { 0.0f, 1.0f, 0.0f };

				// 奥方向にも飛ばして、画として派手にする
				Vector3 forwardDir_ = { 0.0f, 0.0f, 1.0f };

				// 後半では少しずつ縮小するが、完全には消し切らない
				float s_ = 1.0f - 0.3f * k_;

				// 上方向への移動量
				float upDist_ = 15.0f;

				// 前方向への移動量
				float depthDist_ = 40.0f;

				// 打ち上げ開始位置から、上＋前へ補間して移動させる
				c.pos_ = self->bossFinalLaunchStartPos_
					+ upDir_ * (upDist_ * k_)
					+ forwardDir_ * (depthDist_ * k_);

				// 後半は全軸回転を追加して、制御不能に飛んでいく感じにする
				c.rot_.x += 2.5f * c.dt_;
				c.rot_.y += 3.0f * c.dt_;
				c.rot_.z += 1.5f * c.dt_;

				// 小さくなりすぎると見えなくなるので下限を設ける
				if (s_ < 0.1f) s_ = 0.1f;

				// XYZを均等に縮小する
				c.scale_ = {
					self->baseScale_.x * s_,
					self->baseScale_.y * s_,
					self->baseScale_.z * s_,
				};
			}
		}

		// Z方向に進み、指定Zまで来たら止まる基本移動
		static void Move_StraightStop(Enemy* self, MoveCtx& c) {
			// 停止済みでなければ前進させる
			if (!self->stopMove_) {
				c.pos_ += self->velocity_ * c.factor_;

				// 停止ラインを越えたら、その位置で止める
				if (c.pos_.z <= self->stopZ_) {
					c.pos_.z = self->stopZ_;
					self->stopMove_ = true;
				}
			}
		}

		// X方向にサイン波で揺れながら前進する移動
		static void Move_SineX(Enemy* self, MoveCtx& c) {
			// サイン波用の時間を進める
			self->t_ += 0.05f * c.factor_;

			// Z方向は通常移動
			c.pos_.z += self->velocity_.z * c.factor_;

			// X方向だけサイン波で左右に揺らす
			c.pos_.x = self->startX_ + std::sinf(self->sinePhase_ + self->t_ * self->sineFreq_) * self->sineAmpX_;

			// 停止ラインを越えたらZだけ止める
			if (c.pos_.z <= self->stopZ_) {
				c.pos_.z = self->stopZ_;
			}
		}

		// X方向に左右往復しながらZ方向へ進む移動
		static void Move_StrafeLtoR(Enemy* self, MoveCtx& c) {
			// Z方向は前進させる
			c.pos_.z += self->velocity_.z * c.factor_;

			// 左右移動位置を現在の向きに応じて進める
			self->strafePosX_ += self->strafeSpeed_ * self->strafeDir_ * c.factor_;

			// 右端を越えたら右端で止めて左向きへ反転する
			if (self->strafePosX_ > self->strafeRight_) {
				self->strafePosX_ = self->strafeRight_;
				self->strafeDir_ = -1;
			}

			// 左端を越えたら左端で止めて右向きへ反転する
			if (self->strafePosX_ < self->strafeLeft_) {
				self->strafePosX_ = self->strafeLeft_;
				self->strafeDir_ = +1;
			}

			// 計算したX位置を反映する
			c.pos_.x = self->strafePosX_;

			// 停止ラインを越えたらZだけ止める
			if (c.pos_.z <= self->stopZ_) {
				c.pos_.z = self->stopZ_;
			}
		}

		// プレイヤーのXY位置を追いながらZ方向へ進む移動
		static void Move_ChasePlayer(Enemy* self, MoveCtx& c) {
			// Z方向は前進させる
			c.pos_.z += self->velocity_.z * c.factor_;

			// プレイヤー位置取得関数があるときだけ追尾する
			if (self->playerGetter_) {
				// 現在位置からプレイヤーまでのベクトルを求める
				Vector3 toP_ = self->playerGetter_() - c.pos_;

				// Z方向は無視してXY平面だけを追尾対象にする
				Vector3 desire_ = { toP_.x, toP_.y, 0.0f };

				// プレイヤーまでの距離を求める
				float len_ = MyMath::Length(desire_);

				// 近すぎると微振動しやすいので、少し離れている時だけ追う
				if (len_ > 0.001f) {
					Vector3 dir = MyMath::Normalize(desire_);

					// X方向へ追尾
					c.pos_.x += dir.x * self->chaseSpeed_ * c.factor_;

					// Y方向へ追尾
					c.pos_.y += dir.y * self->chaseSpeed_ * c.factor_;
				}
			}

			// 停止ラインを越えたらZだけ止める
			if (c.pos_.z <= self->stopZ_) {
				c.pos_.z = self->stopZ_;
			}
		}

		// 空中から接近して、その後プレイヤーへ急降下する行動
		static void Move_PounceFromAbove(Enemy* self, MoveCtx& c) {
			// 急降下開始指示が無ければ何もしない
			if (!self->pounceStarted_) {
				return;
			}

			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();

			// まだ本格ダイブに入っていない段階
			if (!self->pounceDiving_) {
				// 開始からの経過時間を進める
				self->pounceTime_ += c.dt_;

				// 全体時間に対する進行率を計算する
				float t_ = self->pounceTime_ / self->pounceDuration_;
				if (t_ > 1.0f) t_ = 1.0f;

				// 急降下の入りを自然にするためEaseOutQuadを使う
				auto EaseOutQuad_ = [](float x) {
					return 1.0f - (1.0f - x) * (1.0f - x);
					};

				// イージング後の進行率
				float u_ = EaseOutQuad_(t_);

				// 開始地点→頂点、頂点→目標地点の2本を補間し、
				// さらにその間を補間して放物線っぽい軌道を作る
				Vector3 pos1_ = MyMath::Vector3Lerp(self->pounceStart_, self->pounceApex_, u_);
				Vector3 pos2_ = MyMath::Vector3Lerp(self->pounceApex_, self->pounceTarget_, u_);
				Vector3 newPos_ = MyMath::Vector3Lerp(pos1_, pos2_, u_);

				// 計算した軌道上の位置を反映する
				c.pos_ = newPos_;

				// 接近中の軌道にエフェクトを発生させる
				{
					Vector3 emitPos_ = c.pos_;
					pm_->Emit("enemyPounceTrail", emitPos_, 2);
					pm_->Emit("enemyPounceSpark", emitPos_, 3);
				}

				// 接近フェーズが終了したら、直線的なダイブフェーズへ切り替える
				if (t_ >= 1.0f) {
					// 開始地点から目標地点への方向ベクトルを作る
					Vector3 dir_ = self->pounceTarget_ - self->pounceStart_;

					// ベクトル長を求める
					float len_ = MyMath::Length(dir_);

					// 十分な長さがあるなら正規化して使う
					if (len_ > 0.001f) {
						dir_ = MyMath::Normalize(dir_);
					} else {
						// ほぼ同位置なら、仮の下向きベクトルを使う
						dir_ = { 0.0f, -0.1f, -1.0f };
					}

					// 少しだけ下向き成分を増やして、急降下感を強める
					dir_.y -= 0.2f;

					// 再正規化して方向だけ残す
					dir_ = MyMath::Normalize(dir_);

					// ダイブ速度を設定する
					float diveSpeed_ = 0.7f;

					// 以後の移動に使う速度として保存する
					self->velocity_ = dir_ * diveSpeed_;

					// 本格ダイブ開始フラグを立てる
					self->pounceDiving_ = true;
				}

			} else {
				// ダイブ開始後は保存済み速度で直進する
				c.pos_ += self->velocity_ * c.factor_;

				// ダイブ中も軌跡エフェクトを継続して出す
				Vector3 emitPos_ = c.pos_;
				pm_->Emit("enemyPounceTrail", emitPos_, 2);
				pm_->Emit("enemyPounceSpark", emitPos_, 2);
			}
		}

		// 指定範囲内をランダムに徘徊する行動
		static void Move_FreeRoam(Enemy* self, MoveCtx& c) {
			// 0.0f ～ 1.0f のランダム値を返す簡易関数
			auto random01_ = []() {
				return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
				};

			// 現在の目標位置までの距離を求める
			float distToTarget_ = MyMath::Length(self->roamTarget_ - c.pos_);

			// まだ目標未設定、または目標に近づいたら次の目標を作り直す
			if (!self->hasRoamTarget_ || distToTarget_ < 0.5f) {
				// 目標設定済みフラグを立てる
				self->hasRoamTarget_ = true;

				// ローム範囲内のランダム位置を新しい目標にする
				Vector3 target_;
				target_.x = self->roamMin_.x + (self->roamMax_.x - self->roamMin_.x) * random01_();
				target_.y = self->roamMin_.y + (self->roamMax_.y - self->roamMin_.y) * random01_();
				target_.z = self->roamMin_.z + (self->roamMax_.z - self->roamMin_.z) * random01_();

				// 怒り状態なら、ランダム目標をプレイヤー寄りに補正して圧を出す
				if (self->isAngry_ && self->playerGetter_) {
					Vector3 p = self->playerGetter_();

					// X方向をプレイヤー寄りに寄せる
					target_.x = (target_.x * 0.4f) + (p.x * 0.6f);

					// 範囲外へ出ないようにクランプする
					target_.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, target_.x));
				}

				// 新しく決めた目標を保存する
				self->roamTarget_ = target_;
			}

			// 現在位置から目標へのベクトルを計算する
			Vector3 toT_ = self->roamTarget_ - c.pos_;

			// 目標までの距離を計算する
			float len_ = MyMath::Length(toT_);

			// 少し離れている時だけ目標へ向かって動かす
			if (len_ > 0.001f) {
				Vector3 dir_ = toT_ / len_;

				// 怒り状態なら速度を上げる
				float speed_ = self->isAngry_ ? self->roamSpeedAngry_ : self->roamSpeedNormal_;

				// 目標方向へ移動する
				c.pos_ += dir_ * speed_ * c.factor_;
			}

			// 最終的にローム範囲外へ出ないように位置を補正する
			c.pos_.x = std::max(self->roamMin_.x, std::min(self->roamMax_.x, c.pos_.x));
			c.pos_.y = std::max(self->roamMin_.y, std::min(self->roamMax_.y, c.pos_.y));
			c.pos_.z = std::max(self->roamMin_.z, std::min(self->roamMax_.z, c.pos_.z));
		}

		// 指定された隊列位置まで移動する行動
		static void Move_FormationMove(Enemy* self, MoveCtx& c) {
			// 現在位置から目標位置までのベクトルを求める
			Vector3 toTarget_ = self->formationTarget_ - c.pos_;

			// 目標までの距離を求める
			float dist_ = MyMath::Length(toTarget_);

			// 十分近ければ到達扱いにして位置を確定する
			if (dist_ <= self->formationArriveEpsilon_) {
				c.pos_ = self->formationTarget_;
				self->isInFormation_ = true;
				return;
			}

			// まだ到達していない状態にする
			self->isInFormation_ = false;

			// 距離が極端に小さくない時だけ正規化する
			if (dist_ > 0.0001f) {
				toTarget_ = toTarget_ / dist_;
			}

			// 目標方向へ一定速度で移動する
			c.pos_ += toTarget_ * self->formationMoveSpeed_ * c.factor_;

			// 本体が存在するなら、向いている方向を少し進行方向に合わせる
			if (self->object_) {
				Vector3 rot_ = self->object_->GetRotate();

				// Y軸だけ目標方向へ向けて、姿勢を大きく崩しすぎないようにする
				rot_.y = atan2f(toTarget_.x, -toTarget_.z);

				self->object_->SetRotate(rot_);
			}
		}
	};

	// 死亡演出 enum と実処理関数を対応付けるテーブル
	using DeathFn = void(*)(Enemy*, DeathCtx&);
	static const DeathFn kDeathTable_[] = {
		&Local::Death_BlowAway,
		&Local::Death_RiseAbsorb,
		&Local::Death_Collapse,
		&Local::Death_BossFinal,
	};

	// 行動 enum と実処理関数を対応付けるテーブル
	using MoveFn = void(*)(Enemy*, MoveCtx&);
	static const MoveFn kMoveTable_[] = {
		&Local::Move_StraightStop,
		&Local::Move_SineX,
		&Local::Move_StrafeLtoR,
		&Local::Move_ChasePlayer,
		&Local::Move_PounceFromAbove,
		&Local::Move_FreeRoam,
		&Local::Move_FormationMove,
	};

	//=========================================================
	// 死亡演出更新
	//=========================================================
	if (isDying_) {
		// 死亡演出開始からの経過時間を進める
		deathTimer_ += dt;

		// 進行率を 0.0f ～ 1.0f に収める
		float t_ = std::min(deathTimer_ / deathDuration_, 1.0f);

		// 現在のTransformを元に死亡演出用コンテキストを組み立てる
		DeathCtx c_{};
		c_.dt_ = dt;
		c_.t_ = t_;
		c_.pos_ = object_->GetTranslate();
		c_.rot_ = object_->GetRotate();
		c_.scale_ = baseScale_;

		// enum をテーブル添字に変換する
		const int di_ = static_cast<int>(deathReaction_);

		// 範囲内なら対応する死亡演出関数を呼ぶ
		if (0 <= di_ && di_ < static_cast<int>(std::size(kDeathTable_))) {
			kDeathTable_[di_](this, c_);
		}

		// 演出結果を本体に反映する
		object_->SetTranslate(c_.pos_);
		object_->SetRotate(c_.rot_);
		object_->SetScale(c_.scale_);

		// ボス最終死亡だけは後半からフェードアウトし、それ以外は線形フェードで消す
		if (deathReaction_ == EnemyDeathReaction::BossFinal) {
			if (t_ < 0.7f) {
				// 前半は完全不透明のまま見せる
				deathAlpha_ = 1.0f;
			} else {
				// 後半だけ透明化を開始する
				float u_ = (t_ - 0.7f) / 0.3f;
				if (u_ > 1.0f) u_ = 1.0f;

				// 徐々に消していく
				deathAlpha_ = 1.0f - u_;
			}
		} else {
			// 通常死亡は進行率に応じてそのまま透明化する
			deathAlpha_ = 1.0f - t_;
		}

		// 敵本体のアルファ値に反映する
		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

		// 本体の更新を反映する
		object_->Update();

		// 演出終了時に死亡種別ごとのパーティクルを発生させる
		if (deathTimer_ >= deathDuration_) {
			TKM::ParticleManager* pm_ = TKM::ParticleManager::GetInstance();

			// 発生位置は現在の敵ワールド位置を使う
			Vector3 emitPos_ = GetWorldPosition();

			if (deathReaction_ == EnemyDeathReaction::BlowAway) {
				pm_->Emit("enemyDeath_core", emitPos_, 1);
				pm_->Emit("enemyDeath_shard", emitPos_, 20);
				pm_->Emit("enemyDeath_smoke", emitPos_, 4);
			} else if (deathReaction_ == EnemyDeathReaction::RiseAbsorb) {
				pm_->Emit("enemyDeath_core", emitPos_, 1);
				pm_->Emit("enemyDeath_shard", emitPos_, 14);
				pm_->Emit("enemyDeath_smoke", emitPos_, 6);
			} else if (deathReaction_ == EnemyDeathReaction::Collapse) {
				pm_->Emit("enemyDeath_shard", emitPos_, 10);
				pm_->Emit("enemyDeath_smoke", emitPos_, 3);
			} else if (deathReaction_ == EnemyDeathReaction::BossFinal) {
				// 大爆発をまだ出していない場合だけ一度だけ出す
				if (!bossFinalBigBurstDone_) {
					pm_->Emit("bossClear_core", emitPos_, 1);
					pm_->Emit("bossClear_ring", emitPos_, 3);
					pm_->Emit("bossClear_spark", emitPos_, 80);
					pm_->Emit("bossClear_debris", emitPos_, 60);
				}
			}

			// 死亡完了フラグを立てる
			isDead_ = true;
		}

		// 死亡中でも触手は親に追従させ続ける
		if (tentacle_) {
			// 本体と同じ透明度にする
			tentacle_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

			// ボス以外は触手だけ少し回し続けて見た目を保つ
			if (type_ != EnemyType::Boss) {
				tentacleLocalRot_.y += 0.1f * factor_;
			}

			// ローカルTransformを毎フレ反映する
			tentacle_->SetTranslate(tentacleLocalPos_);
			tentacle_->SetRotate(tentacleLocalRot_);
			tentacle_->SetScale(tentacleLocalScale_);

			// 触手の行列更新を反映する
			tentacle_->Update();
		}

		// 死亡中はこれ以降の通常行動を行わない
		return;
	}

	//=========================================================
	// 怒り状態更新
	//=========================================================
	if (isAngry_) {
		// 怒り状態の経過時間を進める
		angryTimer_ += dt;

		// 持続時間を超えたら怒り状態を解除する
		if (angryTimer_ >= angryDuration_) {
			isAngry_ = false;
		}
	}

	//=========================================================
	// 通常行動更新
	//=========================================================

	// 現在位置を元に行動用コンテキストを作る
	MoveCtx m_{};
	m_.dt_ = dt;
	m_.factor_ = factor_;
	m_.pos_ = object_->GetTranslate();

	// 凍結中でなければ行動パターンに応じた移動を実行する
	if (!freezeMove_) {
		const int bi_ = static_cast<int>(behavior_);

		// enum値が範囲内なら対応する行動関数を実行する
		if (0 <= bi_ && bi_ < static_cast<int>(std::size(kMoveTable_))) {
			kMoveTable_[bi_](this, m_);
		}
	}

	// 行動結果の位置を本体に反映する
	object_->SetTranslate(m_.pos_);

	// 一定以上奥へ抜けた通常敵は、逃走扱いで死亡させる
	if (!isDying_) {
		if (m_.pos_.z < -30.0f) {
			escaped_ = true;
			isDead_ = true;
		}
	}

#ifdef USE_IMGUI
	//=========================================================
	// 当たり判定可視化
	//=========================================================
	{
		// 現在の敵中心位置
		Vector3 center_ = GetWorldPosition();

		// AABBサイズ
		Vector3 size_ = colliderScale_;

		// ライン描画システム取得
		auto* lr_ = TKM::LineRenderer::GetInstance();

		// 通常色
		TKM::LineRenderer::Color normal_{ 0.0f, 1.0f, 0.0f, 1.0f };

		// ヒット色
		TKM::LineRenderer::Color hit_{ 1.0f, 0.0f, 0.0f, 1.0f };

		// レティクルがある場合はレイも使って強調表示する
		if (reticle_) {
			Vector3 rayOrigin_;

			// プレイヤー位置取得関数があればプレイヤー位置をレイ始点にする
			if (playerGetter_) {
				rayOrigin_ = playerGetter_();
			} else {
				// 無ければレティクル中心位置を始点にする
				rayOrigin_ = reticle_->GetCenterWorldPos();
			}

			// レティクルの向きをレイ方向として取得する
			Vector3 rayDir_ = reticle_->GetAimDirection();

			// AABBとレイの関係を可視化する
			lr_->AddAABBWithRayHighlight(center_, size_, rayOrigin_, rayDir_, normal_, hit_);
		} else {
			// レティクルが無い場合はAABBだけ描く
			lr_->AddAABB(center_, size_, normal_);
		}
	}
#endif

	//=========================================================
	// ロック脈動演出
	//=========================================================
	if (isLocked_ && lockPulseEnabled_) {
		// 脈動用時間を進める
		pulseT_ += 0.12f * factor_;

		// サイン波で1.0倍前後に拡縮させる
		float s_ = 1.0f + 0.15f * sinf(pulseT_);

		// 基準スケールに対して拡縮を反映する
		object_->SetScale({ baseScale_.x * s_, baseScale_.y * s_, baseScale_.z * s_ });
	} else {
		// ロックしていない時は基準スケールへ戻す
		object_->SetScale(baseScale_);
	}

	//=========================================================
	// 射撃タイマー更新
	//=========================================================
	if (canShoot_ && !isDying_) {
		// 射撃間隔管理用タイマーを進める
		shootTimer_ += factor_;

		// 間隔に達したらリセットする
		if (shootTimer_ >= shootInterval_) {
			shootTimer_ = 0.0f;
		}
	}

	// 本体のTransform更新を反映する
	object_->Update();

	//=========================================================
	// 触手更新
	//=========================================================

	// 触手も本体と同じアルファ値を使う
	tentacle_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });

	// ボス以外は触手を常時回転させる
	if (type_ != EnemyType::Boss) {
		tentacleLocalRot_.y += 0.1f * factor_;
	}

	// 毎フレ触手のローカルTransformを再反映する
	tentacle_->SetTranslate(tentacleLocalPos_);
	tentacle_->SetRotate(tentacleLocalRot_);
	tentacle_->SetScale(tentacleLocalScale_);

	// 触手の行列更新を反映する
	tentacle_->Update();
}

void Enemy::Draw(TKM::DirectXCommon* dxCommon) {
	// 本体が無ければ描画できないので終了する
	if (!object_) return;

	// 敵本体を描画する
	object_->Draw(dxCommon);

	// 触手が無ければ触手描画は行わない
	if (!tentacle_) return;

	// 触手を描画する
	tentacle_->Draw(dxCommon);
}

void Enemy::SetHP(int hp) {
	// 現在HPと最大HPを同じ値で初期化する
	hp_ = hp;
	maxHP_ = hp;
}

void Enemy::SetModel(const std::string& modelName) {
	// 本体が存在する時だけモデルを差し替える
	if (object_) object_->SetModel(modelName);
}

void Enemy::SetScale(const Vector3& scale) {
	// ロック脈動や死亡演出の基準になるスケールを更新する
	baseScale_ = scale;

	// 本体があるなら現在スケールにも即反映する
	if (object_) object_->SetScale(scale);
}

void Enemy::SetRotate(const Vector3& rot) {
	// 本体が無ければ回転を設定できないので終了する
	if (!object_) { return; }

	// 本体の回転を設定する
	object_->SetRotate(rot);
}

void Enemy::SetCamera(TKM::Camera* camera) {
	// カメラ参照を保持する
	camera_ = camera;

	// 本体があれば本体に反映する
	if (object_) { object_->SetCamera(camera); }

	// 触手があれば触手にも反映する
	if (tentacle_) { tentacle_->SetCamera(camera); }
}

void Enemy::SetPosition(const Vector3& pos) {
	// 本体がある場合だけ位置を反映する
	if (object_) { object_->SetTranslate(pos); }
}

void Enemy::SetParentScene(TKM::BaseScene* scene) {
	// 親シーンを保持しておく
	parentScene_ = scene;

	// 本体があれば親シーンを設定する
	if (object_) { object_->SetParentScene(scene); }

	// 触手があれば触手にも親シーンを設定する
	if (tentacle_) { tentacle_->SetParentScene(scene); }
}

void Enemy::SetLocked(bool v) {
	// ロック状態を更新する
	isLocked_ = v;

	// ロック解除時は脈動位相をリセットして見た目を戻しやすくする
	if (!v) pulseT_ = 0.0f;
}

void Enemy::SetColliderScale(const Vector3& s) {
	// 当たり判定可視化や被弾判定に使うサイズを保存する
	colliderScale_ = s;
}

void Enemy::SetBehavior(EnemyBehavior b) {
	// 行動パターンを設定する
	behavior_ = b;
}

void Enemy::SetVelocity(const Vector3& v) {
	// 移動速度を設定する
	velocity_ = v;
}

void Enemy::SetStopZ(float z) {
	// 停止ラインとなるZ座標を設定する
	stopZ_ = z;
}

void Enemy::SetSineParams(float ampX, float freq) {
	// サイン移動の振れ幅と周波数を設定する
	sineAmpX_ = ampX;
	sineFreq_ = freq;
}

void Enemy::SetStrafeX(float left, float right, float speed) {
	// 左右移動の範囲を設定する
	strafeLeft_ = left;
	strafeRight_ = right;

	// 左右移動の速度を設定する
	strafeSpeed_ = speed;

	// 初期位置が未設定に近い場合は左端から開始させる
	if (strafePosX_ == 0.0f) strafePosX_ = left;
}

void Enemy::SetCanShoot(bool v, float interval) {
	// 射撃可否を更新する
	canShoot_ = v;

	// 射撃間隔を設定する
	shootInterval_ = interval;
}

void Enemy::SetPlayer(std::function<Vector3()> getter) {
	// プレイヤー位置取得関数を保持する
	playerGetter_ = std::move(getter);
}

void Enemy::SetSinePhase(float rad) {
	// サイン移動の開始位相を設定する
	sinePhase_ = rad;
}

void Enemy::SetReticle(Reticle* r) {
	// レティクル参照を保持する
	reticle_ = r;
}

void Enemy::SetPounceParameters(const Vector3& start, const Vector3& apex, const Vector3& target, float duration) {
	// 接近開始位置を設定する
	pounceStart_ = start;

	// 中間の山頂位置を設定する
	pounceApex_ = apex;

	// 最終的な着地点を設定する
	pounceTarget_ = target;

	// 接近フェーズ全体にかける時間を設定する
	pounceDuration_ = duration;

	// 経過時間をリセットする
	pounceTime_ = 0.0f;

	// 急降下開始済みフラグを立てる
	pounceStarted_ = true;

	// まだ本格ダイブには入っていない状態に戻す
	pounceDiving_ = false;
}

void Enemy::SetType(EnemyType t) {
	// 敵タイプを更新する
	type_ = t;

	// ボスはロック脈動を無効にする
	lockPulseEnabled_ = (type_ != EnemyType::Boss);

	// 無効化された場合は脈動時間もリセットする
	if (!lockPulseEnabled_) { pulseT_ = 0.0f; }
}

void Enemy::SetFreeRoamArea(const Vector3& min, const Vector3& max, float normalSpeed, float angrySpeed) {

}

void Enemy::SetAngry(float duration) {
	// 怒り状態を有効化する
	isAngry_ = true;

	// 持続時間を保存する
	angryDuration_ = duration;

	// 経過タイマーをリセットする
	angryTimer_ = 0.0f;
}

void Enemy::SetFreezeMove(bool v) {
	// 移動凍結状態を更新する
	freezeMove_ = v;
}

void Enemy::SetCurrentHP(int hp) {
	// 0未満にはしない
	if (hp < 0) { hp = 0; }

	// 最大HPを超えないようにする
	if (hp > maxHP_) { hp = maxHP_; }

	// 補正後の値を現在HPとして保存する
	hp_ = hp;
}

void Enemy::SetTentacleModel(const std::string& modelName) {
	// 触手が存在する時だけモデルを差し替える
	if (tentacle_) tentacle_->SetModel(modelName);
}

void Enemy::SetTentacleLocal(const Vector3& pos, const Vector3& rot, const Vector3& scale) {
	// 触手のローカル位置を保存する
	tentacleLocalPos_ = pos;

	// 触手のローカル回転を保存する
	tentacleLocalRot_ = rot;

	// 触手のローカルスケールを保存する
	tentacleLocalScale_ = scale;
}

void Enemy::SetRoamArea(const Vector3& min, const Vector3& max) {
	// 徘徊範囲の最小値を設定する
	roamMin_ = min;

	// 徘徊範囲の最大値を設定する
	roamMax_ = max;

	// 次回更新時に新しい目標を作り直すためフラグを落とす
	hasRoamTarget_ = false;
}

void Enemy::SetRoamSpeed(float normal, float angry) {
	// 通常時の徘徊速度を設定する
	roamSpeedNormal_ = normal;

	// 怒り時の徘徊速度を設定する
	roamSpeedAngry_ = angry;
}

void Enemy::SetFormationTarget(const Vector3& target) {
	// 隊列の目標位置を保存する
	formationTarget_ = target;

	// 新しい目標へ向かい直すため、到達フラグを落とす
	isInFormation_ = false;
}

void Enemy::SetFormationMoveSpeed(float speed) {
	// あまりに小さい値が入った時に停止同然にならないよう下限を設ける
	if (speed < 0.001f) {
		speed = 0.001f;
	}

	// 補正後の速度を保存する
	formationMoveSpeed_ = speed;
}

Vector3 Enemy::GetWorldPosition() const {
	// 本体が無い場合は安全のため原点を返す
	if (!object_) {
		return { 0.0f, 0.0f, 0.0f };
	}

	// 本体の現在位置をワールド位置として返す
	return object_->GetTranslate();
}

void Enemy::ImGuiDebug() {
#ifdef USE_IMGUI
	// 本体が無ければ編集対象が無いので終了する
	if (!object_) return;

	ImGui::Begin("Enemy");

	// 現在のTransformを取得する
	Vector3 pos_ = object_->GetTranslate();
	Vector3 rot_ = object_->GetRotate();
	Vector3 scale_ = object_->GetScale();

	// 位置をドラッグ編集できるようにする
	if (ImGui::DragFloat3("位置", &pos_.x, 0.01f)) {
		object_->SetTranslate(pos_);
	}

	// 回転をドラッグ編集できるようにする
	if (ImGui::DragFloat3("回転", &rot_.x, 0.01f)) {
		object_->SetRotate(rot_);
	}

	// スケールをドラッグ編集し、基準スケールにも反映させる
	if (ImGui::DragFloat3("拡縮", &scale_.x, 0.01f)) {
		SetScale(scale_);
	}

	// 当たり判定サイズもドラッグ編集できるようにする
	Vector3 col_ = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ", &col_.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col_);
	}

	// HP表示
	ImGui::Text("HP: %d / %d", hp_, maxHP_);

	// 生死状態表示
	ImGui::Text("生死: %s", isDead_ ? "死" : "生");

	ImGui::End();
#endif
}

void Enemy::OnHitWithDamage(int damage) {
	// すでに死んでいる、死亡演出中、無敵中なら被弾処理を受け付けない
	if (isDead_ || isDying_ || damageInvincible_) {
		return;
	}

	// ダメージ分だけHPを減らす
	hp_ -= damage;

	// HPが0以下になったら死亡演出へ入る
	if (hp_ <= 0) {
		hp_ = 0;

		// ボスだけは専用死亡演出を使う
		if (type_ == EnemyType::Boss) {
			StartBossDeathReaction({ 0.0f, 0.0f, 1.0f });
		} else {
			StartDeathReaction({ 0.0f, 0.0f, 1.0f });
		}
	}
}

void Enemy::StartDeathReaction(const Vector3& hitDir) {
	// すでに死亡演出中、または無敵中なら開始しない
	if (isDying_ || damageInvincible_) {
		return;
	}

	// ボスは通常死亡演出ではなく専用演出へ流す
	if (type_ == EnemyType::Boss) {
		StartBossDeathReaction(hitDir);
		return;
	}

	// 移動を止める
	freezeMove_ = true;

	// 撃破済みフラグを立てる
	defeated_ = true;

	// 死亡演出中フラグを立てる
	isDying_ = true;

	// 演出タイマーをリセットする
	deathTimer_ = 0.0f;

	// アルファ値を初期化する
	deathAlpha_ = 1.0f;

	// 3種類の通常死亡演出からランダムで選ぶ
	int r_ = std::rand() % 3;

	// 被弾方向を基準ノックバック方向として使う
	Vector3 dir_ = hitDir;

	// 方向がほぼゼロなら、正面方向を仮方向として使う
	if (MyMath::Length(dir_) < 0.001f) {
		dir_ = { 0.0f, 0.0f, 1.0f };
	}

	// 正規化して方向だけにする
	dir_ = MyMath::Normalize(dir_);

	// 吹き飛び系死亡を選んだ場合の設定
	auto Pick0_BlowAway_ = [&]() {
		deathReaction_ = EnemyDeathReaction::BlowAway;
		deathDuration_ = 3.0f;
		deathVelocity_ = dir_ * 4.0f;
		deathRotateSpeed_ = { 1.5f, 2.0f, 0.8f };
		};

	// 浮き上がり吸収系死亡を選んだ場合の設定
	auto Pick1_RiseAbsorb_ = [&]() {
		deathReaction_ = EnemyDeathReaction::RiseAbsorb;
		deathDuration_ = 1.2f;
		deathVelocity_ = { 0.0f, 3.0f, 0.0f };
		deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f };
		};

	// 崩れ落ち系死亡を選んだ場合の設定
	auto Pick2_Collapse_ = [&]() {
		deathReaction_ = EnemyDeathReaction::Collapse;
		deathDuration_ = 0.9f;
		deathVelocity_ = { dir_.x * 1.5f, -3.0f, dir_.z * 1.5f };
		deathRotateSpeed_ = { 3.0f, 0.5f, 0.0f };
		};

	// ランダム値に応じて1つだけ採用する
	if (r_ == 0) {
		Pick0_BlowAway_();
	} else if (r_ == 1) {
		Pick1_RiseAbsorb_();
	} else {
		Pick2_Collapse_();
	}
}

void Enemy::SyncTransform() {
	// 本体が無ければ同期できない
	if (!object_) return;

	// 触手が無ければ同期できない
	if (!tentacle_) return;

	// 本体の行列や定数バッファを更新する
	object_->Update();

	// 触手の行列や定数バッファも更新する
	tentacle_->Update();
}

void Enemy::StartBossDeathReaction(const Vector3& hitDir) {
	// すでに死亡演出中なら何もしない
	if (isDying_) {
		return;
	}

	// 無敵中も死亡開始させない
	if (damageInvincible_) {
		return;
	}

	// 移動を止める
	freezeMove_ = true;

	// 撃破済みフラグを立てる
	defeated_ = true;

	// 死亡演出中フラグを立てる
	isDying_ = true;

	// 演出タイマーをリセットする
	deathTimer_ = 0.0f;

	// アルファを初期化する
	deathAlpha_ = 1.0f;

	// ボス専用の最終死亡演出を使う
	deathReaction_ = EnemyDeathReaction::BossFinal;

	// ボス演出は見せ場なので長めに取る
	deathDuration_ = 5.0f;

	// BossFinal 側で位置・回転を制御するのでここではゼロ初期化する
	deathVelocity_ = { 0.0f, 0.0f, 0.0f };
	deathRotateSpeed_ = { 0.0f, 0.0f, 0.0f };

	// 打ち上げ開始位置の初期値を現在位置で保存する
	bossFinalLaunchStartPos_ = object_->GetTranslate();

	// 後半演出の各種フラグを初期化する
	bossFinalLaunchStarted_ = false;
	bossFinalBigBurstDone_ = false;
	bossFinalCameraInited_ = false;

	(void)hitDir;
}