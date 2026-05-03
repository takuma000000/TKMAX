#include "ActionBlock.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionBlock::Initialize(TKM::DirectXCommon* dxCommon, const Vector2& position) {
	position_ = position;

	const std::string texturePath = "./resources/texture/block.png";
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
		(float)meta.width,
		(float)meta.height
	};

	sprite_->SetTextureLeftTop({ 0,0 });
	sprite_->SetTextureSize(texSize);

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kSize_, kSize_ });
}

void ActionBlock::Update() {
}

void ActionBlock::Draw(float scrollX) {
	sprite_->SetPosition({ position_.x - scrollX, position_.y });
	sprite_->Update();
	sprite_->Draw();
}