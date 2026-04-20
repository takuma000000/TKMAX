#include "PlayerShotManager.h"

#include "Player.h"
#include "Enemy.h"
#include "MidBossCore.h"
#include "BarrierCoreManager.h"
#include "AABB.h"
#include "MyMath.h"

#include <limits>

void PlayerShotManager::Initialize(Player* owner, TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	owner_ = owner;
	common_ = common;
	dxCommon_ = dxCommon;

	rbAmmo_ = 0;
	rbEmptyTimer_ = 0.0f;
	rbRefilling_ = false;
	rbRefillValue_ = 0.0f;
	rbNoFireTimer_ = 0.0f;
	rbShotCooldownTimer_ = 0.0f;

	lbAmmo_ = 0;
	lbNoFireTimer_ = 0.0f;
}

void PlayerShotManager::Update(float dt, bool canShoot) {
	RemoveDeadTargets();

	if (canShoot && shootingEnabled_) {
		UpdateLockState_();
		HandleShooting_(dt);
	} else {
		ClearLockState();
	}

	for (auto it = bullets_.begin(); it != bullets_.end();) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = bullets_.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = homingBullets_.begin(); it != homingBullets_.end();) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = homingBullets_.erase(it);
		} else {
			++it;
		}
	}
}

void PlayerShotManager::UpdateLockState_() {
	TKM::Input* input = TKM::Input::GetInstance();
	Enemy* cur = (enemy_ && !enemy_->IsDead()) ? enemy_ : nullptr;

	bool hold = (input->GetRightTrigger() > kTriggerThreshold) && (canUseSpecial_ || debugUnlimitedSpecial_);

	if (lastLockedEnemy_ && lastLockedEnemy_ != cur) {
		lastLockedEnemy_->SetLocked(false);
	}

	if (cur && hold) {
		cur->SetLocked(true);
		lastLockedEnemy_ = cur;
	} else {
		if (cur) {
			cur->SetLocked(false);
		}
		lastLockedEnemy_ = nullptr;
	}
}

void PlayerShotManager::ClearLockState() {
	if (lastLockedEnemy_) {
		lastLockedEnemy_->SetLocked(false);
		lastLockedEnemy_ = nullptr;
	}
}

void PlayerShotManager::RemoveDeadTargets() {
	if (enemy_ && enemy_->IsDead()) {
		enemy_ = nullptr;
	}

	if (core_ && core_->IsDead()) {
		core_ = nullptr;
	}
}

void PlayerShotManager::SetShootingEnabled(bool enabled) {
	shootingEnabled_ = enabled;

	if (!enabled) {
		rtHeld_ = false;
		ltHeld_ = false;
		ClearLockState();

		bullets_.clear();
		homingBullets_.clear();

		rbShotCooldownTimer_ = 0.0f;
		rbRefilling_ = false;
		rbRefillValue_ = float(rbAmmo_);
		rbEmptyTimer_ = 0.0f;
		rbNoFireTimer_ = 0.0f;
		lbNoFireTimer_ = 0.0f;
	}
}

void PlayerShotManager::DrawTrails(TKM::DirectXCommon* dxCommon) {
	for (auto& bullet : bullets_) {
		if (!bullet) { continue; }
		bullet->DrawTrail(dxCommon);
	}
	for (auto& bullet : homingBullets_) {
		if (!bullet) { continue; }
		bullet->DrawTrail(dxCommon);
	}
}

void PlayerShotManager::DrawBullets(TKM::DirectXCommon* dxCommon) {
	for (auto& bullet : bullets_) {
		if (!bullet) { continue; }
		bullet->Draw(dxCommon);
	}
}

void PlayerShotManager::OnEnemyDestroyed(Enemy* e) {
	if (enemy_ == e) {
		enemy_ = nullptr;
	}

	for (auto& b : bullets_) {
		if (!b) { continue; }
		if (b->GetEnemy() == e) {
			b->SetEnemy(nullptr);
		}
	}

	for (auto& b : homingBullets_) {
		if (!b) { continue; }
		if (b->GetEnemy() == e) {
			b->SetEnemy(nullptr);
		}
	}
}

void PlayerShotManager::OnMidBossCoreDestroyed(MidBossCore* core) {
	if (!core) {
		return;
	}

	for (auto& b : bullets_) {
		if (!b) { continue; }
		b->SetCore(nullptr);
	}

	for (auto& b : homingBullets_) {
		if (!b) { continue; }
		b->SetCore(nullptr);
	}

	core_ = nullptr;
}

