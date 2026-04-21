#define NOMINMAX
#include "HomingBullet.h"
#include <algorithm>
#include <ParticleManager.h>
#include "AABB.h"
#include "Player.h"
#include "Enemy.h"
#include "BarrierCore.h"
#include "TrailRibbonRenderer.h"

namespace {
	// ホーミング弾の寿命（秒）
	static Vector3 DirToEuler_(const Vector3& dir) {
		Vector3 d = dir; // 引数はconstなのでコピーしてから操作
		float len = MyMath::Length(d); // 長さを求める

		// 長さが極端に小さい場合は向きを決定できないので、デフォルトの向きを返す
		if (len < 0.0001f) { return { 0,0,0 }; }
		d = d / len; // 正規化
		
		float yaw = std::atan2(d.x, d.z); // Y軸回りの回転（ヨー）
		float pitch = -std::asin(d.y); // X軸回りの回転（ピッチ）。符号を反転しているのは、一般的な3D空間での上方向がY軸正方向であるため

		// Z軸回りの回転（ロール）は、ホーミング弾の場合は通常必要ないため0とする
		return { pitch, yaw, 0.0f };
	}
}

void HomingBullet::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	// ホーミング弾のモデルオブジェクトを生成して初期化
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj"); // 仮のモデル。必要に応じて差し替える
	object_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ }); // スケールを小さくする

	prevPos_ = object_->GetTranslate(); // 初期位置を保存

	lifeTimer_ = 0.0f; // 寿命タイマー
	trailPts_.clear(); // トレイルの点をクリア
	trailDistAcc_ = 0.0f; // トレイルの距離蓄積をリセット
}

void HomingBullet::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置を更新
	prevPos_ = pos; // トレイル更新のために前回位置も更新
	
	// 位置が大きく変わった場合はトレイルをリセットして、急な移動に伴う不自然なトレイルを防止
	trailPts_.clear();
	// 新しい位置をトレイルの最初の点として追加
	trailPts_.push_back(pos);

	trailDistAcc_ = 0.0f; // トレイルの距離蓄積をリセット
	isTrailFading_ = false; // トレイルフェードをリセット
	trailFadeTimer_ = 0.0f; // トレイルフェードタイマーをリセット
}

void HomingBullet::SetCamera(TKM::Camera* camera) {
	camera_ = camera;
	if (object_) {
		object_->SetCamera(camera); // ホーミング弾のオブジェクトにもカメラを設定
	}
}

void HomingBullet::SetEnemy(Enemy* enemy) {
	enemy_ = enemy;
}

void HomingBullet::SetPlayer(Player* player) {
	player_ = player;
}

void HomingBullet::SetCore(BarrierCore* core) {
	core_ = core;
}

void HomingBullet::StartArc(
	const Vector3& start,
	const Vector3& control1,
	const Vector3& control2,
	const Vector3& end,
	float duration
) {
	p0_ = start;
	p1_ = control1;
	p2_ = control2;
	p3_ = end;

	arcT_ = 0.0f; // 弾道の進行度をリセット
	arcDuration_ = std::max(0.001f, duration); // 継続時間は0に近い値を許容しない（0だと割り算で問題が起きるため）
	isArcActive_ = true; // 弾道をアクティブにする
	// 弾道開始位置にホーミング弾を配置
	SetPosition(start);
}

void HomingBullet::UpdateTrail_(const Vector3& p) {
	// トレイル点がない場合は、現在位置を最初の点として追加して終了
	if (trailPts_.empty()) {
		trailPts_.push_back(p); // 現在位置をトレイルの最初の点として追加
		trailDistAcc_ = 0.0f; // トレイル距離蓄積をリセット
		return;
	}

	float moveDist = MyMath::Length(p - prevPos_); // 前回位置からの移動距離を計算
	trailDistAcc_ += moveDist; // 移動距離を蓄積

	// 一定距離以上移動したらトレイル点を追加
	if (trailDistAcc_ >= kTrailStep_) {
		trailPts_.push_back(p); // 現在位置をトレイルの新しい点として追加
		trailDistAcc_ = 0.0f; // トレイル距離蓄積をリセット

		// トレイル点が多すぎる場合は古い点から削除していく（ハードキャップ）
		while (trailPts_.size() > kTrailHardCap_) {
			trailPts_.erase(trailPts_.begin()); // player側(古い点)から消す
		}
	}
}

