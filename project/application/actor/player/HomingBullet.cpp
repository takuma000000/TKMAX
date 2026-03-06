#define NOMINMAX
#include "HomingBullet.h"
#include <algorithm>
#include <ParticleManager.h>
#include "AABB.h"
#include "Player.h"
#include "Enemy.h"
#include "MidBossCore.h"
#include "TrailRibbonRenderer.h"

namespace {
	static Vector3 DirToEuler_(const Vector3& dir) {
		Vector3 d = dir;
		float len = MyMath::Length(d);
		if (len < 0.0001f) { return { 0,0,0 }; }
		d = d / len;

		float yaw = std::atan2(d.x, d.z);
		float pitch = -std::asin(d.y);
		return { pitch, yaw, 0.0f };
	}
}

void HomingBullet::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetScale({ kDefaultScale_, kDefaultScale_, kDefaultScale_ });

	prevPos_ = object_->GetTranslate();
	lifeTimer_ = 0.0f;
	trailPts_.clear();
	trailDistAcc_ = 0.0f;
}

void HomingBullet::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos);
	prevPos_ = pos;

	trailPts_.clear();
	trailPts_.push_back(pos);
	trailDistAcc_ = 0.0f;
	isTrailFading_ = false;
	trailFadeTimer_ = 0.0f;
}

void HomingBullet::SetCamera(TKM::Camera* camera) {
	camera_ = camera;
	if (object_) {
		object_->SetCamera(camera);
	}
}

void HomingBullet::SetEnemy(Enemy* enemy) {
	enemy_ = enemy;
}

void HomingBullet::SetPlayer(Player* player) {
	player_ = player;
}

void HomingBullet::SetCore(MidBossCore* core) {
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

	arcT_ = 0.0f;
	arcDuration_ = std::max(0.001f, duration);
	isArcActive_ = true;

	SetPosition(start);
}

void HomingBullet::UpdateTrail_(const Vector3& p) {
	if (trailPts_.empty()) {
		trailPts_.push_back(p);
		trailDistAcc_ = 0.0f;
		return;
	}

	float moveDist = MyMath::Length(p - prevPos_);
	trailDistAcc_ += moveDist;

	if (trailDistAcc_ >= kTrailStep_) {
		trailPts_.push_back(p);
		trailDistAcc_ = 0.0f;

		while (trailPts_.size() > kTrailHardCap_) {
			trailPts_.erase(trailPts_.begin());
		}
	}
}

