#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

class FogEffect : public BaseEffect {
public:
	void Initialize(DirectXCommon* dx) override {
		BaseEffect::Initialize(dx);
	}

	void Update(float dt) override;
	void Draw() override {}  // 描画は DirectXCommon 側のチェーンでやる

	bool IsActive() const { return active_; }
	void SetActive(bool a) { active_ = a; }

#ifdef USE_IMGUI
	void ImGuiDebug();
#endif

private:
	bool   active_ = false;            // 有効フラグ
	float  density_ = 0.8f;             // 全体の濃さ
	float  start_ = 0.0f;             // 霧開始の高さ
	float  end_ = 0.7f;             // 霧最大の高さ
	float  noiseScale_ = 4.0f;             // ノイズ細かさ
	float  noiseStrength_ = 0.3f;             // ノイズの強さ
	float  time_ = 0.0f;             // 経過時間

	Vector3 color_ = { 0.9f, 0.9f, 1.0f }; // 霧の色（薄い青白）
};