void HomingBullet::Update() {
	if (!object_ || isDead_) { return; } // オブジェクトがないか、すでに死亡している場合は更新しない

	// トレイルの更新
	if (isTrailFading_) {
		trailFadeTimer_ += dt_; // フェードタイマーを進める

		// フェードタイマーが一定時間を超えたら、トレイル点を削除していく
		while (trailFadeTimer_ >= trailFadeInterval_) {
			trailFadeTimer_ -= trailFadeInterval_; // インターバル分だけタイマーを減らす

			// トレイル点が残っている場合は、古い点から削除していく
			if (!trailPts_.empty()) {
				trailPts_.erase(trailPts_.begin()); // player側(古い点)から消す
			}
		}

		// トレイル点が2つ未満になったら完全に消えたとみなしてホーミング弾も死亡させる
		if (trailPts_.size() < 2) {
			isDead_ = true; // ホーミング弾を死亡させる
		}
		return;
	}

	Vector3 oldPos = object_->GetTranslate(); // 現在の位置を取得
	prevPos_ = oldPos; // 前回位置を更新（トレイル更新のため）
	lifeTimer_ += dt_; // 寿命タイマーを進める

	// 寿命が尽きたら死亡フラグを立てて更新処理を終了
	if (lifeTimer_ >= kLifeTime_) {
		isDead_ = true; // 死亡フラグを立てる
		return;
	}

	// 山なり弾道の更新
	if (isArcActive_) {
		arcT_ += dt_ / arcDuration_; // 弾道の進行度を更新（0から1へ）
		float t = std::clamp(arcT_, 0.0f, 1.0f); // クランプして0～1の範囲に収める
		// ベジェ曲線上の位置を計算
		Vector3 pos = MyMath::Bezier3(p0_, p1_, p2_, p3_, t);

		// 終盤だけ現在のターゲット位置へ少しずつ寄せる
		if (t >= 0.65f) {
			bool hasTarget = false;
			Vector3 targetPos = pos;

			if (core_ && !core_->IsDead()) {
				targetPos = core_->GetWorldPosition();
				hasTarget = true;
			} else if (enemy_ && !enemy_->IsDead()) {
				targetPos = enemy_->GetWorldPosition();
				targetPos.y += 1.5f; // 敵は少し上を狙う
				hasTarget = true;
			}

			if (hasTarget) {
				float followT = (t - 0.65f) / (1.0f - 0.65f);
				followT = std::clamp(followT, 0.0f, 1.0f);
				pos = pos + (targetPos - pos) * followT;
			}
		}
		object_->SetTranslate(pos); // ホーミング弾の位置を更新

		// ============================
		// LB弾：トレイル全体にキラキラ + 稲光
		// ============================
		{
			auto* pm = TKM::ParticleManager::GetInstance();

			// トレイル点が2つ以上ある場合にキラキラと稲光を発生させる
			if (trailPts_.size() >= 2) {

				// 0.0～1.0の範囲でランダムな浮動小数点数を生成するラムダ関数
				auto frand = [](float a, float b) {
					return a + (b - a) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)); // 0.0～1.0の範囲でランダムな浮動小数点数を生成して、a～bの範囲にスケーリングして返す
					};

				// トレイル履歴を間引きながら走査
				const int step = 2; // 大きいほど軽い

				// トレイル点を走査して、キラキラと稲光を発生させる位置を決定
				for (size_t i = 0; i < trailPts_.size(); i += step) {
					Vector3 centerPos = trailPts_[i]; // トレイル点の位置
					Vector3 dir = { 0,0,1 }; // トレイルの向き（初期値は適当な値）

					// トレイル点の向きを、前後の点から計算して決定する
					if (i + 1 < trailPts_.size()) {
						dir = trailPts_[i + 1] - trailPts_[i]; // 次の点との差分から向きを計算
						float len = MyMath::Length(dir); // 長さを求める

						// 長さが極端に小さい場合は向きを決定できないので、デフォルトの向きを使用する
						if (len > 0.0001f) dir = dir / len;
					}

					// ----------------------------
					// トレイル横方向
					// ----------------------------
					Vector3 side = { 1,0,0 };

					// カメラがある場合は、カメラの位置からトレイル点へのベクトルとトレイルの向きの外積を取ることで、トレイルの横方向を計算する
					if (camera_) {
						Vector3 camVec = camera_->GetTranslate() - centerPos; // カメラの位置からトレイル点へのベクトル
						float camLen = MyMath::Length(camVec); // 長さを求める

						// 長さが極端に小さい場合は横方向を決定できないので、デフォルトの横方向を使用する
						if (camLen > 0.0001f) {
							camVec /= camLen; // 正規化
							Vector3 s = MyMath::Cross(camVec, dir); // トレイルの向きとカメラベクトルの外積を取ることで、トレイルの横方向を計算
							float sLen = MyMath::Length(s); // 長さを求める

							// 長さが極端に小さい場合は横方向を決定できないので、デフォルトの横方向を使用する
							if (sLen > 0.0001f) side = s / sLen;
						}
					}

					float sign = (rand() % 2 == 0) ? -1.0f : 1.0f; // ランダムに左右どちらかの方向にオフセットするための符号
					float edgeBase = 0.72f; // トレイルの中心からキラキラや稲光を発生させる位置の基本オフセット距離
					float overhang = frand(0.05f, 0.30f); // トレイルの中心からキラキラや稲光を発生させる位置のオーバーハング距離（ランダムに少し前方に出すことで、トレイルの中心に密着しすぎないようにする）
					Vector3 edgePos = centerPos + side * sign * (edgeBase + overhang); // トレイルの中心から横方向にオフセットした位置を計算

					// ----------------------------
					// キラキラ
					// ----------------------------

					// ランダムに80%の確率でキラキラを発生させる
					if ((rand() % 100) < 80) {
						pm->Emit("trail_lb_glitter", edgePos, 2);

						// さらにランダムに55%の確率でキラキラをもう1回発生させる（1回だけだと寂しいので、2回重ねることで少し賑やかにする）
						if ((rand() % 100) < 55) {
							pm->Emit("trail_lb_glitter", edgePos, 2);
						}
					}

					// ----------------------------
					// 稲光
					// ----------------------------

					// ランダムに45%の確率で稲光を発生させる
					if ((rand() % 100) < 45) {
						Vector3 boltPos = centerPos + side * sign * frand(0.65f, 1.15f); // 稲光の位置は、トレイルの中心から横方向にランダムな距離だけオフセットした位置を計算（キラキラよりも少し広い範囲で出す）

						pm->Emit("trail_lb_bolt_main", boltPos, 2);
						pm->Emit("trail_lb_bolt_core", boltPos, 1);

						// さらにランダムに40%の確率で稲光をもう1回発生させる（1回だけだと寂しいので、2回重ねることで少し賑やかにする）
						if ((rand() % 100) < 40) {
							pm->Emit("trail_lb_bolt_core", boltPos, 1);
						}
					}
				}
			}
		}

		// 向きも弾道に沿わせる
		float t2 = std::min(1.0f, t + 0.01f);
		Vector3 nextPos = MyMath::Bezier3(p0_, p1_, p2_, p3_, t2);

		// 終盤だけ現在のターゲット位置へ少しずつ寄せる
		if (t2 >= 0.65f) {
			bool hasTarget2 = false;
			Vector3 targetPos2 = nextPos;

			if (core_ && !core_->IsDead()) {
				targetPos2 = core_->GetWorldPosition();
				hasTarget2 = true;
			} else if (enemy_ && !enemy_->IsDead()) {
				targetPos2 = enemy_->GetWorldPosition();
				targetPos2.y += 1.5f; // 敵は少し上を狙う
				hasTarget2 = true;
			}

			if (hasTarget2) {
				float followT2 = (t2 - 0.65f) / (1.0f - 0.65f);
				followT2 = std::clamp(followT2, 0.0f, 1.0f);
				nextPos = nextPos + (targetPos2 - nextPos) * followT2;
			}
		}
		
		Vector3 dir = nextPos - pos; // 次の位置と現在位置の差分から向きを計算

		// 向きの長さが極端に小さい場合は向きを決定できないので、回転を更新しない
		if (MyMath::Length(dir) > 0.0001f) {
			object_->SetRotate(DirToEuler_(dir)); // 向きをオイラー角に変換してホーミング弾の回転を更新
		}

		// トレイルの更新
		UpdateTrail_(pos);

		// =========================================
		// Wave1バリアとの当たり判定
		// =========================================
		if (player_ && player_->IsWave1BarrierActive()) {

			const Vector3 bulletPos_ = object_->GetTranslate();
			const Vector3 bulletScale_ = object_->GetScale();

			AABB bulletBox_(bulletPos_, bulletScale_);
			AABB barrierBox_(player_->GetWave1BarrierCenter(), player_->GetWave1BarrierSize());

			bool barrierHit_ = false;

			if (barrierBox_.IsIntersectSegment(prevPos_, bulletPos_)) {
				barrierHit_ = true;
			}
			if (!barrierHit_ && bulletBox_.IsCollidingWithAABB(barrierBox_)) {
				barrierHit_ = true;
			}

			if (barrierHit_) {
				isHit_ = true;
				isDead_ = true;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				if (pm) {
					pm->Emit("enemyHit_flash", bulletPos_, 1);
					pm->Emit("enemyHit_ring", bulletPos_, 1);
					pm->Emit("enemyHit_spark", bulletPos_, 12);
				}

				if (player_) {
					player_->AddWave1BarrierHit(bulletPos_);
					player_->RequestWave1BarrierFlash(bulletPos_);
					player_->StartCameraShake(6);
				}
				return;
			}
		}

		Vector3 bulletPos = object_->GetTranslate(); // ホーミング弾の現在位置
		Vector3 bulletScale = object_->GetScale(); // ホーミング弾のスケール（当たり判定の大きさに使用）

		// ============================
		auto CheckSweptHitAABB = [&](const Vector3& targetPos, const Vector3& targetSize) -> bool {
			AABB bulletBox(bulletPos, bulletScale); // ホーミング弾の当たり判定用AABBを作成
			AABB targetBox(targetPos, targetSize); // ターゲットの位置とサイズから当たり判定用AABBを作成

			// ホーミング弾の前回位置から現在位置への線分とターゲットのAABBが交差しているかをチェック
			if (targetBox.IsIntersectSegment(prevPos_, bulletPos)) {
				return true; // 線分とAABBが交差している場合はヒットとみなす
			}

			// ホーミング弾の現在位置のAABBとターゲットのAABBが重なっているかをチェック
			if (bulletBox.IsCollidingWithAABB(targetBox)) {
				return true; // AABB同士が重なっている場合もヒットとみなす
			}
			return false;
			};

		// 敵と当たり判定をチェック
		if (enemy_ && !enemy_->IsDead()) {
			Vector3 enemyPos = enemy_->GetWorldPosition(); // 敵の現在位置
			Vector3 enemySize = enemy_->GetColliderScale(); // 敵の当たり判定用サイズ（コライダーのスケールを使用）

			// ホーミング弾の前回位置から現在位置への線分と敵のAABBが交差しているか、またはホーミング弾の現在位置のAABBと敵のAABBが重なっているかをチェック
			if (CheckSweptHitAABB(enemyPos, enemySize)) {
				// ヒットした場合の処理
				isHit_ = true;
				isTrailFading_ = true;
				trailFadeTimer_ = 0.0f;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				Vector3 hitPos = bulletPos; // ヒット位置はホーミング弾の現在位置を使用
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);

				enemy_->OnHitWithDamage(kEnemyDamage_); // 敵にダメージを与える処理を呼び出す

				// カメラシェイクを開始する
				if (player_) {
					player_->StartCameraShake(40); // 40フレームのカメラシェイクを開始
				}
				return;
			}
		}

		// コアと当たり判定をチェック
		if (core_ && !core_->IsDead()) {
			Vector3 corePos = core_->GetWorldPosition(); // コアの現在位置
			Vector3 coreSize = core_->GetColliderScale(); // コアの当たり判定用サイズ（コライダーのスケールを使用）

			// ホーミング弾の前回位置から現在位置への線分とコアのAABBが交差しているか、またはホーミング弾の現在位置のAABBとコアのAABBが重なっているかをチェック
			if (CheckSweptHitAABB(corePos, coreSize)) {
				// ヒットした場合の処理
				isHit_ = true;
				isTrailFading_ = true;
				trailFadeTimer_ = 0.0f;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				Vector3 hitPos = bulletPos; // ヒット位置はホーミング弾の現在位置を使用
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);

				core_->OnHitWithDamage(kCoreDamage_); // コアにダメージを与える処理を呼び出す

				// カメラシェイクを開始する
				if (player_) {
					player_->StartCameraShake(40); // 40フレームのカメラシェイクを開始
				}
				return;
			}
		}

		// 弾道が終盤に差し掛かっている場合は、ホーミング弾を敵の位置に寄せる処理を開始する
		if (t >= 1.0f) {
			isArcActive_ = false; // 弾道の更新を終了して、以降は敵の位置に寄せる処理に切り替える
			isTrailFading_ = true; // トレイルフェードを開始する
			trailFadeTimer_ = 0.0f; // トレイルフェードタイマーをリセット
		}
	}

	object_->Update(); // ホーミング弾のオブジェクトの更新処理を呼び出す
}

