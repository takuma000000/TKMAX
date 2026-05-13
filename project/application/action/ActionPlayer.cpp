#include "ActionPlayer.h"
#include "SpriteCommon.h"
#include "Input.h"
#include <cmath>
#include <cstdlib>

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

	// 走るときの砂埃エフェクト用スプライトの初期化
	runDustSprites_.reserve(kRunDustCount_);
	runDusts_.resize(kRunDustCount_);
	for (int i = 0; i < kRunDustCount_; ++i) {
		auto dustSprite = std::make_unique<TKM::Sprite>();
		dustSprite->Initialize(
			TKM::SpriteCommon::GetInstance(),
			dxCommon,
			"./resources/texture/circle2.png"
		);
		dustSprite->SetAutoAdjustTextureSize(false);
		dustSprite->SetTextureLeftTop({ 0.0f, 0.0f });
		dustSprite->SetTextureSize(texSize);
		dustSprite->SetColor({ 0.55f, 0.42f, 0.30f, 0.0f });
		runDustSprites_.push_back(std::move(dustSprite));
	}
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

	//=============================================================
	// 操作ロック中は移動・ジャンプ入力を受け付けない
	//=============================================================
	if (!isControlLocked_) {
		if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) {
			velocity_.x = -moveSpeed_;
			facingDirection_ = -1.0f;
		}

		if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) {
			velocity_.x = moveSpeed_;
			facingDirection_ = 1.0f;
		}

		if ((input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP)) && isGrounded_) {
			velocity_.y = jumpPower_;
			isGrounded_ = false;
		}
	}

	UpdateRunDust_();

	velocity_.y += gravity_;
	position_.x += velocity_.x;
	position_.y += velocity_.y;

	//=============================================================
	// ステージ外へ出ないように制限
	//=============================================================
	if (position_.x < 0.0f) {
		position_.x = 0.0f;
	}

	if (position_.x > stageWidth_ - kPlayerWidth_) {
		position_.x = stageWidth_ - kPlayerWidth_;
	}

	if (position_.y >= groundY_) {
		position_.y = groundY_;
		velocity_.y = 0.0f;
		isGrounded_ = true;
	}

	sprite_->SetPosition(position_);
	sprite_->SetSize({ kPlayerWidth_, kPlayerHeight_ });
	sprite_->Update();
}

void ActionPlayer::Draw(float scrollX) {
	sprite_->SetPosition({ position_.x - scrollX, position_.y });
	sprite_->Update();
	sprite_->Draw();

	for (size_t i = 0; i < runDustSprites_.size(); ++i) {
		if (!runDusts_[i].isActive) {
			continue;
		}

		runDustSprites_[i]->SetPosition({ runDusts_[i].position.x - scrollX, runDusts_[i].position.y });
		runDustSprites_[i]->Update();
		runDustSprites_[i]->Draw();
	}
}

void ActionPlayer::UpdateRunDust_() {
	const bool isRunningOnGround = isGrounded_ && std::abs(velocity_.x) > 0.01f;

	if (isRunningOnGround) {
		runDustSpawnTimer_ -= kFrameTime_;
		if (runDustSpawnTimer_ <= 0.0f) {
			SpawnRunDust_();
			runDustSpawnTimer_ = kRunDustSpawnInterval_;
		}
	} else {
		runDustSpawnTimer_ = 0.0f;
	}

	for (size_t i = 0; i < runDusts_.size(); ++i) {
		auto& dust = runDusts_[i];
		if (!dust.isActive) {
			continue;
		}

		dust.life -= kFrameTime_;
		if (dust.life <= 0.0f) {
			dust.isActive = false;
			runDustSprites_[i]->SetColor({ 0.55f, 0.42f, 0.30f, 0.0f });
			continue;
		}

		dust.position.x += dust.velocity.x;
		dust.position.y += dust.velocity.y;
		dust.velocity.y -= 0.03f;
		dust.velocity.x *= 0.92f;

		const float t = dust.life / dust.maxLife;
		const float size = dust.size * (1.3f - t * 0.3f);
		runDustSprites_[i]->SetPosition(dust.position);
		runDustSprites_[i]->SetSize({ size, size });
		runDustSprites_[i]->SetColor({ 0.55f, 0.42f, 0.30f, dust.alpha * t });
		runDustSprites_[i]->Update();
	}
}

void ActionPlayer::SpawnRunDust_() {
	for (auto& dust : runDusts_) {
		if (dust.isActive) {
			continue;
		}

		const float centerX = position_.x + kPlayerWidth_ * 0.5f;
		const float spawnJitterX = ((std::rand() % 11) - 5) * 0.6f;
		dust.position = { centerX + spawnJitterX, position_.y + kPlayerHeight_ - 2.0f };
		dust.velocity = { -facingDirection_ * (1.4f + (std::rand() % 45) * 0.01f), -0.7f - (std::rand() % 35) * 0.01f };
		dust.maxLife = kRunDustLifetime_ + (std::rand() % 10) * 0.005f;
		dust.life = dust.maxLife;
		dust.size = 10.0f + static_cast<float>(std::rand() % 8);
		dust.alpha = 0.7f;
		dust.isActive = true;
		break;
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