#include "ActionGround.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionGround::Initialize(TKM::DirectXCommon* dxCommon) {
	TKM::TextureManager::GetInstance()->LoadTexture(texturePath_);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath_);

	Vector2 texSize = {
		static_cast<float>(meta.width),
		static_cast<float>(meta.height)
	};

	for (int i = 0; i < kGroundCount_; ++i) {
		sprites_[i] = std::make_unique<TKM::Sprite>();

		sprites_[i]->Initialize(
			TKM::SpriteCommon::GetInstance(),
			dxCommon,
			texturePath_
		);

		sprites_[i]->SetAutoAdjustTextureSize(false);
		sprites_[i]->SetTextureLeftTop({ 0.0f, 0.0f });
		sprites_[i]->SetTextureSize(texSize);

		sprites_[i]->SetPosition({
			position_.x + static_cast<float>(i) * kGroundWidth_,
			position_.y
			});

		sprites_[i]->SetSize({ kGroundWidth_, kGroundHeight_ });
		sprites_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

void ActionGround::Update() {
	for (auto& sprite : sprites_) {
		if (sprite) {
			sprite->Update();
		}
	}
}

void ActionGround::Draw(float scrollX) {
	for (int i = 0; i < kGroundCount_; ++i) {
		if (!sprites_[i]) {
			continue;
		}

		Vector2 drawPosition = {
			position_.x + static_cast<float>(i) * kGroundWidth_ - scrollX,
			position_.y
		};

		sprites_[i]->SetPosition(drawPosition);
		sprites_[i]->Update();
		sprites_[i]->Draw();
	}
}