void HomingBullet::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_ || isDead_ || isTrailFading_) { return; }
	object_->Draw(dxCommon);
}

void HomingBullet::DrawTrail(TKM::DirectXCommon* dxCommon) {
	if (!camera_ || trailPts_.empty() || isDead_) { return; }

	auto* rr = TKM::TrailRibbonRenderer::GetInstance();
	const auto& p = rr->GetDebugParams(); // デバッグパラメータを取得
	if (!p.enable) { return; }

	std::vector<Vector3> drawPts = trailPts_; // 描画用のトレイル点のリストを作成（必要に応じて間引きや補間を行うためにコピーする）
	Vector3 currentPos = object_->GetTranslate(); // ホーミング弾の現在位置を取得

	// トレイルの最後の点と現在位置が近すぎる場合は、現在位置をトレイル点に追加しない（トレイルが過密になって重くなるのを防止）
	if (drawPts.empty() || MyMath::Length(currentPos - drawPts.back()) > 0.0001f) {
		drawPts.push_back(currentPos); // 現在位置を描画用のトレイル点のリストに追加
	}

	// 描画用のトレイル点が2つ未満の場合は、リボンを描画できないので処理を終了
	if (drawPts.size() < 2) { return; }

	rr->DrawRibbon( // リボンを描画
		dxCommon,
		*camera_,
		drawPts,
		p.headWidth,
		p.tailWidth,
		p.intensity,
		p.color,
		p.uvTiling,
		p.uvScroll
	);
}