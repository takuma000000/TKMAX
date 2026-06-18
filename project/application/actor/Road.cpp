#include "Road.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void Road::Initialize(
	TKM::DirectXCommon* dxCommon,
	const Vector2& pos,
	bool isCorrect
) {
	position_ = pos;
	isCorrect_ = isCorrect;

	const std::string tex =
		"./resources/texture/goal.png";

	sprite_ = std::make_unique<TKM::Sprite>();

	sprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		tex
	);

	sprite_->SetAutoAdjustTextureSize(false);

	const auto& meta =
		TKM::TextureManager::GetInstance()->GetMetadata(tex);

	sprite_->SetTextureLeftTop({ 0,0 });
	sprite_->SetTextureSize({
		(float)meta.width,
		(float)meta.height
		});

	sprite_->SetSize(size_);
	sprite_->SetPosition(position_);

	if (isCorrect_) {
		sprite_->SetColor({ 0,0,1,1 });
	}

	sprite_->Update();
}

void Road::Update() {
	if (isHit_) {
		sprite_->SetColor({ 1.0f, 0.3f, 0.3f, 1.0f });
	} else if (isCorrect_) {
		sprite_->SetColor({ 0.0f, 0.0f, 1.0f, 1.0f });
	} else {
		sprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}

	sprite_->Update();
}

void Road::Draw() {
	sprite_->Draw();
}