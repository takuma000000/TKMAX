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
	bool   active_ = true;             // 霧は最初から有効でOK
	float  density_ = 0.495f;            // 画面全体の濃さ
	float  start_ = 0.17f;              // 全画面に霧をかけたいので 0〜1 のまま
	float  end_ = 0.376f;
	float  noiseScale_ = 10.0f;         // 塊の大きさ）
	float  noiseStrength_ = 0.817f;      // ムラの強さ
	float  time_ = 0.0f; // 時間経過用
	Vector3 color_ = { 0.9f, 0.9f, 1.0f }; // OK（青白い霧）
};