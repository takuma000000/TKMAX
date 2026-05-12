#include "ActionEnemy.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include <cmath>
#include <algorithm>
#include <random>

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
	baseMoveSpeed_ = moveSpeed;
	moveSpeed_ = moveSpeed;
	direction_ = 1.0f;
	randomEngine_.seed(std::random_device{}());
	typeARandomTimer_ = 0.0f;
	typeARandomInterval_ = 0.0f;

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

	dropTexturePath_ = "./resources/texture/circle2.png";

	TKM::TextureManager::GetInstance()->LoadTexture(dropTexturePath_);

	const auto& dropMeta = TKM::TextureManager::GetInstance()->GetMetadata(dropTexturePath_);

	Vector2 dropTexSize = {
		static_cast<float>(dropMeta.width),
		static_cast<float>(dropMeta.height)
	};

	dropObjects_.resize(kDropMax_);

	dropSprites_.resize(kDropMax_);
	for (int i = 0; i < kDropMax_; ++i) {
		dropSprites_[i] = std::make_unique<TKM::Sprite>();
		dropSprites_[i]->Initialize(
			TKM::SpriteCommon::GetInstance(),
			dxCommon,
			dropTexturePath_
		);
		dropSprites_[i]->SetAutoAdjustTextureSize(false);
		dropSprites_[i]->SetTextureLeftTop({ 0.0f, 0.0f });
		dropSprites_[i]->SetTextureSize(dropTexSize);
		dropSprites_[i]->SetSize({ kDropSize_, kDropSize_ });
		dropSprites_[i]->SetColor({ 1.0f, 0.8f, 0.2f, 1.0f });
	}

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

	for (size_t i = 0; i < dropObjects_.size(); ++i) {
		auto& drop = dropObjects_[i];
		if (!drop.isActive || !dropSprites_[i]) {
			continue;
		}

		dropSprites_[i]->SetPosition({ drop.position.x - scrollX, drop.position.y });
		dropSprites_[i]->SetSize({ kDropSize_, kDropSize_ });
		dropSprites_[i]->Update();
		dropSprites_[i]->Draw();
	}
}

std::string ActionEnemy::GetTexturePathByType_(EnemyType type) {
	switch (type) {
	case EnemyType::TypeA:
		return "./resources/texture/enemy_typeA.png";

	case EnemyType::TypeB:
		return "./resources/texture/enemy_typeB.png";

	case EnemyType::TypeC:
		return "./resources/texture/circle2.png";

	default:
		return "./resources/texture/circle2.png";
	}
}

void ActionEnemy::UpdateNormal_() {
	if (type_ == EnemyType::TypeB) {
		UpdateTypeB_();
		return;
	}

	if (type_ == EnemyType::TypeC) {
		UpdateTypeC_();
		return;
	}

	if (isMagicLocked_) {
		sprite_->SetPosition(position_);
		sprite_->Update();
		return;
	}

	typeARandomTimer_ += kFrameTime_;
	if (typeARandomTimer_ >= typeARandomInterval_) {
		typeARandomTimer_ = 0.0f;
		std::uniform_real_distribution<float> intervalDist(0.25f, 0.9f);
		std::uniform_real_distribution<float> speedScaleDist(0.65f, 1.5f);
		std::bernoulli_distribution flipDist(0.35);
		typeARandomInterval_ = intervalDist(randomEngine_);
		if (flipDist(randomEngine_)) {
			direction_ *= -1.0f;
		}
		moveSpeed_ = std::clamp(baseMoveSpeed_ * speedScaleDist(randomEngine_), 0.8f, 4.0f);
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

void ActionEnemy::StartMagicExplosionDeath(float scrollX) {
	if (isDead_ || deathType_ != EnemyDeathType::None) {
		return;
	}

	isMagicLocked_ = false;
	deathType_ = EnemyDeathType::MagicExplosion;
	deathTimer_ = 0.0f;

	const Vector3 deathCenter = {
		(position_.x - scrollX) * 0.01f - 6.4f + kEnemyWidth_ * 0.005f,
		-(position_.y * 0.01f) + 3.6f - kEnemyHeight_ * 0.005f,
		0.0f
	};

	TKM::ParticleManager* particleManager = TKM::ParticleManager::GetInstance();
	particleManager->Emit("enemyDeath_core", deathCenter, 18);
	particleManager->Emit("enemyDeath_shard", deathCenter, 48);
	particleManager->Emit("enemyDeath_smoke", deathCenter, 20);
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

void ActionEnemy::UpdateTypeC_() {
	if (isMagicLocked_) {
		sprite_->SetPosition(position_);
		sprite_->Update();
		return;
	}

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

	const float enemyLeft = position_.x;
	const float enemyRight = position_.x + kEnemyWidth_;
	const float screenRight = screenLeft_ + screenWidth_;
	const bool isOnScreen = !(enemyRight < screenLeft_ || enemyLeft > screenRight);
	if (isOnScreen && dropTimer_ >= kTypeCBulletInterval_) {
		dropTimer_ = 0.0f;
		SpawnTypeCBullet_();
	}

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->SetColor({ 1.0f, 0.45f, 0.45f, 1.0f });
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

		drop.lifeTimer = 0.0f;
		drop.isActive = true;
		return;
	}
}

void ActionEnemy::SpawnTypeCBullet_() {
	for (auto& drop : dropObjects_) {
		if (drop.isActive) {
			continue;
		}

		drop.position = {
			position_.x + kEnemyWidth_ * 0.5f - kDropSize_ * 0.5f,
			position_.y + kEnemyHeight_ * 0.35f
		};

		Vector2 sourceCenter = {
			drop.position.x + kDropSize_ * 0.5f,
			drop.position.y + kDropSize_ * 0.5f
		};

		Vector2 targetCenter = {
			targetPosition_.x + kEnemyWidth_ * 0.5f,
			targetPosition_.y + kEnemyHeight_ * 0.5f
		};

		Vector2 toTarget = {
			targetCenter.x - sourceCenter.x,
			targetCenter.y - sourceCenter.y
		};

		float length = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
		if (length > 0.001f) {
			toTarget.x /= length;
			toTarget.y /= length;
		} else {
			toTarget = { -1.0f, 0.0f };
		}

		drop.velocity = {
			toTarget.x * kTypeCBulletSpeed_,
			toTarget.y * kTypeCBulletSpeed_
		};

		drop.lifeTimer = 0.0f;
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
		drop.lifeTimer += kFrameTime_;

		drop.velocity.y += kDropGravity_;

		if (type_ == EnemyType::TypeC) {
			if (drop.lifeTimer >= kTypeCBulletLifetime_) {
				drop.isActive = false;
				continue;
			}

			Vector2 bulletCenter = {
				drop.position.x + kDropSize_ * 0.5f,
				drop.position.y + kDropSize_ * 0.5f
			};

			Vector2 targetCenter = {
				targetPosition_.x + kEnemyWidth_ * 0.5f,
				targetPosition_.y + kEnemyHeight_ * 0.5f
			};

			Vector2 toTarget = {
				targetCenter.x - bulletCenter.x,
				targetCenter.y - bulletCenter.y
			};

			float length = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
			if (length > 0.001f) {
				toTarget.x /= length;
				toTarget.y /= length;
				drop.velocity.x = toTarget.x * kTypeCBulletSpeed_;
				drop.velocity.y = toTarget.y * kTypeCBulletSpeed_;
			}
		}

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