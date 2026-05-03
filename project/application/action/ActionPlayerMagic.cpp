#include "ActionPlayerMagic.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "Input.h"
#include <cmath>

void ActionPlayerMagic::Initialize(TKM::DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	phase_ = Phase::Idle;
	targetEnemy_ = nullptr;
	timer_ = 0.0f;

	CreateSprite_(
		auraSprite_,
		"./resources/texture/circle2.png",
		{ kAuraWidth_, kAuraHeight_ },
		{ 0.4f, 0.2f, 1.0f, 0.0f }
	);

	CreateSprite_(
		portalSprite_,
		"./resources/texture/circle2.png",
		{ kPortalWidth_, kPortalHeight_ },
		{ 0.2f, 0.1f, 1.0f, 0.0f }
	);

	arrows_.clear();

	for (int i = 0; i < kArrowCount_; ++i) {
		MagicArrow arrow{};
		arrow.position = {};
		arrow.fallSpeed = 10.0f + static_cast<float>(i % 3) * 2.0f;
		arrow.active = false;

		CreateSprite_(
			arrow.sprite,
			"./resources/texture/circle2.png",
			{ kArrowWidth_, kArrowHeight_ },
			{ 0.7f, 0.9f, 1.0f, 0.0f }
		);

		arrows_.push_back(std::move(arrow));
	}
}

void ActionPlayerMagic::Update(
	const Vector2& playerPosition,
	const Vector2& playerSize,
	float facingDirection,
	float scrollX,
	float screenWidth,
	std::vector<std::unique_ptr<ActionEnemy>>& enemies
) {
	if (phase_ == Phase::Idle) {
		if (TKM::Input::GetInstance()->TriggerKey(DIK_K)) {
			ActionEnemy* target = FindTargetEnemy_(
				playerPosition,
				facingDirection,
				scrollX,
				screenWidth,
				enemies
			);

			if (target) {
				StartMagic_(target, playerPosition);
			}
		}

		return;
	}

	if (!targetEnemy_ || targetEnemy_->IsDead()) {
		phase_ = Phase::Idle;
		targetEnemy_ = nullptr;
		return;
	}

	if (phase_ == Phase::Charge) {
		UpdateCharge_();
		return;
	}

	if (phase_ == Phase::Rain) {
		UpdateRain_();
		return;
	}

	if (phase_ == Phase::Finish) {
		FinishMagic_();
	}
}

void ActionPlayerMagic::Draw(float scrollX) {
	if (phase_ == Phase::Idle) {
		return;
	}

	if (auraSprite_) {
		auraSprite_->SetPosition({ auraPosition_.x - scrollX, auraPosition_.y });
		auraSprite_->Update();
		auraSprite_->Draw();
	}

	if (portalSprite_) {
		portalSprite_->SetPosition({ portalPosition_.x - scrollX, portalPosition_.y });
		portalSprite_->Update();
		portalSprite_->Draw();
	}

	for (auto& arrow : arrows_) {
		if (!arrow.active || !arrow.sprite) {
			continue;
		}

		arrow.sprite->SetPosition({ arrow.position.x - scrollX, arrow.position.y });
		arrow.sprite->Update();
		arrow.sprite->Draw();
	}
}

ActionEnemy* ActionPlayerMagic::FindTargetEnemy_(
	const Vector2& playerPosition,
	float facingDirection,
	float scrollX,
	float screenWidth,
	std::vector<std::unique_ptr<ActionEnemy>>& enemies
) {
	ActionEnemy* nearest = nullptr;
	float nearestDistance = 999999.0f;

	const float screenLeft = scrollX;
	const float screenRight = scrollX + screenWidth;

	for (auto& enemy : enemies) {
		if (!enemy || enemy->IsDead() || enemy->IsDying() || enemy->IsMagicLocked()) {
			continue;
		}

		const Vector2& enemyPos = enemy->GetPosition();
		Vector2 enemySize = enemy->GetSize();

		const float enemyLeft = enemyPos.x;
		const float enemyRight = enemyPos.x + enemySize.x;

		//=============================================================
		// 画面外の敵は魔法対象にしない
		//=============================================================
		if (enemyRight < screenLeft || enemyLeft > screenRight) {
			continue;
		}

		float diffX = enemyPos.x - playerPosition.x;

		if (facingDirection > 0.0f && diffX < 0.0f) {
			continue;
		}

		if (facingDirection < 0.0f && diffX > 0.0f) {
			continue;
		}

		float distance = std::abs(diffX);

		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearest = enemy.get();
		}
	}

	return nearest;
}

