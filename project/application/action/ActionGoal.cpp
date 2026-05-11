#include "ActionGoal.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionGoal::Initialize(TKM::DirectXCommon* dxCommon) {
	const std::string goalTexturePath = "./resources/texture/goal.png";
	const std::string closedTexturePath = "./resources/texture/mass.png";

	TKM::TextureManager::GetInstance()->LoadTexture(goalTexturePath);
	TKM::TextureManager::GetInstance()->LoadTexture(closedTexturePath);

	goalSprite_ = std::make_unique<TKM::Sprite>();
	goalSprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		goalTexturePath
	);

	goalSprite_->SetAutoAdjustTextureSize(false);

	const auto& goalMeta = TKM::TextureManager::GetInstance()->GetMetadata(goalTexturePath);

	Vector2 goalTexSize = {
		static_cast<float>(goalMeta.width),
		static_cast<float>(goalMeta.height)
	};

	goalSprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	goalSprite_->SetTextureSize(goalTexSize);
	goalSprite_->SetPosition(position_);
	goalSprite_->SetSize({ kGoalWidth_, kGoalHeight_ });
	goalSprite_->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });

	closedSprite_ = std::make_unique<TKM::Sprite>();
	closedSprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		closedTexturePath
	);

	closedSprite_->SetAutoAdjustTextureSize(false);

	const auto& closedMeta = TKM::TextureManager::GetInstance()->GetMetadata(closedTexturePath);

	Vector2 closedTexSize = {
		static_cast<float>(closedMeta.width),
		static_cast<float>(closedMeta.height)
	};

	closedSprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	closedSprite_->SetTextureSize(closedTexSize);
	closedSprite_->SetPosition(position_);
	closedSprite_->SetSize({ kGoalWidth_, kGoalHeight_ });
	closedSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
}

void ActionGoal::Update() {
	if (goalSprite_) {
		goalSprite_->SetPosition(position_);
		goalSprite_->SetSize({ kGoalWidth_, kGoalHeight_ });
		goalSprite_->Update();
	}

	if (closedSprite_) {
		closedSprite_->SetPosition(position_);
		closedSprite_->SetSize({ kGoalWidth_, kGoalHeight_ });
		closedSprite_->Update();
	}
}

void ActionGoal::Draw(float scrollX) {
	TKM::Sprite* drawSprite = isOpen_ ? goalSprite_.get() : closedSprite_.get();

	if (!drawSprite) {
		return;
	}

	drawSprite->SetPosition({ position_.x - scrollX, position_.y });
	drawSprite->Update();
	drawSprite->Draw();
}