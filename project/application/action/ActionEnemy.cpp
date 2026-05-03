#include "ActionEnemy.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionEnemy::Initialize(
	TKM::DirectXCommon* dxCommon,
	const Vector2& position,
	EnemyType type,
	float moveRange,
	float moveSpeed
) {
	position_ = position;
	basePosition_ = position;
	type_ = type;
	moveRange_ = moveRange;
	moveSpeed_ = moveSpeed;
	direction_ = 1.0f;

	texturePath_ = GetTexturePathByType_(type_);

	TKM::TextureManager::GetInstance()->LoadTexture(texturePath_);

	sprite_ = std::make_unique<TKM::Sprite>();

	sprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		texturePath_
	);

	sprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath_);

	Vector2 texSize = {
		static_cast<float>(meta.width),
		static_cast<float>(meta.height)
	};

	sprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	sprite_->SetTextureSize(texSize);
	sprite_->SetPosition(position_);
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
}

void ActionEnemy::Update() {
	if (isMagicLocked_) {
		if (sprite_) {
			sprite_->SetPosition(position_);
			sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
			sprite_->Update();
		}
		return;
	}

	position_.x += moveSpeed_ * direction_;

	if (isDying_) {
		position_.x += knockbackVelocity_.x;
		position_.y += knockbackVelocity_.y;

		knockbackVelocity_.y += kKnockbackGravity_;

		if (position_.y > kDeadBottomY_) {
			isDead_ = true;
		}

		if (sprite_) {
			sprite_->SetPosition(position_);
			sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
			sprite_->Update();
		}

		return;
	}

	const float leftLimit = basePosition_.x - moveRange_;
	const float rightLimit = basePosition_.x + moveRange_;
	if (position_.x <= leftLimit) {
		position_.x = leftLimit;
		direction_ = 1.0f;
	}
	if (position_.x >= rightLimit) {
		position_.x = rightLimit;
		direction_ = -1.0f;
	}

	if (sprite_) {
		sprite_->SetPosition(position_);
		sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
		sprite_->Update();
	}
}

void ActionEnemy::Draw(float scrollX) {
	if (!sprite_) {
		return;
	}

	sprite_->SetPosition({ position_.x - scrollX, position_.y });
	sprite_->Update();
	sprite_->Draw();
}

void ActionEnemy::TakeDamage(float hitDirection) {
	if (isDying_ || isDead_) {
		return;
	}

	isDying_ = true;

	knockbackVelocity_ = {
		kKnockbackSpeedX_ * hitDirection,
		kKnockbackSpeedY_
	};
}

std::string ActionEnemy::GetTexturePathByType_(EnemyType type) {
	switch (type) {
	case EnemyType::TypeA:
		return "./resources/texture/circle2.png";

	case EnemyType::TypeB:
		return "./resources/texture/enemy_b.png";

	case EnemyType::TypeC:
		return "./resources/texture/enemy_c.png";

	default:
		return "./resources/texture/circle2.png";
	}
}