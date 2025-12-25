#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;
}

class AuraVolumeRenderer {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);

	void Draw(
		const Matrix4x4& viewProj,
		const Vector3& bossCenter,
		const Vector3& collider,
		bool active);

#ifdef USE_IMGUI
	void DrawImGui(const char* label = "AuraVolume");
#endif

	// 調整用（必要なら後でImGuiにしてもいい）
	void SetSliceCount(uint32_t v) { sliceCount_ = v; }
	void SetRadiusMul(float v) { radiusMul_ = v; }
	void SetHeightMul(float v) { heightMul_ = v; }
	void SetIntensity(float v) { intensity_ = v; }

	void SetAlwaysOn(bool v) { alwaysOn_ = v; }
	bool IsAlwaysOn() const { return alwaysOn_; }

private:
	TKM::DirectXCommon* dxCommon_ = nullptr;

	uint32_t sliceCount_ = 12;   // 8〜16が目安
	float radiusMul_ = 1.25f;    // collider.x に掛ける
	float heightMul_ = 1.60f;    // collider.y に掛ける

	Vector3 color_ = { 0.2f, 0.6f, 1.0f };
	float intensity_ = 1.35f;

	// 炎パラメータ
	float noiseScale_ = 10.0f;
	float noiseSpeed_ = 1.6f;
	float rimPower_ = 2.2f;
	float alphaBase_ = 1.0f;

	float time_ = 0.0f;

	// デバッグ用
	uint32_t drawCallsThisFrame_ = 0;

	bool alwaysOn_ = false; // デバッグ用：常にONにする
};
