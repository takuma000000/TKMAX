#include "ActionBack.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionBack::Initialize(TKM::DirectXCommon* dxCommon) {
	TKM::TextureManager::GetInstance()->LoadTexture(texturePath_);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath_);

	Vector2 texSize = {
		static_cast<float>(meta.width),
		static_cast<float>(meta.height)
	};

	for (int i = 0; i < kBackCount_; ++i) {
		sprites_[i] = std::make_unique<TKM::Sprite>();

		sprites_[i]->Initialize(
			TKM::SpriteCommon::GetInstance(),
			dxCommon,
			texturePath_
		);

		sprites_[i]->SetAutoAdjustTextureSize(false);
		const float kUvInset_ = 0.5f;

		sprites_[i]->SetTextureLeftTop({ kUvInset_, kUvInset_ });
		sprites_[i]->SetTextureSize({
			texSize.x - kUvInset_ * 2.0f,
			texSize.y - kUvInset_ * 2.0f
			});
		
		sprites_[i]->SetPosition({
			static_cast<float>(i) * kBackWidth_,
			0.0f
			});

		sprites_[i]->SetSize({ kBackWidth_, kBackHeight_ });
		sprites_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}
}

void ActionBack::Update() {
	for (auto& sprite : sprites_) {
		if (sprite) {
			sprite->Update();
		}
	}
}

void ActionBack::Draw(float scrollX) {
	const float backScrollX = scrollX * kParallaxRate_;

	for (int i = 0; i < kBackCount_; ++i) {
		if (!sprites_[i]) {
			continue;
		}

		Vector2 drawPosition = {
		static_cast<float>(i)* (kBackWidth_ - 1.0f) - backScrollX,
			0.0f
		};

		sprites_[i]->SetPosition(drawPosition);
		sprites_[i]->Update();
		sprites_[i]->Draw();
	}
}