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
	float facingDirection) {

	if (shotCooldownTimer_ > 0.0f) {
		shotCooldownTimer_ -= kFrameTime_;

		if (shotCooldownTimer_ < 0.0f) {
			shotCooldownTimer_ = 0.0f;
		}
	}

	auto* input = TKM::Input::GetInstance();

	if (input->TriggerKey(DIK_SPACE) && shotCooldownTimer_ <= 0.0f) {
		Shoot_(playerPosition, playerSize, facingDirection);
	}

	for (auto& bullet : bullets_) {
		if (bullet) {
			bullet->Update();
		}
	}

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