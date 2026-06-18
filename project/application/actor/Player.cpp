#include "Player.h"
#include "Input.h"
#include "SpriteCommon.h"

void Player::Initialize(TKM::DirectXCommon* dxCommon) {
	const std::string playerTex = "./resources/texture/circle2.png";

	sprite_ = std::make_unique<TKM::Sprite>();

	sprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		playerTex
	);

	sprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(playerTex);
	sprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	sprite_->SetTextureSize({ (float)meta.width, (float)meta.height });

	sprite_->SetSize({ 64.0f, 64.0f });
	sprite_->SetPosition(position_);
	sprite_->Update();
}

void Player::Update() {
	auto input = TKM::Input::GetInstance();

	if (input->PushKey(DIK_A)) {
		position_.x -= speed_;
	}
	if (input->PushKey(DIK_D)) {
		position_.x += speed_;
	}
	if (input->PushKey(DIK_W)) {
		position_.y -= speed_;
	}
	if (input->PushKey(DIK_S)) {
		position_.y += speed_;
	}

	sprite_->SetPosition(position_);
	sprite_->Update();
}

void Player::Draw() {
	sprite_->Draw();
}