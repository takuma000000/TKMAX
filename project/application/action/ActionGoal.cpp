#include "ActionGoal.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionGoal::Initialize(TKM::DirectXCommon* dxCommon) {
	sprite_ = std::make_unique<TKM::Sprite>();

	sprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		"./resources/texture/circle2.png"
	);

	sprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata("./resources/texture/circle2.png");
	Vector2 texSize = { (float)meta.width, (float)meta.height };

	sprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	sprite_->SetTextureSize(texSize);

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kGoalWidth_, kGoalHeight_ });
	sprite_->SetColor({ 1.0f, 1.0f, 0.0f, 1.0f });
}

void ActionGoal::Update() {
	sprite_->SetPosition(position_);
	sprite_->SetSize({ kGoalWidth_, kGoalHeight_ });
	sprite_->Update();
}

void ActionGoal::Draw(float scrollX) {
	if (sprite_) {
		sprite_->SetPosition({ position_.x - scrollX, position_.y });
		sprite_->Update();
		sprite_->Draw();
	}
}