void PlayerShotManager::SetConfig(const PlayerShotConfig* config) {
	config_ = config;

	assert(config_ && "PlayerShotConfig が未設定です");

	rbAmmo_ = std::max(0, config_->GetRB().ammoMax_);
	lbAmmo_ = std::max(0, config_->GetLB().ammoMax_);

	rbRefillValue_ = float(rbAmmo_);
	rbEmptyTimer_ = 0.0f;
	rbNoFireTimer_ = 0.0f;
	lbNoFireTimer_ = 0.0f;
	rbRefilling_ = false;
	rbShotCooldownTimer_ = 0.0f;
}

void PlayerShotManager::HandleShooting_(float dt) {
	assert(config_ && "PlayerShotConfig が未設定です");

	const PlayerShotConfig::RBConfig& rb = config_->GetRB();
	const PlayerShotConfig::LBConfig& lb = config_->GetLB();

	//====================
	// RB弾 リチャージ更新（0回復 + アイドル回復）
	//====================
	{
		if (!rbRefilling_) {
			rbNoFireTimer_ += dt;
		}

		const bool empty = (rbAmmo_ <= 0);
		const bool idleReady = (!empty && rbNoFireTimer_ >= rb.refillWaitSec_);
		const bool emptyReady = (empty && (rbEmptyTimer_ >= rb.refillWaitSec_));

		if (!rbRefilling_) {
			if (empty) {
				rbEmptyTimer_ += dt;
			} else {
				rbEmptyTimer_ = 0.0f;
			}
		}

		if (!rbRefilling_ && (idleReady || emptyReady)) {
			rbRefilling_ = true;
			rbRefillValue_ = empty ? 0.0f : float(rbAmmo_);
		}

		if (rbRefilling_) {
			const float speed = float(std::max(1, rb.ammoMax_)) / std::max(0.001f, rb.refillSec_);
			rbRefillValue_ += speed * dt;
			rbAmmo_ = std::clamp(int(rbRefillValue_), 0, std::max(1, rb.ammoMax_));
			if (rbAmmo_ >= std::max(1, rb.ammoMax_)) {
				rbAmmo_ = std::max(1, rb.ammoMax_);
				rbRefilling_ = false;
				rbEmptyTimer_ = 0.0f;
				rbNoFireTimer_ = 0.0f;
			}
		}
	}

	//====================
	// LB弾 自動満タン回復
	//====================
	{
		if (!debugUnlimitedLB_ && lbAmmo_ < std::max(1, lb.ammoMax_)) {
			lbNoFireTimer_ += dt;

			if (lbNoFireTimer_ >= lb.refillWaitSec_) {
				lbAmmo_ = std::max(1, lb.ammoMax_);
				lbNoFireTimer_ = 0.0f;
			}
		} else {
			lbNoFireTimer_ = 0.0f;
		}
	}

	//====================
	// RB弾 クールダウン更新
	//====================
	if (rbShotCooldownTimer_ > 0.0f) {
		rbShotCooldownTimer_ -= dt;
		if (rbShotCooldownTimer_ < 0.0f) {
			rbShotCooldownTimer_ = 0.0f;
		}
	}

	RBShoot_();
	LBShoot_();
}

