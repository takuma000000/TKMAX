#include "ActionPlayerBullet.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionPlayerBullet::Initialize(TKM::DirectXCommon* dxCommon, const Vector2& position, float direction) {
	position_ = position;
	isDead_ = false;

	velocity_.x = 12.0f * direction;

	const std::string texturePath = "./resources/texture/circle2.png";

	TKM::TextureManager::GetInstance()->LoadTexture(texturePath);

	sprite_ = std::make_unique<TKM::Sprite>();

	sprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		texturePath
	);

	sprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath);

	Vector2 texSize = {
		static_cast<float>(meta.width),
		static_cast<float>(meta.height)
	};

	sprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	sprite_->SetTextureSize(texSize);
	sprite_->SetPosition(position_);
	sprite_->SetSize({ kBulletWidth_, kBulletHeight_ });
	sprite_->SetColor({ 1.0f, 1.0f, 0.2f, 1.0f });
	sprite_->SetIsFlipX(direction < 0);
}

void ActionPlayerBullet::Update() {
	position_.x += velocity_.x;
	position_.y += velocity_.y;

	if (position_.x > kDeadX_) {
		isDead_ = true;
	}

	if (sprite_) {
		sprite_->SetPosition(position_);
		sprite_->SetSize({ kBulletWidth_, kBulletHeight_ });
		sprite_->Update();
	}
}

void ActionPlayerBullet::Draw(float scrollX) {
	if (!sprite_) {
		return;
	}

	sprite_->SetPosition({ position_.x - scrollX, position_.y });
	sprite_->Update();
	sprite_->Draw();
}

void ActionPlayerBullet::Kill() {
	isDead_ = true;
}

AABB ActionPlayerBullet::GetAABB() const {
	return AABB(
		{
			position_.x + kBulletWidth_ * 0.5f,
			position_.y + kBulletHeight_ * 0.5f,
			0.0f
		},
		{
			kBulletWidth_,
			kBulletHeight_,
			1.0f
		}
	);
}