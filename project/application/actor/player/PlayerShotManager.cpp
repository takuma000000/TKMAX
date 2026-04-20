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

	rbAmmo_ = kRbAmmoMax_;
	rbEmptyTimer_ = 0.0f;
	rbRefilling_ = false;
	rbRefillValue_ = float(rbAmmo_);

	lbAmmo_ = kLbAmmoMax_;
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

}