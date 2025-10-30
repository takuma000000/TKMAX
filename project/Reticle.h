#pragma once
#define NOMINMAX
#include <memory>
#include <algorithm>
#include <cmath>
#include "engine/2d/Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "WindowsAPI.h"
#include "Input.h"
#include "externals/imgui/imgui.h"

class Reticle {
public:
	using Vec2 = Sprite::Vector2;

	Reticle() = default;
	~Reticle() = default;

	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dx,
		const char* texPath = "./resources/reticle.png")
	{
		for (int i = 0; i < 3; ++i) {
			layers_[i] = std::make_unique<Sprite>();
			layers_[i]->Initialize(spriteCommon, dx, texPath);
			layers_[i]->SetAnchorPoint({ 0.5f, 0.5f });
			layers_[i]->SetSize(size_);
			layers_[i]->SetPosition(position_);
		}
	}

	void Update(float dt)
	{
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
		time_ += dt;

		// 全レイヤーに共通設定を適用
		for (int i = 0; i < 3; ++i) {
			layers_[i]->SetPosition(position_);
			layers_[i]->SetSize(size_);
			layers_[i]->SetRotation(angleRad_ * (i == 1 ? 1.0f : (i == 2 ? -1.0f : 0.5f)));
		}

		// ==== 虹色発光処理 ====
		if (rainbow_) {
			// ベース白
			layers_[0]->SetColor({ 1.0f, 1.0f, 1.0f, 0.7f });

			// レイヤー1（順方向のHue回転）
			float r1, g1, b1;
			HSVtoRGB(fmodf(time_ * hueSpeed_ * 1.0f, 1.0f), sat_, val_, r1, g1, b1);
			layers_[1]->SetColor({ r1, g1, b1, 0.6f });

			// レイヤー2（逆方向＋明滅）
			float pulse = 1.0f + pulseAmp_ * std::sin(2.0f * 3.14159265f * pulseFreq_ * time_);
			float r2, g2, b2;
			HSVtoRGB(fmodf(1.0f - time_ * hueSpeed_ * 0.8f, 1.0f), sat_, val_ * pulse, r2, g2, b2);
			layers_[2]->SetColor({ r2, g2, b2, 0.5f });
		}

		for (auto& sp : layers_) sp->Update();
	}

	void Draw()
	{
		if (!visible_) return;
		for (auto& sp : layers_) sp->Draw();
	}

	

	// ==== setters ====
	void SetPosition(const Vec2& p) { position_ = p; for (auto& sp : layers_) if (sp) sp->SetPosition(p); }
	void SetSize(const Vec2& s) { size_ = s; for (auto& sp : layers_) if (sp) sp->SetSize(s); }
	void SetAngularSpeed(float radPerSec) { angularSpeedRadPerSec_ = radPerSec; }
	void SetVisible(bool v) { visible_ = v; }
	void EnableRainbow(bool e = true) { rainbow_ = e; }
	void SetHueSpeed(float hps) { hueSpeed_ = hps; }
	void SetSaturation(float s) { sat_ = std::clamp(s, 0.f, 1.f); }
	void SetValue(float v) { val_ = std::max(0.f, v); }
	void SetPulse(float amp, float freq) { pulseAmp_ = amp; pulseFreq_ = freq; }

private:
	std::unique_ptr<Sprite> layers_[3]; // 3層構成
	Vec2  position_{ WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight * 0.5f };
	Vec2  size_{ 150.0f, 150.0f };
	float speedPixelPerSec_ = 900.0f;
	float deadZone_ = 5000.0f;
	float angleRad_ = 0.0f;
	float angularSpeedRadPerSec_ = 1.8f;
	float time_ = 0.0f;
	bool  visible_ = true;
	bool  rainbow_ = true;

	// 虹色用パラメータ
	float hueSpeed_ = 0.5f;
	float sat_ = 1.0f;
	float val_ = 1.2f;
	float pulseAmp_ = 0.25f;
	float pulseFreq_ = 4.0f;

	static void HSVtoRGB(float h, float s, float v, float& r, float& g, float& b)
	{
		h = fmodf(h, 1.0f); if (h < 0) h += 1.0f;
		float i = floorf(h * 6.0f);
		float f = h * 6.0f - i;
		float p = v * (1.0f - s);
		float q = v * (1.0f - f * s);
		float t = v * (1.0f - (1.0f - f) * s);
		switch (static_cast<int>(i) % 6) {
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		case 5: r = v; g = p; b = q; break;
		}
	}
};
