#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

class VignettingEffect : public BaseEffect {
public:
	VignettingEffect() = default;
	~VignettingEffect() override = default;

	void Initialize(TKM::DirectXCommon* dx) override;
	void Update(float dt) override;
	void Draw() override {} // 今回は DX 側で DrawPostEffectToSwapchain を呼ぶので何もしない

	bool IsActive() const { return active_; }

	// ボス Wave 突入/終了で呼ぶ用
	void SetBossWave(bool inBoss) { inBossWave_ = inBoss; }

private:
	// パラメータ
	Vector4 color_{ 0.012f, 0.016f, 0.165f, 1.0f }; // 色
	float   intensity_ = 0.8f;   // 強度
	float   radius_ = 0.6f;      // 基本の半径（非ボス時など）
	float   softness_ = 1.0f;    // ぼかし

	bool    active_ = false;
	bool    inBossWave_ = false; // ボス Wave 中フラグ

	// フェード用
	float   currentIntensity_ = 0.0f;

	// ボス戦中の半径ゆらぎ用 ==========================
	float radiusMin_ = 0.677f;    // 最小半径
	float radiusMax_ = 0.382f;    // 最大半径
	float radiusAnimT_ = 0.0f;   // アニメ用タイマー
	float radiusAnimSpeed_ = 0.8f; // 揺れる速さ
	// ===================================================
	
	/// <summary>
	/// ImGui デバッグ表示
	/// </summary>
	void ImGuiDebug();
};