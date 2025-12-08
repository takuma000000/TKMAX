#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

class VignettingEffect : public BaseEffect {
public:
	VignettingEffect() = default;
	~VignettingEffect() override = default;

	void Initialize(DirectXCommon* dx) override;
	void Update(float dt) override;
	void Draw() override {} // 今回は DX 側で DrawPostEffectToSwapchain を呼ぶので何もしない

	bool IsActive() const { return active_; }

	// ボス Wave 突入/終了で呼ぶ用
	void SetBossWave(bool inBoss) { inBossWave_ = inBoss; }

private:
	// パラメータ
	Vector4 color_{ 0.0f, 0.0f, 0.0f, 1.0f }; // 黒縁
	float   intensity_ = 0.8f;   // 強度
	float   radius_ = 0.6f;   // 中央からどれくらいまで普通に見せるか
	float   softness_ = 0.4f;   // 周辺のボケ具合

	bool    active_ = false;
	bool    inBossWave_ = false; // ボス Wave 中フラグ

	// フェード用（必要なら）
	float   currentIntensity_ = 0.0f;

	void ImGuiControl();
};