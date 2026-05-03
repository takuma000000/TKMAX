#include "ActionEnemy.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void ActionEnemy::Initialize(TKM::DirectXCommon* dxCommon) {
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
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->SetColor({ 1.0f, 0.2f, 0.2f, 1.0f });
}

void ActionEnemy::Update() {
	sprite_->SetPosition(position_);
	sprite_->SetSize({ kEnemyWidth_, kEnemyHeight_ });
	sprite_->Update();
}

void ActionEnemy::Draw() {
	if (sprite_) {
		sprite_->Draw();
	}
}