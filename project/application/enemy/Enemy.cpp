#define NOMINMAX
#include "Enemy.h"
#include "ModelManager.h"
#include <algorithm>
#include <cstdlib> 
#include <AABB.h>

void Enemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>(); // Object3d のインスタンスを生成
	object_->Initialize(common, dxCommon); // 初期化
	object_->SetModel("enemy.obj"); // モデル名は適宜変更

	// カメラ設定
	if (camera) {
		object_->SetCamera(camera);
	}

	baseScale_ = object_->GetScale(); // 元のスケールを保持
	startX_ = object_->GetTranslate().x; // サイン波の基準用
}

void Enemy::Update(float dt) {

	// 60fps基準の値をそのまま使えるようにする係数
	// dt=1/60 のとき factor=1.0 になる
	const float factor = dt * 60.0f;

	// 死亡演出中ならこっちを優先
	if (isDying_) {
		deathTimer_ += dt;
		float t = std::min(deathTimer_ / deathDuration_, 1.0f);

		Vector3 pos = object_->GetTranslate();
		Vector3 rot = object_->GetRotate();
		Vector3 scale = baseScale_;

		switch (deathReaction_) {
		case EnemyDeathReaction::BlowAway: {
			float speed = 1.0f - t;
			pos += deathVelocity_ * speed * dt;

			rot.x += deathRotateSpeed_.x * dt;
			rot.y += deathRotateSpeed_.y * dt;
			rot.z += deathRotateSpeed_.z * dt;

			float s = 1.0f - t;
			scale = { baseScale_.x * s, baseScale_.y * s, baseScale_.z * s };
			break;
		}
		case EnemyDeathReaction::RiseAbsorb: {
			pos += deathVelocity_ * dt;

			rot.y += deathRotateSpeed_.y * dt;

			float s = 1.0f - t;
			scale = {
				baseScale_.x * s * 0.5f,
				baseScale_.y * (1.0f - t * 0.2f),
				baseScale_.z * s * 0.5f
			};
			break;
		}
		case EnemyDeathReaction::Collapse: {
			pos += deathVelocity_ * dt;

			rot.x += deathRotateSpeed_.x * dt;

			float s = 1.0f - t;
			scale = {
				baseScale_.x,
				baseScale_.y * s * 0.2f,
				baseScale_.z
			};
			break;
		}
		case EnemyDeathReaction::BossFinal: {
			const float launchStartT = 0.5f;

			if (t < launchStartT) {
				float shakeAmp = 0.25f;
				float shakeFreq = 18.0f;

				pos.x += sinf(deathTimer_ * shakeFreq) * shakeAmp;
				pos.y += cosf(deathTimer_ * shakeFreq * 0.7f) * shakeAmp * 0.6f;

				float pulse = 1.0f + 0.10f * sinf(deathTimer_ * 10.0f);
				scale = {
					baseScale_.x * pulse,
					baseScale_.y * pulse,
					baseScale_.z * pulse,
				};

				ParticleManager* pm = ParticleManager::GetInstance();
				if (std::rand() % 3 != 0) {
					Vector3 center = GetWorldPosition();
					Vector3 off = {
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * colliderScale_.x,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * colliderScale_.y,
						(static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * colliderScale_.z
					};
					Vector3 emitPos = center + off * 0.5f;
					pm->Emit("bossDeath_bomb", emitPos, 1);
				}
			} else {
				if (!bossFinalLaunchStarted_) {
					bossFinalLaunchStarted_ = true;
					bossFinalLaunchStartPos_ = pos;

					ParticleManager* pm = ParticleManager::GetInstance();
					Vector3 center = GetWorldPosition();
					pm->Emit("bossDeath_ring", center, 2);
					pm->Emit("bossDeath_bomb", center, 10);
					pm->Emit("bossDeath_smoke", center, 24);
				}

				float u = (t - launchStartT) / (1.0f - launchStartT);
				if (u < 0.0f) u = 0.0f;
				if (u > 1.0f) u = 1.0f;

				float k = u * u * u;

				Vector3 upDir = { 0.0f, 1.0f, 0.0f };
				Vector3 forwardDir = { 0.0f, 0.0f, 1.0f };

				float upDist = 15.0f;
				float depthDist = 40.0f;

				pos = bossFinalLaunchStartPos_
					+ upDir * (upDist * k)
					+ forwardDir * (depthDist * k);

				rot.x += 2.5f * dt;
				rot.y += 3.0f * dt;
				rot.z += 1.5f * dt;

				float s = 1.0f - 0.3f * k;
				if (s < 0.1f) s = 0.1f;
				scale = {
					baseScale_.x * s,
					baseScale_.y * s,
					baseScale_.z * s,
				};
			}
			break;
		}
		}

		object_->SetTranslate(pos);
		object_->SetRotate(rot);
		object_->SetScale(scale);

		if (deathReaction_ == EnemyDeathReaction::BossFinal) {
			if (t < 0.7f) {
				deathAlpha_ = 1.0f;
			} else {
				float u = (t - 0.7f) / 0.3f;
				if (u > 1.0f) u = 1.0f;
				deathAlpha_ = 1.0f - u;
			}
		} else {
			deathAlpha_ = 1.0f - t;
		}

		object_->SetColor({ 1.0f, 1.0f, 1.0f, deathAlpha_ });
		object_->Update();

		if (deathTimer_ >= deathDuration_) {
			ParticleManager* pm = ParticleManager::GetInstance();
			Vector3 emitPos = GetWorldPosition();

			switch (deathReaction_) {
			case EnemyDeathReaction::BlowAway:
				pm->Emit("enemyDeath_core", emitPos, 1);
				pm->Emit("enemyDeath_shard", emitPos, 20);
				pm->Emit("enemyDeath_smoke", emitPos, 4);
				break;
			case EnemyDeathReaction::RiseAbsorb:
				pm->Emit("enemyDeath_core", emitPos, 1);
				pm->Emit("enemyDeath_shard", emitPos, 14);
				pm->Emit("enemyDeath_smoke", emitPos, 6);
				break;
			case EnemyDeathReaction::Collapse:
				pm->Emit("enemyDeath_shard", emitPos, 10);
				pm->Emit("enemyDeath_smoke", emitPos, 3);
				break;
			case EnemyDeathReaction::BossFinal:
				if (!bossFinalBigBurstDone_) {
					pm->Emit("bossClear_core", emitPos, 1);
					pm->Emit("bossClear_ring", emitPos, 3);
					pm->Emit("bossClear_spark", emitPos, 80);
					pm->Emit("bossClear_debris", emitPos, 60);
				}
				break;
			}
			isDead_ = true;
		}
		return;
	}

	// ===== 怒りタイマー更新 =====
	if (isAngry_) {
		angryTimer_ += dt;
		if (angryTimer_ >= angryDuration_) {
			isAngry_ = false;
		}
	}

	Vector3 pos = object_->GetTranslate();

	if (!freezeMove_) {
		switch (behavior_) {
		case EnemyBehavior::StraightStop: {
			if (!stopMove_) {
				pos += velocity_ * factor;
				if (pos.z <= stopZ_) { pos.z = stopZ_; stopMove_ = true; }
			}
			break;
		}
		case EnemyBehavior::SineX: {
			t_ += 0.05f * factor;
			pos.z += velocity_.z * factor;
			pos.x = startX_ + std::sinf(sinePhase_ + t_ * sineFreq_) * sineAmpX_;
			if (pos.z <= stopZ_) { pos.z = stopZ_; }
			break;
		}
		case EnemyBehavior::StrafeLtoR: {
			pos.z += velocity_.z * factor;
			strafePosX_ += strafeSpeed_ * strafeDir_ * factor;
			if (strafePosX_ > strafeRight_) { strafePosX_ = strafeRight_; strafeDir_ = -1; }
			if (strafePosX_ < strafeLeft_) { strafePosX_ = strafeLeft_;  strafeDir_ = +1; }
			pos.x = strafePosX_;
			if (pos.z <= stopZ_) { pos.z = stopZ_; }
			break;
		}
		case EnemyBehavior::ChasePlayer: {
			pos.z += velocity_.z * factor;
			if (playerGetter_) {
				Vector3 toP = playerGetter_() - pos;
				Vector3 desire = { toP.x, toP.y, 0.0f };
				float len = MyMath::Length(desire);
				if (len > 0.001f) {
					Vector3 dir = MyMath::Normalize(desire);
					pos.x += dir.x * chaseSpeed_ * factor;
					pos.y += dir.y * chaseSpeed_ * factor;
				}
			}
			if (pos.z <= stopZ_) { pos.z = stopZ_; }
			break;
		}
		case EnemyBehavior::PounceFromAbove: {
			if (!pounceStarted_) { break; }

			ParticleManager* pm = ParticleManager::GetInstance();

			if (!pounceDiving_) {
				pounceTime_ += dt;
				float t = pounceTime_ / pounceDuration_;
				if (t > 1.0f) t = 1.0f;

				auto EaseOutQuad = [](float x) {
					return 1.0f - (1.0f - x) * (1.0f - x);
					};
				float u = EaseOutQuad(t);

				Vector3 pos1 = MyMath::Vector3Lerp(pounceStart_, pounceApex_, u);
				Vector3 pos2 = MyMath::Vector3Lerp(pounceApex_, pounceTarget_, u);
				Vector3 newPos = MyMath::Vector3Lerp(pos1, pos2, u);

				pos = newPos;

				{
					Vector3 emitPos = pos;
					pm->Emit("enemyPounceTrail", emitPos, 2);
					pm->Emit("enemyPounceSpark", emitPos, 3);
				}

				if (t >= 1.0f) {
					Vector3 dir = pounceTarget_ - pounceStart_;
					float len = MyMath::Length(dir);
					if (len > 0.001f) {
						dir = MyMath::Normalize(dir);
					} else {
						dir = { 0.0f, -0.1f, -1.0f };
					}

					dir.y -= 0.2f;
					dir = MyMath::Normalize(dir);

					float diveSpeed = 0.7f;
					velocity_ = dir * diveSpeed;

					pounceDiving_ = true;
				}
			} else {
				pos += velocity_ * factor;

				Vector3 emitPos = pos;
				pm->Emit("enemyPounceTrail", emitPos, 2);
				pm->Emit("enemyPounceSpark", emitPos, 2);
			}
			break;
		}
		case EnemyBehavior::FreeRoam: {
			auto random01 = []() {
				return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
				};

			float distToTarget = MyMath::Length(roamTarget_ - pos);
			if (!hasRoamTarget_ || distToTarget < 0.5f) {
				hasRoamTarget_ = true;

				Vector3 target;
				target.x = roamMin_.x + (roamMax_.x - roamMin_.x) * random01();
				target.y = roamMin_.y + (roamMax_.y - roamMin_.y) * random01();
				target.z = roamMin_.z + (roamMax_.z - roamMin_.z) * random01();

				if (isAngry_ && playerGetter_) {
					Vector3 p = playerGetter_();
					target.x = (target.x * 0.4f) + (p.x * 0.6f);
					target.x = std::max(roamMin_.x, std::min(roamMax_.x, target.x));
				}

				roamTarget_ = target;
			}

			Vector3 toT = roamTarget_ - pos;
			float len = MyMath::Length(toT);
			if (len > 0.001f) {
				Vector3 dir = toT / len;
				float speed = isAngry_ ? roamSpeedAngry_ : roamSpeedNormal_;
				pos += dir * speed * factor;
			}

			pos.x = std::max(roamMin_.x, std::min(roamMax_.x, pos.x));
			pos.y = std::max(roamMin_.y, std::min(roamMax_.y, pos.y));
			pos.z = std::max(roamMin_.z, std::min(roamMax_.z, pos.z));
			break;
		}
		}
	}

	object_->SetTranslate(pos);

	if (!isDying_) {
		if (pos.z < -30.0f) {
			escaped_ = true;
			isDead_ = true;
		}
	}

	{
		Vector3 center = GetWorldPosition();
		Vector3 size = colliderScale_;

		auto* lr = LineRenderer::GetInstance();

		LineRenderer::Color normal{ 0.0f, 1.0f, 0.0f, 1.0f };
		LineRenderer::Color hit{ 1.0f, 0.0f, 0.0f, 1.0f };

		if (reticle_) {
			Vector3 rayOrigin;
			if (playerGetter_) {
				rayOrigin = playerGetter_();
			} else {
				rayOrigin = reticle_->GetCenterWorldPos();
			}

			Vector3 rayDir = reticle_->GetAimDirection();
			lr->AddAABBWithRayHighlight(center, size, rayOrigin, rayDir, normal, hit);
		} else {
			lr->AddAABB(center, size, normal);
		}
	}

	// ★ ロック脈動（ボスは無効化）
	if (isLocked_ && lockPulseEnabled_) {
		pulseT_ += 0.12f * factor;
		float s = 1.0f + 0.15f * sinf(pulseT_);
		object_->SetScale({ baseScale_.x * s, baseScale_.y * s, baseScale_.z * s });
	} else {
		// ロックしていても脈動が無効なら、基準スケールを維持
		object_->SetScale(baseScale_);
	}
 
	if (canShoot_ && !isDying_) {
		shootTimer_ += factor; // ★ フレーム加算→dt換算
		if (shootTimer_ >= shootInterval_) {
			shootTimer_ = 0.0f;
		}
	}

	object_->Update();
}

void Enemy::Draw(DirectXCommon* dxCommon) {
	if (!object_) return;
	object_->Draw(dxCommon); // Object3d の描画
}

void Enemy::SetCamera(Camera* camera) {
	this->camera = camera; // メンバ変数に保存
	if (object_) {
		object_->SetCamera(camera); // Object3d に反映
	}
}
void Enemy::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos); // 位置設定
}
void Enemy::SetParentScene(BaseScene* scene) {
	parentScene_ = scene; // メンバ変数に保存
}
Vector3 Enemy::GetWorldPosition() const {
	return object_->GetTranslate(); // ワールド位置を返す
}