void HomingBullet::Update() {
	if (!object_ || isDead_) { return; }

	if (isTrailFading_) {
		trailFadeTimer_ += dt_;

		while (trailFadeTimer_ >= trailFadeInterval_) {
			trailFadeTimer_ -= trailFadeInterval_;

			if (!trailPts_.empty()) {
				trailPts_.erase(trailPts_.begin()); // player側(古い点)から消す
			}
		}

		if (trailPts_.size() < 2) {
			isDead_ = true;
		}

		return;
	}

	Vector3 oldPos = object_->GetTranslate();
	prevPos_ = oldPos;

	lifeTimer_ += dt_;
	if (lifeTimer_ >= kLifeTime_) {
		isDead_ = true;
		return;
	}

	if (isArcActive_) {
		arcT_ += dt_ / arcDuration_;
		float t = std::clamp(arcT_, 0.0f, 1.0f);

		Vector3 pos = MyMath::Bezier3(p0_, p1_, p2_, p3_, t);

		// 終盤だけ敵の現在位置へ少しずつ寄せる
		if (enemy_ && !enemy_->IsDead() && t >= 0.65f) {
			float followT = (t - 0.65f) / (1.0f - 0.65f);
			followT = std::clamp(followT, 0.0f, 1.0f);

			Vector3 enemyPos = enemy_->GetWorldPosition();

			// そのままだと敵の中心へ刺さりすぎるなら少し上を狙う
			enemyPos.y += 1.5f;

			pos = pos + (enemyPos - pos) * followT;
		}
		object_->SetTranslate(pos);

		// ============================
		// LB弾：トレイル全体にキラキラ + 稲光
		// ============================
		{
			auto* pm = TKM::ParticleManager::GetInstance();

			if (trailPts_.size() >= 2) {

				auto frand = [](float a, float b) {
					return a + (b - a) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
					};

				// トレイル履歴を間引きながら走査
				const int step = 2; // 大きいほど軽い

				for (size_t i = 0; i < trailPts_.size(); i += step) {

					Vector3 centerPos = trailPts_[i];

					Vector3 dir = { 0,0,1 };
					if (i + 1 < trailPts_.size()) {
						dir = trailPts_[i + 1] - trailPts_[i];
						float len = MyMath::Length(dir);
						if (len > 0.0001f) dir = dir / len;
					}

					// ----------------------------
					// トレイル横方向
					// ----------------------------
					Vector3 side = { 1,0,0 };

					if (camera_) {
						Vector3 camVec = camera_->GetTranslate() - centerPos;
						float camLen = MyMath::Length(camVec);
						if (camLen > 0.0001f) {
							camVec /= camLen;
							Vector3 s = MyMath::Cross(camVec, dir);
							float sLen = MyMath::Length(s);
							if (sLen > 0.0001f) side = s / sLen;
						}
					}

					float sign = (rand() % 2 == 0) ? -1.0f : 1.0f;

					float edgeBase = 0.72f;
					float overhang = frand(0.05f, 0.30f);

					Vector3 edgePos = centerPos + side * sign * (edgeBase + overhang);

					// ----------------------------
					// キラキラ
					// ----------------------------
					if ((rand() % 100) < 80) {
						pm->Emit("trail_lb_glitter", edgePos, 2);

						if ((rand() % 100) < 55) {
							pm->Emit("trail_lb_glitter", edgePos, 2);
						}
					}

					// ----------------------------
					// 稲光
					// ----------------------------
					if ((rand() % 100) < 45) {
						Vector3 boltPos = centerPos + side * sign * frand(0.65f, 1.15f);

						pm->Emit("trail_lb_bolt_main", boltPos, 2);
						pm->Emit("trail_lb_bolt_core", boltPos, 1);

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

		if (enemy_ && !enemy_->IsDead() && t2 >= 0.65f) {
			float followT2 = (t2 - 0.65f) / (1.0f - 0.65f);
			followT2 = std::clamp(followT2, 0.0f, 1.0f);

			Vector3 enemyPos2 = enemy_->GetWorldPosition();
			enemyPos2.y += 1.5f;

			nextPos = nextPos + (enemyPos2 - nextPos) * followT2;
		}

		Vector3 dir = nextPos - pos;
		if (MyMath::Length(dir) > 0.0001f) {
			object_->SetRotate(DirToEuler_(dir));
		}

		UpdateTrail_(pos);

		Vector3 bulletPos = object_->GetTranslate();
		Vector3 bulletScale = object_->GetScale();

		auto CheckSweptHitAABB = [&](const Vector3& targetPos, const Vector3& targetSize) -> bool {
			AABB bulletBox(bulletPos, bulletScale);
			AABB targetBox(targetPos, targetSize);

			if (targetBox.IsIntersectSegment(prevPos_, bulletPos)) {
				return true;
			}
			if (bulletBox.IsCollidingWithAABB(targetBox)) {
				return true;
			}
			return false;
			};

		if (enemy_ && !enemy_->IsDead()) {
			Vector3 enemyPos = enemy_->GetWorldPosition();
			Vector3 enemySize = enemy_->GetColliderScale();

			if (CheckSweptHitAABB(enemyPos, enemySize)) {
				isHit_ = true;
				isTrailFading_ = true;
				trailFadeTimer_ = 0.0f;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				Vector3 hitPos = bulletPos;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);

				enemy_->OnHitWithDamage(kEnemyDamage_);

				if (player_) {
					player_->StartCameraShake(40);
				}
				return;
			}
		}

		if (core_ && !core_->IsDead()) {
			Vector3 corePos = core_->GetWorldPosition();
			Vector3 coreSize = core_->GetColliderScale();

			if (CheckSweptHitAABB(corePos, coreSize)) {
				isHit_ = true;
				isTrailFading_ = true;
				trailFadeTimer_ = 0.0f;

				TKM::ParticleManager* pm = TKM::ParticleManager::GetInstance();
				Vector3 hitPos = bulletPos;
				pm->Emit("lt_nova_core", hitPos, 1);
				pm->Emit("lt_nova_wave", hitPos, 3);
				pm->Emit("lt_nova_burst", hitPos, 40);
				pm->Emit("lt_nova_debris", hitPos, 120);
				pm->Emit("lt_nova_crack", hitPos, 80);

				core_->OnHitWithDamage(kCoreDamage_);

				if (player_) {
					player_->StartCameraShake(40);
				}
				return;
			}
		}

		if (t >= 1.0f) {
			isArcActive_ = false;
			isTrailFading_ = true;
			trailFadeTimer_ = 0.0f;
		}
	}

	object_->Update();
}

void HomingBullet::Draw(TKM::DirectXCommon* dxCommon) {
	if (!object_ || isDead_ || isTrailFading_) { return; }
	object_->Draw(dxCommon);
}

void HomingBullet::DrawTrail(TKM::DirectXCommon* dxCommon) {
	if (!camera_ || trailPts_.empty() || isDead_) { return; }

	auto* rr = TKM::TrailRibbonRenderer::GetInstance();
	const auto& p = rr->GetDebugParams();
	if (!p.enable) { return; }

	std::vector<Vector3> drawPts = trailPts_;
	Vector3 currentPos = object_->GetTranslate();
	if (drawPts.empty() || MyMath::Length(currentPos - drawPts.back()) > 0.0001f) {
		drawPts.push_back(currentPos);
	}

	if (drawPts.size() < 2) { return; }

	rr->DrawRibbon(
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