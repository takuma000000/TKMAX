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
	bool   active_ = false; // エフェクト有効フラグ
	float  time_ = 0.0f;
	float  duration_ = 2.492f; // 継続時間

	Vector2 centerUV_ = { 0.5f, 0.5f }; // 波紋中心 (UV)

	// パラメータ（ImGuiでいじれるように）
	float  radiusMax_ = 0.857f;   // 画面全体に広がる最大半径 (UVベース)
	float  amplitude_ = 0.1f;  // ゆがみ量
	float  frequency_ = 80.0f;  // 波の細かさ
	float  width_ = 10.0f;  // 帯の幅(大きいほど細くシャープ)
	Vector3 color_ = { 1.0f, 1.0f, 1.0f }; // 波紋色
	float colorIntensity_ = 0.0f; // 波紋色の強さ
};
