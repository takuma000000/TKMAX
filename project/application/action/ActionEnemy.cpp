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
	if (isMagicVanishing_) {
		magicVanishTimer_ += kFrameTime_;

		float t = magicVanishTimer_ / kMagicVanishDuration_;
		if (t > 1.0f) {
			t = 1.0f;
		}

		float scale = 1.0f;
		Vector4 color = { 1, 1, 1, 1 };

		//=============================================================
		// 爆発演出
		//=============================================================
		if (t < 0.2f) {
			// ① ちょい縮む（溜め）
			scale = 1.0f - t * 0.5f;

		} else if (t < 0.5f) {
			// ② 一気に膨張
			float explodeT = (t - 0.2f) / 0.3f;
			scale = 0.9f + explodeT * 2.5f;

			// フラッシュ（白飛び）
			color = { 2.5f, 2.5f, 2.5f, 1.0f };

		} else {
			// ③ 余韻（少し暗くして消す）
			float fadeT = (t - 0.5f) / 0.5f;
			scale = 3.4f + fadeT * 0.5f;
			color = { 1.0f, 0.4f, 0.1f, 1.0f - fadeT };
		}

		Vector2 drawSize = {
			kEnemyWidth_ * scale,
			kEnemyHeight_ * scale
		};

		sprite_->SetSize(drawSize);
		sprite_->Update();

		if (magicVanishTimer_ >= kMagicVanishDuration_) {
			isDead_ = true;
		}

		return;
	}

	if (isMagicLocked_) {
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

	sprite_->Update();
}

void ActionEnemy::Draw(float scrollX) {
	if (!sprite_) {
		return;
	}

	if (isMagicVanishing_) {
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
		sprite_->Draw();
		return;
	}

	sprite_->SetPosition({ position_.x - scrollX, position_.y });
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->Draw();
}

void ActionEnemy::KillByMagic() {
	if (isDead_ || isDying_ || isMagicVanishing_) {
		return;
	}

	isMagicLocked_ = false;
	isMagicVanishing_ = true;
	magicVanishTimer_ = 0.0f;
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