#include "ActionPlayerBulletManager.h"
#include "Input.h"
#include <algorithm>
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionPlayerBulletManager::Initialize(TKM::DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	bullets_.clear();
	clashEffects_.clear();
	shotCooldownTimer_ = 0.0f;

	const std::string texturePath = "./resources/texture/circle2.png";
	TKM::TextureManager::GetInstance()->LoadTexture(texturePath);
	clashEffectSprite_ = std::make_unique<TKM::Sprite>();
	clashEffectSprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		texturePath
	);
	clashEffectSprite_->SetAutoAdjustTextureSize(false);
	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath);
	clashEffectSprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	clashEffectSprite_->SetTextureSize({ static_cast<float>(meta.width), static_cast<float>(meta.height) });
}

void ActionPlayerBulletManager::Update(
	const Vector2& playerPosition,
	const Vector2& playerSize,
	float facingDirection,
	std::vector<std::unique_ptr<ActionEnemy>>& enemies,
	const std::vector<std::unique_ptr<ActionBlock>>& blocks,
	float scrollX,
	float screenWidth) {

	if (shotCooldownTimer_ > 0.0f) {
		shotCooldownTimer_ -= kFrameTime_;

		if (shotCooldownTimer_ < 0.0f) {
			shotCooldownTimer_ = 0.0f;
		}
	}

	auto* input = TKM::Input::GetInstance();

	if (input->TriggerKey(DIK_J) && shotCooldownTimer_ <= 0.0f) {
		Shoot_(playerPosition, playerSize, facingDirection);
		shotCooldownTimer_ = kShotCooldownSec_;
	}

	for (auto& bullet : bullets_) {
		if (bullet) {
			bullet->Update();
		}
	}

	// 画面外に出た弾を先に消す
	KillOutOfScreen_(scrollX, screenWidth);

	// ブロックに当たった弾を消す
	CheckHitBlocks_(blocks);

	// TypeC追従弾と相殺
	CheckHitEnemyBullets_(enemies);

	// 敵に当たった弾を処理
	CheckHitEnemies_(enemies);
	UpdateClashEffects_();

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

	if (!clashEffectSprite_) {
		return;
	}

	for (const auto& effect : clashEffects_) {
		const float t = std::clamp(effect.timer / kClashEffectDuration_, 0.0f, 1.0f);
		const float scale = 1.0f + t * 1.8f;
		const float size = kClashEffectBaseSize_ * scale;
		const float alpha = 1.0f - t;

		clashEffectSprite_->SetPosition({
			effect.worldCenter.x - scrollX - size * 0.5f,
			effect.worldCenter.y - size * 0.5f
			});
		clashEffectSprite_->SetSize({ size, size });
		clashEffectSprite_->SetColor({ 1.0f, 0.45f + 0.35f * (1.0f - t), 0.1f, alpha });
		clashEffectSprite_->Update();
		clashEffectSprite_->Draw();
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

void ActionPlayerBulletManager::CheckHitBlocks_(const std::vector<std::unique_ptr<ActionBlock>>& blocks) {
	for (auto& bullet : bullets_) {
		if (!bullet || bullet->IsDead()) {
			continue;
		}

		for (auto& block : blocks) {
			if (!block) {
				continue;
			}

			if (bullet->GetAABB().IsCollidingWithAABB(block->GetAABB())) {
				bullet->Kill();
				break;
			}
		}
	}
}

void ActionPlayerBulletManager::KillOutOfScreen_(float scrollX, float screenWidth) {
	const float screenLeft = scrollX;
	const float screenRight = scrollX + screenWidth;

	for (auto& bullet : bullets_) {
		if (!bullet || bullet->IsDead()) {
			continue;
		}

		AABB bulletAABB = bullet->GetAABB();

		Vector3 center = bulletAABB.GetCenter();
		Vector3 half = bulletAABB.GetHalfSize();

		const float bulletLeft = center.x - half.x;
		const float bulletRight = center.x + half.x;

		if (bulletRight < screenLeft || bulletLeft > screenRight) {
			bullet->Kill();
		}
	}
}

void ActionPlayerBulletManager::CheckHitEnemyBullets_(std::vector<std::unique_ptr<ActionEnemy>>& enemies) {
	for (auto& bullet : bullets_) {
		if (!bullet || bullet->IsDead()) {
			continue;
		}

		for (auto& enemy : enemies) {
			if (!enemy || enemy->IsDead()) {
				continue;
			}

			Vector2 hitPosition;
			if (!enemy->TryHitDropObject(bullet->GetAABB(), &hitPosition)) {
				continue;
			}

			bullet->Kill();
			SpawnClashEffect_(hitPosition);
			break;
		}
	}
}

void ActionPlayerBulletManager::UpdateClashEffects_() {
	for (auto& effect : clashEffects_) {
		effect.timer += kFrameTime_;
	}

	clashEffects_.erase(
		std::remove_if(
			clashEffects_.begin(),
			clashEffects_.end(),
			[](const ClashEffect& effect) {
				return effect.timer >= kClashEffectDuration_;
			}
		),
		clashEffects_.end()
	);
}

void ActionPlayerBulletManager::SpawnClashEffect_(const Vector2& worldCenter) {
	clashEffects_.push_back({ worldCenter, 0.0f });
}