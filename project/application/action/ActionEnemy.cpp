#include "ActionEnemy.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include <cmath>

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

	dropTexturePath_ = "./resources/texture/circle2.png";

	TKM::TextureManager::GetInstance()->LoadTexture(dropTexturePath_);

	dropSprite_ = std::make_unique<TKM::Sprite>();

	dropSprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		dropTexturePath_
	);

	dropSprite_->SetAutoAdjustTextureSize(false);

	const auto& dropMeta = TKM::TextureManager::GetInstance()->GetMetadata(dropTexturePath_);

	Vector2 dropTexSize = {
		static_cast<float>(dropMeta.width),
		static_cast<float>(dropMeta.height)
	};

	dropSprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	dropSprite_->SetTextureSize(dropTexSize);
	dropSprite_->SetSize({ kDropSize_, kDropSize_ });
	dropSprite_->SetColor({ 1.0f, 0.8f, 0.2f, 1.0f });

	dropObjects_.resize(kDropMax_);
	floatTimer_ = 0.0f;
	dropTimer_ = 0.0f;
	dropIndex_ = 0;
}

void ActionEnemy::Update() {
	switch (deathType_) {
	case EnemyDeathType::Knockback:
		UpdateKnockbackDeath_();
		break;

	case EnemyDeathType::MagicExplosion:
		UpdateMagicExplosionDeath_();
		break;

	case EnemyDeathType::None:
	default:
		UpdateNormal_();
		break;
	}

	UpdateDropObjects_();
}

void ActionEnemy::Draw(float scrollX) {
	if (!sprite_) {
		return;
	}

	if (deathType_ == EnemyDeathType::MagicExplosion) {
		Vector2 drawSize = sprite_->GetSize();

		Vector2 center = {
			position_.x + kEnemyWidth_ * 0.5f,
			position_.y + kEnemyHeight_ * 0.5f
		};

		Vector2 drawPosition = {
			center.x - drawSize.x * 0.5f - scrollX,
			center.y - drawSize.y * 0.5f
		};

		sprite_->SetPosition(drawPosition);
		sprite_->Update();
		sprite_->Draw();
	} else {
		sprite_->SetPosition({ position_.x - scrollX, position_.y });
		sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
		sprite_->Update();
		sprite_->Draw();
	}

	for (auto& drop : dropObjects_) {
		if (!drop.isActive) {
			continue;
		}

		if (!dropSprite_) {
			continue;
		}

		dropSprite_->SetPosition({ drop.position.x - scrollX, drop.position.y });
		dropSprite_->SetSize({ kDropSize_, kDropSize_ });
		dropSprite_->SetColor({ 1.0f, 0.8f, 0.2f, 1.0f });
		dropSprite_->Update();
		dropSprite_->Draw();
	}
}

std::string ActionEnemy::GetTexturePathByType_(EnemyType type) {
	switch (type) {
	case EnemyType::TypeA:
		return "./resources/texture/circle2.png";

	case EnemyType::TypeB:
		return "./resources/texture/circle2.png";

	case EnemyType::TypeC:
		return "./resources/texture/enemy_c.png";

	default:
		return "./resources/texture/circle2.png";
	}
}

void ActionEnemy::UpdateNormal_() {
	if (type_ == EnemyType::TypeB) {
		UpdateTypeB_();
		return;
	}

	if (isMagicLocked_) {
		sprite_->SetPosition(position_);
		sprite_->Update();
		return;
	}

	position_.x += moveSpeed_ * direction_;

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

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
	sprite_->Update();
}

void ActionEnemy::UpdateKnockbackDeath_() {
	position_.x += knockbackVelocity_.x;
	position_.y += knockbackVelocity_.y;

	knockbackVelocity_.y += kKnockbackGravity_;

	if (position_.y > kDeadBottomY_) {
		isDead_ = true;
	}

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
	sprite_->Update();
}

