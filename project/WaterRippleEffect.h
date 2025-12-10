#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

class WaterRippleEffect : public BaseEffect {
public:
	void Initialize(DirectXCommon* dx) override {
		BaseEffect::Initialize(dx);
	}

	void Update(float dt) override;

	// 描画自体は DirectXCommon::DrawPostEffectToSwapchain() 側でやるので
	// ここでは何もしないでもOK
	void Draw() override {}

	// 波紋開始
	void Trigger(const Vector2& centerUV, float durationSec = 0.6f);

	bool IsActive() const { return active_; }

#ifdef USE_IMGUI
	void ImGuiDebug();
#endif

private:
	bool   active_ = false;
	float  time_ = 0.0f;
	float  duration_ = 0.6f;

	Vector2 centerUV_ = { 0.5f, 0.5f };

	// パラメータ（ImGuiでいじれるように）
	float  radiusMax_ = 1.2f;   // 画面全体に広がる最大半径 (UVベース)
	float  amplitude_ = 0.02f;  // ゆがみ量
	float  frequency_ = 40.0f;  // 波の細かさ
	float  width_ = 40.0f;  // 帯の幅(大きいほど細くシャープ)
};
