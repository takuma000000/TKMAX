#include "ActionPlayerBulletManager.h"
#include "Input.h"
#include <algorithm>

void ActionPlayerBulletManager::Initialize(TKM::DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	bullets_.clear();
	shotCooldownTimer_ = 0.0f;
}

void ActionPlayerBulletManager::Update(
	const Vector2& playerPosition,
	const Vector2& playerSize,
	float facingDirection,
	std::vector<std::unique_ptr<ActionEnemy>>& enemies) {

	if (shotCooldownTimer_ > 0.0f) {
		shotCooldownTimer_ -= kFrameTime_;

		if (shotCooldownTimer_ < 0.0f) {
			shotCooldownTimer_ = 0.0f;
		}
	}

	auto* input = TKM::Input::GetInstance();

	if (input->TriggerKey(DIK_J) && shotCooldownTimer_ <= 0.0f) {
		Shoot_(playerPosition, playerSize, facingDirection);
	}

	for (auto& bullet : bullets_) {
		if (bullet) {
			bullet->Update();
		}
	}

	CheckHitEnemies_(enemies);

	enemies.erase(
		std::remove_if(
			enemies.begin(),
			enemies.end(),
			[](const std::unique_ptr<ActionEnemy>& enemy) {
				return !enemy || enemy->IsDead();
			}
		),
		enemies.end()
	);

	bullets_.erase(
		std::remove_if(
			bullets_.begin(),
			bullets_.end(),
			[](const std::unique_ptr<ActionPlayerBullet>& bullet) {
				return !bullet || bullet->IsDead();
			}
		),
		bullets_.end()
	);
}

void ActionPlayerBulletManager::Draw(float scrollX) {
	for (auto& bullet : bullets_) {
		if (bullet) {
			bullet->Draw(scrollX);
		}
	}
}

void ActionPlayerBulletManager::Shoot_(
	const Vector2& playerPosition,
	const Vector2& playerSize,
	float facingDirection
) {
	Vector2 bulletPosition = {
		playerPosition.x + (facingDirection > 0 ? playerSize.x : 0.0f),
		playerPosition.y + playerSize.y * 0.5f - 6.0f
	};

	auto bullet = std::make_unique<ActionPlayerBullet>();
	bullet->Initialize(dxCommon_, bulletPosition, facingDirection);

	bullets_.push_back(std::move(bullet));
}

void ActionPlayerBulletManager::CheckHitEnemies_(std::vector<std::unique_ptr<ActionEnemy>>& enemies) {
	for (auto& bullet : bullets_) {
		if (!bullet || bullet->IsDead()) {
			continue;
		}

		for (auto& enemy : enemies) {
			if (!enemy || enemy->IsDead() || enemy->IsDying()) {
				continue;
			}

			if (bullet->GetAABB().IsCollidingWithAABB(enemy->GetAABB())) {
				enemy->StartKnockbackDeath(bullet->GetDirection());
				bullet->Kill();
				break;
			}
		}
	}
}