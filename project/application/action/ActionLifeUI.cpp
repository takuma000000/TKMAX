#include "ActionLifeUI.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionLifeUI::Initialize(TKM::DirectXCommon* dxCommon) {
	TKM::TextureManager::GetInstance()->LoadTexture(lifeTexturePath_);
	TKM::TextureManager::GetInstance()->LoadTexture(lifeOutTexturePath_);

	const auto& lifeMeta = TKM::TextureManager::GetInstance()->GetMetadata(lifeTexturePath_);
	const auto& lifeOutMeta = TKM::TextureManager::GetInstance()->GetMetadata(lifeOutTexturePath_);

	Vector2 lifeTextureSize = {
		static_cast<float>(lifeMeta.width),
		static_cast<float>(lifeMeta.height)
	};

	Vector2 lifeOutTextureSize = {
		static_cast<float>(lifeOutMeta.width),
		static_cast<float>(lifeOutMeta.height)
	};

	for (int i = 0; i < kMaxLife_; ++i) {
		Vector2 position = {
			basePosition_.x + static_cast<float>(i) * (kLifeWidth_ + kLifeSpacing_),
			basePosition_.y
		};

		lifeSprites_[i] = std::make_unique<TKM::Sprite>();
		lifeSprites_[i]->Initialize(
			TKM::SpriteCommon::GetInstance(),
			dxCommon,
			lifeTexturePath_
		);
		lifeSprites_[i]->SetAutoAdjustTextureSize(false);
		lifeSprites_[i]->SetTextureLeftTop({ 0.0f, 0.0f });
		lifeSprites_[i]->SetTextureSize(lifeTextureSize);
		lifeSprites_[i]->SetPosition(position);
		lifeSprites_[i]->SetSize({ kLifeWidth_, kLifeHeight_ });
		lifeSprites_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

		lifeOutSprites_[i] = std::make_unique<TKM::Sprite>();
		lifeOutSprites_[i]->Initialize(
			TKM::SpriteCommon::GetInstance(),
			dxCommon,
			lifeOutTexturePath_
		);
		lifeOutSprites_[i]->SetAutoAdjustTextureSize(false);
		lifeOutSprites_[i]->SetTextureLeftTop({ 0.0f, 0.0f });
		lifeOutSprites_[i]->SetTextureSize(lifeOutTextureSize);
		lifeOutSprites_[i]->SetPosition(position);
		lifeOutSprites_[i]->SetSize({ kLifeWidth_, kLifeHeight_ });
		lifeOutSprites_[i]->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	}
}

void ActionLifeUI::Update(int currentHP) {
	for (int i = 0; i < kMaxLife_; ++i) {
		bool isAliveHeart = i < currentHP;

		if (lifeSprites_[i]) {
			lifeSprites_[i]->SetColor({
				1.0f,
				1.0f,
				1.0f,
				isAliveHeart ? 1.0f : 0.0f
				});
			lifeSprites_[i]->Update();
		}

		if (lifeOutSprites_[i]) {
			lifeOutSprites_[i]->SetColor({
				1.0f,
				1.0f,
				1.0f,
				isAliveHeart ? 0.0f : 1.0f
				});
			lifeOutSprites_[i]->Update();
		}
	}
}

void ActionLifeUI::Draw() {
	for (int i = 0; i < kMaxLife_; ++i) {
		if (lifeOutSprites_[i]) {
			lifeOutSprites_[i]->Draw();
		}

		if (lifeSprites_[i]) {
			lifeSprites_[i]->Draw();
		}
	}
}