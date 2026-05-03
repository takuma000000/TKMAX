#include "ActionPlayer.h"
#include "SpriteCommon.h"
#include "Input.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void ActionPlayer::Initialize(TKM::DirectXCommon* dxCommon) {
	sprite_ = std::make_unique<TKM::Sprite>();

	sprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon,
		"./resources/texture/circle2.png"
	);

	sprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata("./resources/texture/circle2.png");

	Vector2 texSize = {
		(float)meta.width,
		(float)meta.height
	};

	sprite_->SetTextureLeftTop({ 0.0f, 0.0f });
	sprite_->SetTextureSize(texSize);

	// 表示
	sprite_->SetPosition(position_);
	sprite_->SetSize({ kPlayerWidth_, kPlayerHeight_ });
	sprite_->SetColor({ 0.0f, 1.0f, 1.0f, 1.0f });
}

void ActionPlayer::Update() {
	auto* input = TKM::Input::GetInstance();

	if (damageCooldownTimer_ > 0.0f) {
		damageCooldownTimer_ -= kFrameTime_;

		if (damageCooldownTimer_ < 0.0f) {
			damageCooldownTimer_ = 0.0f;
		}
	}

	//=============================================================
	// 無敵中の点滅
	//=============================================================
	if (damageCooldownTimer_ > 0.0f) {

		// 0.1秒ごとにON/OFF
		int blink = static_cast<int>(damageCooldownTimer_ / kBlinkInterval_) % 2;

		if (blink == 0) {
			sprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.3f }); // 半透明
		} else {
			sprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 通常
		}

	} else {
		// 無敵じゃないときは普通に戻す
		sprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	}


	velocity_.x = 0.0f;

	if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) {
		velocity_.x = -moveSpeed_;
	}

	if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) {
		velocity_.x = moveSpeed_;
	}

	if ((input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP)) && isGrounded_) {
		velocity_.y = jumpPower_;
		isGrounded_ = false;
	}

	velocity_.y += gravity_;
	position_.x += velocity_.x;
	position_.y += velocity_.y;

	if (position_.y >= groundY_) {
		position_.y = groundY_;
		velocity_.y = 0.0f;
		isGrounded_ = true;
	}

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kPlayerWidth_, kPlayerHeight_ });
	sprite_->Update();
}

void ActionPlayer::Draw() {
	if (sprite_) {
		sprite_->Draw();
	}
}

void ActionPlayer::ImGuiDebug() {
#ifdef USE_IMGUI
	ImGui::Begin("ActionPlayer");

	ImGui::Text("Position : %.1f, %.1f", position_.x, position_.y);
	ImGui::Text("Velocity : %.1f, %.1f", velocity_.x, velocity_.y);
	ImGui::Text("Grounded : %s", isGrounded_ ? "true" : "false");

	ImGui::DragFloat2("Position", &position_.x, 1.0f);
	ImGui::DragFloat("Move Speed", &moveSpeed_, 0.1f, 0.0f, 30.0f);
	ImGui::DragFloat("Jump Power", &jumpPower_, 0.1f, -50.0f, 0.0f);
	ImGui::DragFloat("Gravity", &gravity_, 0.01f, 0.0f, 5.0f);
	ImGui::DragFloat("Ground Y", &groundY_, 1.0f, 0.0f, 720.0f);

	ImGui::End();
#endif
}

void ActionPlayer::TakeDamage() {
	if (!CanTakeDamage()) {
		return;
	}

	if (hp_ <= 0) {
		return;
	}

	--hp_;
	damageCooldownTimer_ = kDamageCooldownSec_;
}