void PlayerShotManager::RBShoot_() {
	assert(config_ && "PlayerShotConfig が未設定です");
	const PlayerShotConfig::RBConfig& rb = config_->GetRB();

	TKM::Input* input = TKM::Input::GetInstance();

	const bool padRB = input->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER);
	const bool keyK = input->PushKey(DIK_K);

	if (!padRB && !keyK) {
		return;
	}
	if (rbShotCooldownTimer_ > 0.0f) {
		return;
	}
	if (rbAmmo_ <= 0 || rbRefilling_) {
		return;
	}
	if (!ownerObject_) {
		return;
	}

	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(common_, dxCommon_);

	Vector3 startPos = ownerObject_->GetTranslate();
	bullet->SetPosition(startPos);

	Vector3 dir = { 0, 0, 1 };

	if (reticle_) {
		dir = reticle_->GetAimDirection();
		float len = MyMath::Length(dir);
		if (len <= 0.01f) {
			dir = { 0, 0, 1 };
		}
	}

	bullet->SetVelocity(dir * rb.bulletSpeed_);
	bullet->SetCamera(camera_);
	bullet->SetPlayer(owner_);
	bullet->SetUseTrail(false);
	bullet->SetBarrierCoreManager(barrierCoreManager_);

	Enemy* targetEnemy = nullptr;

	if (allEnemies_) {
		Vector3 rayDir = dir;
		float len = MyMath::Length(rayDir);
		if (len > 0.001f) {
			rayDir = rayDir / len;
		}

		Vector3 rayEnd = startPos + rayDir * 150.0f;
		float closestDist = std::numeric_limits<float>::max();

		for (auto& e : *allEnemies_) {
			if (!e) continue;
			if (e->IsDead() || e->IsDying()) continue;

			Vector3 center = e->GetWorldPosition();
			Vector3 size = e->GetColliderScale();
			AABB box(center, size);

			if (box.IsIntersectSegment(startPos, rayEnd)) {
				float dist = MyMath::Length(center - startPos);
				if (dist < closestDist) {
					closestDist = dist;
					targetEnemy = e.get();
				}
			}
		}
	}

	if (!targetEnemy) {
		if (enemy_ && !enemy_->IsDead()) {
			targetEnemy = enemy_;
		}
	}

	bullet->SetEnemy(targetEnemy);
	bullets_.push_back(std::move(bullet));

	rbShotCooldownTimer_ = rb.shotCooldownSec_;
	rbAmmo_ = std::max(0, rbAmmo_ - 1);
	rbNoFireTimer_ = 0.0f;
}

void PlayerShotManager::LBShoot_() {
	assert(config_ && "PlayerShotConfig が未設定です");
	const PlayerShotConfig::LBConfig& lb = config_->GetLB();

	TKM::Input* input = TKM::Input::GetInstance();

	// ▼ LB：山なりホーミング弾（ロックオンしてる敵に向かう、LB弾は自動で満タン回復する）
	if ((input->TriggerButton(XINPUT_GAMEPAD_LEFT_SHOULDER) || input->TriggerKey(DIK_L)) && !ltHeld_) {

		// デバッグ無限LBモードでないなら、弾数が0のときは発射できない
		if (!debugUnlimitedLB_ && lbAmmo_ <= 0) {
			return;
		}
		if (!ownerObject_) {
			return;
		}

		// LB弾はロックオンしてる敵に向かう山なりホーミング弾
		auto bullet = std::make_unique<HomingBullet>();
		bullet->Initialize(common_, dxCommon_);

		Vector3 start = ownerObject_->GetTranslate(); // 発射位置

		// 終点
		Vector3 end = start + Vector3{ 0.0f, 0.0f, lb.forwardOffsetZ_ };

		// 優先順位：
		// 1. コア
		// 2. ロック中の敵
		// 3. 前方固定
		if (core_ && !core_->IsDead()) {
			end = core_->GetWorldPosition();
		} else if (enemy_ && !enemy_->IsDead()) {
			end = enemy_->GetWorldPosition();
		}

		// 山なり制御点を作る
		Vector3 flat = end - start;
		flat.y = 0.0f;
		float flatLen = MyMath::Length(flat);

		Vector3 forward = { 0.0f, 0.0f, 1.0f };
		if (flatLen > 0.001f) {
			forward = flat / flatLen;
		}

		float arcHeight = std::clamp(flatLen * 0.25f, 6.0f, lb.arcHeight_);

		// 制御点は、スタートから前方に少し進んだ位置と、エンドから前方に少し戻った位置の2点を、さらに上に持ち上げる
		Vector3 c1 = start + forward * (flatLen * 0.25f) + Vector3{ 0.0f, arcHeight, 0.0f };
		Vector3 c2 = end - forward * (flatLen * 0.20f) + Vector3{ 0.0f, arcHeight * 0.85f, 0.0f };

		// 弾の基本設定
		bullet->SetPosition(start);
		bullet->SetEnemy(enemy_);
		bullet->SetCamera(camera_);
		bullet->SetPlayer(owner_);
		bullet->SetCore(core_);
		bullet->StartArc(start, c1, c2, end, 0.4f);

		homingBullets_.push_back(std::move(bullet));

		if (!debugUnlimitedLB_) {
			lbAmmo_ = std::max(0, lbAmmo_ - 1);
		}
		lbNoFireTimer_ = 0.0f;

		// カメラズームとコントローラーの振動を開始
		if (owner_) {
			owner_->ZoomCamera();
			owner_->StartRumble(0.12f, 42000, 42000);
		}

		ltHeld_ = true;
	}

	// 離した瞬間：ホールド状態解除
	if (!input->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER) && !input->PushKey(DIK_L)) {
		ltHeld_ = false;
	}
}