void Enemy::ImGuiDebug() {
#ifdef USE_IMGUI
	if (!object_) return;

	ImGui::Begin("Enemy");

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	if (ImGui::DragFloat3("位置", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("回転", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}
	if (ImGui::DragFloat3("拡縮", &scale.x, 0.01f)) {
		SetScale(scale);   // モデルと当たり判定両方に反映される
	}

	// 当たり判定スケール編集
	Vector3 col = colliderScale_;
	if (ImGui::DragFloat3("当たり判定サイズ", &col.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col);
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
	int r = std::rand() % 3;

	// 共通で使うノックバック方向
	Vector3 dir = hitDir;
	if (MyMath::Length(dir) < 0.001f) {
		dir = { 0.0f, 0.0f, 1.0f };
	}
	dir = MyMath::Normalize(dir);

	switch (r) {
	case 0: // 吹っ飛び
	default:
		deathReaction_ = EnemyDeathReaction::BlowAway;
		deathDuration_ = 3.0f; // 1秒で消える
		deathVelocity_ = dir * 4.0f;             // ヒット方向へ吹っ飛ぶ
		deathRotateSpeed_ = { 1.5f, 2.0f, 0.8f }; // ぐるっと回転
		break;
	case 1: // 上に吸い込まれる
		deathReaction_ = EnemyDeathReaction::RiseAbsorb;
		deathDuration_ = 1.2f;
		deathVelocity_ = { 0.0f, 3.0f, 0.0f };   // 上方向にスッと上がる
		deathRotateSpeed_ = { 0.0f, 2.0f, 0.0f }; // 少しだけY回転
		break;
	case 2: // 崩れ落ち
		deathReaction_ = EnemyDeathReaction::Collapse;
		deathDuration_ = 0.9f;
		// ちょい前＋下に崩れ落ちる
		deathVelocity_ = { dir.x * 1.5f, -3.0f, dir.z * 1.5f };
		deathRotateSpeed_ = { 3.0f, 0.5f, 0.0f }; // 前に倒れ込む感じ
		break;
	}
}

void Enemy::SyncTransform() {
	if (!object_) return;
	object_->Update();  // 行列と定数バッファだけ更新
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