#include "ActionGround.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionGround::Initialize(TKM::DirectXCommon* dxCommon) {
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
	sprite_->SetSize({ kGroundWidth_, kGroundHeight_ });
	sprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
}

void ActionGround::Update() {
	if (sprite_) {
		sprite_->SetPosition(position_);
		sprite_->SetSize({ kGroundWidth_, kGroundHeight_ });
		sprite_->Update();
	}
}

void ActionGround::Draw() {
	if (sprite_) {
		sprite_->Draw();
	}
}