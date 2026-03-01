#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	//=============================================================
	// VignettingEffectクラス
	// 画面の周囲を暗くするビネットエフェクトの管理を行うクラス。
	//=============================================================
	class VignettingEffect : public TKM::BaseEffect {
	public:
		VignettingEffect() = default;
		~VignettingEffect() override = default;

		/// <summary>
		/// ポストエフェクトの初期化
		/// </summary>
		/// <param name="dx"></param>
		void Initialize(TKM::DirectXCommon* dx) override;
		/// <summary>
		/// ポストエフェクトの更新
		/// </summary>
		/// <param name="dt"></param>
		void Update(float dt) override;
		/// <summary>
		/// ポストエフェクトの描画
		/// </summary>
		void Draw() override {} // 今回は DX 側で DrawPostEffectToSwapchain を呼ぶので何もしない
		/// <summary>
		/// ImGui デバッグ表示
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// ビネットの有効・無効
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		// Setter========================================
		/// <summary>
		/// ビネットの有効・無効をセットします。
		/// </summary>
		/// <param name="active"></param>
		void SetLowHP(bool inLowHp) { inLowHP_ = inLowHp; }
		// ==============================================

	private:
		// パラメータ
		Vector4 color_{ 0.012f, 0.016f, 0.165f, 1.0f }; // 色
		float   intensity_ = 0.8f;   // 強度
		float   radius_ = 0.6f;      // 基本の半径（非ボス時など）
		float   softness_ = 1.0f;    // ぼかし

		bool    active_ = false;
		bool    inLowHP_ = false;    // HP低下（危険）フラグ

		// フェード用
		float   currentIntensity_ = 0.0f;

		//=============================================
		// ボス戦中の半径ゆらぎ用
		//=============================================
		float radiusMin_ = 0.677f; // 最小半径
		float radiusMax_ = 0.382f; // 最大半径
		float radiusAnimT_ = 0.0f; // アニメ用タイマー
		float radiusAnimSpeed_ = 0.8f; // 揺れる速さ
		//=============================================
		// 低HP用パラメータ
		//=============================================
		Vector4 lowHPColor_{ 0.85f, 0.05f, 0.05f, 1.0f }; // 赤
		float lowHPIntensity_ = 0.95f;  // 強度（固定寄り。パッパ防止）
		float lowHPRadiusMin_ = 0.42f;  // 半径：小さいほど覆う範囲が広い（想定）
		float lowHPRadiusMax_ = 0.62f;  // 半径：大きいほど覆う範囲が狭い（想定）
		float lowHPSoftness_ = 1.0f;    // ぼかし
		float lowHPPulseT_ = 0.0f;      // 揺れタイマー
		float lowHPPulseSpeed_ = 0.45f; // ゆっくり揺れる（これが速度）
	};
}