void ActionPlayerMagic::StartMagic_(ActionEnemy* target, const Vector2& playerPosition) {
	phase_ = Phase::Charge;
	targetEnemy_ = target;
	timer_ = 0.0f;

	targetEnemy_->SetMagicLocked(true);

	auraPosition_ = {
		playerPosition.x - 16.0f,
		playerPosition.y - 16.0f
	};

	for (auto& arrow : arrows_) {
		arrow.active = false;
	}

	if (auraSprite_) {
		auraSprite_->SetColor({ 0.4f, 0.2f, 1.0f, 0.65f });
	}

	if (portalSprite_) {
		portalSprite_->SetColor({ 0.2f, 0.1f, 1.0f, 0.0f });
	}
}

void ActionPlayerMagic::UpdateCharge_() {
	timer_ += kFrameTime_;

	if (auraSprite_) {
		float pulse = 0.55f + std::sin(timer_ * 18.0f) * 0.25f;
		auraSprite_->SetColor({ 0.4f, 0.2f, 1.0f, pulse });
	}

	if (timer_ < kChargeTime_) {
		return;
	}

	phase_ = Phase::Rain;
	timer_ = 0.0f;

	const Vector2& targetPos = targetEnemy_->GetPosition();
	Vector2 targetSize = targetEnemy_->GetSize();

	portalPosition_ = {
		targetPos.x + targetSize.x * 0.5f - kPortalWidth_ * 0.5f,
		targetPos.y - 120.0f
	};

	if (auraSprite_) {
		auraSprite_->SetColor({ 0.4f, 0.2f, 1.0f, 0.0f });
	}

	if (portalSprite_) {
		portalSprite_->SetColor({ 0.2f, 0.1f, 1.0f, 0.85f });
	}

	for (int i = 0; i < kArrowCount_; ++i) {
		float offsetX = -48.0f + static_cast<float>(i) * 14.0f;

		arrows_[i].position = {
			targetPos.x + targetSize.x * 0.5f + offsetX,
			portalPosition_.y + 24.0f - static_cast<float>(i % 3) * 34.0f
		};

		arrows_[i].fallSpeed = 9.0f + static_cast<float>(i % 4) * 2.0f;
		arrows_[i].active = true;

		if (arrows_[i].sprite) {
			arrows_[i].sprite->SetColor({ 0.7f, 0.9f, 1.0f, 1.0f });
		}
	}
}

void ActionPlayerMagic::UpdateRain_() {
	timer_ += kFrameTime_;

	const Vector2& targetPos = targetEnemy_->GetPosition();

	for (auto& arrow : arrows_) {
		if (!arrow.active) {
			continue;
		}

		arrow.position.y += arrow.fallSpeed;

		if (arrow.position.y > targetPos.y + 50.0f) {
			arrow.active = false;
		}
	}

	if (timer_ >= kRainTime_) {
		phase_ = Phase::Finish;
	}
}

void ActionPlayerMagic::FinishMagic_() {
	if (targetEnemy_) {
		targetEnemy_->SetMagicLocked(false);
		targetEnemy_->TakeDamage(1.0f);
	}

	for (auto& arrow : arrows_) {
		arrow.active = false;
	}

	if (portalSprite_) {
		portalSprite_->SetColor({ 0.2f, 0.1f, 1.0f, 0.0f });
	}

	phase_ = Phase::Idle;
	targetEnemy_ = nullptr;
}

void ActionPlayerMagic::CreateSprite_(
	std::unique_ptr<TKM::Sprite>& sprite,
	const std::string& texturePath,
	const Vector2& size,
	const Vector4& color
) {
	TKM::TextureManager::GetInstance()->LoadTexture(texturePath);

	sprite = std::make_unique<TKM::Sprite>();

	sprite->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		texturePath
	);

	sprite->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath);

	Vector2 texSize = {
		static_cast<float>(meta.width),
		static_cast<float>(meta.height)
	};

	sprite->SetTextureLeftTop({ 0.0f, 0.0f });
	sprite->SetTextureSize(texSize);
	sprite->SetSize(size);
	sprite->SetColor(color);
}