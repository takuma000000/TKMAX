#pragma once
#include "engine/2d/Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "WindowsAPI.h"
#include "Input.h"
#include <algorithm>
#include <cmath>

class Reticle {
public:
	using Vec2 = Sprite::Vector2; // Sprite の 2D ベクトルを採用

	Reticle() = default;
	~Reticle() = default;

	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx,
		const char* texPath = "./resources/reticle.png") {
		sprite_ = std::make_unique<Sprite>();
		sprite_->Initialize(spriteCommon, dx, texPath);
		sprite_->SetAnchorPoint({ 0.5f, 0.5f });
		sprite_->SetSize(size_);
		sprite_->SetPosition(position_);
	}

	void Update(float dt) {
		Input* input = Input::GetInstance();

		float rx = static_cast<float>(input->GetRightStickX());
		float ry = static_cast<float>(input->GetRightStickY());

		if (std::fabs(rx) < deadZone_) rx = 0.0f;
		if (std::fabs(ry) < deadZone_) ry = 0.0f;

		Vec2 v{ rx / 32768.0f, -(ry / 32768.0f) };
		position_.x += v.x * speedPixelPerSec_ * dt;
		position_.y += v.y * speedPixelPerSec_ * dt;

		position_.x = std::clamp(position_.x, 0.0f, static_cast<float>(WindowsAPI::kClientWidth));
		position_.y = std::clamp(position_.y, 0.0f, static_cast<float>(WindowsAPI::kClientHeight));

		angleRad_ += angularSpeedRadPerSec_ * dt;
		sprite_->SetRotation(angleRad_);

		sprite_->SetPosition(position_);
		sprite_->SetSize(size_);
		sprite_->Update();
	}

	void Draw() {
		if (!visible_) return;
		sprite_->Draw();
	}

	// ---- setters / getters ----
	void SetPosition(const Vec2& p) { position_ = p; if (sprite_) sprite_->SetPosition(p); }
	const Vec2& GetPosition() const { return position_; }

	void SetSize(const Vec2& s) { size_ = s; if (sprite_) sprite_->SetSize(s); }
	void SetSpeed(float pixelsPerSec) { speedPixelPerSec_ = pixelsPerSec; }
	void SetDeadZone(float v) { deadZone_ = v; }                 // 例: 3000〜8000
	void SetAngularSpeed(float radPerSec) { angularSpeedRadPerSec_ = radPerSec; }
	void SetVisible(bool v) { visible_ = v; }

private:
	std::unique_ptr<Sprite> sprite_;
	Vec2  position_{ static_cast<float>(WindowsAPI::kClientWidth) * 0.5f,
					 static_cast<float>(WindowsAPI::kClientHeight) * 0.5f };
	Vec2  size_{ 50.0f, 50.0f };
	float speedPixelPerSec_ = 900.0f;
	float deadZone_ = 5000.0f;
	float angleRad_ = 0.0f;
	float angularSpeedRadPerSec_ = 0.0f;
	bool  visible_ = true;
};