void ActionEnemy::UpdateMagicExplosionDeath_() {
	deathTimer_ += kFrameTime_;

	float t = deathTimer_ / kMagicVanishDuration_;

	if (t > 1.0f) {
		t = 1.0f;
	}

	float scale = 1.0f;
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	if (t < 0.2f) {
		scale = 1.0f - t * 0.5f;

	} else if (t < 0.5f) {
		float explodeT = (t - 0.2f) / 0.3f;
		scale = 0.9f + explodeT * 2.5f;
		color = { 2.5f, 2.5f, 2.5f, 1.0f };

	} else {
		float fadeT = (t - 0.5f) / 0.5f;
		scale = 3.4f + fadeT * 0.5f;
		color = { 1.0f, 0.4f, 0.1f, 1.0f - fadeT };
	}

	Vector2 drawSize = {
		kEnemyWidth_ * scale,
		kEnemyHeight_ * scale
	};

	sprite_->SetSize(drawSize);
	sprite_->SetColor(color);
	sprite_->Update();

	if (deathTimer_ >= kMagicVanishDuration_) {
		isDead_ = true;
	}
}

void ActionEnemy::SetActionOffset(float actionOffset) {
	floatTimer_ = actionOffset;
	dropTimer_ = actionOffset;
}

void ActionEnemy::StartKnockbackDeath(float hitDirection) {
	if (isDead_ || deathType_ != EnemyDeathType::None) {
		return;
	}

	isMagicLocked_ = false;
	deathType_ = EnemyDeathType::Knockback;

	knockbackVelocity_ = {
		kKnockbackSpeedX_ * hitDirection,
		kKnockbackSpeedY_
	};
}

void ActionEnemy::StartMagicExplosionDeath() {
	if (isDead_ || deathType_ != EnemyDeathType::None) {
		return;
	}

	isMagicLocked_ = false;
	deathType_ = EnemyDeathType::MagicExplosion;
	deathTimer_ = 0.0f;
}

void ActionEnemy::UpdateTypeB_() {
	if (isMagicLocked_) {
		sprite_->SetPosition(position_);
		sprite_->Update();
		return;
	}

	floatTimer_ += kTypeBFloatSpeed_;
	dropTimer_ += kFrameTime_;

	position_.x += moveSpeed_ * direction_;

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

	position_.y = basePosition_.y + std::sin(floatTimer_) * kTypeBFloatRange_;

	if (dropTimer_ >= kDropInterval_) {
		dropTimer_ = 0.0f;
		SpawnDropObject_();
	}

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->SetColor({ 0.3f, 0.6f, 1.0f, 1.0f });
	sprite_->Update();
}

void ActionEnemy::SpawnDropObject_() {
	for (auto& drop : dropObjects_) {
		if (drop.isActive) {
			continue;
		}

		const float directions[] = {
			-1.0f,
			1.0f
		};

		const float throwDirection = directions[dropIndex_ % 2];
		dropIndex_++;

		drop.position = {
			position_.x + kEnemyWidth_ * 0.5f - kDropSize_ * 0.5f,
			position_.y + kEnemyHeight_ * 0.5f
		};

		drop.velocity = {
			kDropThrowSpeedX_ * throwDirection,
			kDropThrowSpeedY_
		};

		drop.isActive = true;
		return;
	}
}

void ActionEnemy::UpdateDropObjects_() {
	for (auto& drop : dropObjects_) {
		if (!drop.isActive) {
			continue;
		}

		drop.position.x += drop.velocity.x;
		drop.position.y += drop.velocity.y;

		drop.velocity.y += kDropGravity_;

		if (drop.position.y > kDropBottomY_) {
			drop.isActive = false;
		}
	}
}

AABB ActionEnemy::GetDropObjectAABB_(const DropObject& drop) const {
	return AABB(
		{
			drop.position.x + kDropSize_ * 0.5f,
			drop.position.y + kDropSize_ * 0.5f,
			0.0f
		},
		{
			kDropSize_,
			kDropSize_,
			1.0f
		}
	);
}

bool ActionEnemy::IsHitAttack(const AABB& playerAABB) {
	for (auto& drop : dropObjects_) {
		if (!drop.isActive) {
			continue;
		}

		if (playerAABB.IsCollidingWithAABB(GetDropObjectAABB_(drop))) {
			drop.isActive = false;
			return true;
		}
	}

	return false;
}