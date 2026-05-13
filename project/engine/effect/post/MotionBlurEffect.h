#pragma once
#include "BaseEffect.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	//=============================================================
	// MotionBlurEffect
	// 前フレームの画面を混ぜて残像を出すモーションブラー
	//=============================================================
	class MotionBlurEffect : public TKM::BaseEffect {
	public:
		MotionBlurEffect() = default;
		~MotionBlurEffect() override = default;

		/// <summary>
		/// モーションブラーエフェクトの初期化
		/// </summary>
		/// <param name="dx">DirectXCommonのインスタンス</param>
		void Initialize(TKM::DirectXCommon* dx) override;
		/// <summary>
		/// モーションブラーエフェクトの更新
		/// </summary>
		/// <param name="dt">デルタタイム</param>
		void Update(float dt) override;
		/// <summary>
		/// モーションブラーエフェクトの描画
		/// </summary>
		void Draw() override {}
		/// <summary>
		///	モーションブラーエフェクトのImGuiデバッグ
		/// </summary>
		void ImGuiDebug();

		bool IsActive() const { return active_; }

		// Getter====================================
		/// <summary>
		/// モーションブラーの強度を取得
		/// </summary>
		/// <returns>モーションブラーの強度</returns>
		float GetStrength() const { return strength_; }
		// ==========================================
		// Setter====================================
		/// <summary>
		/// モーションブラーの有効/無効を設定
		/// </summary>
		/// <param name="active">有効にするかどうか</param>
		void SetActive(bool active) { active_ = active; }
		/// <summary>
		/// モーションブラーの強度を設定
		/// </summary>
		/// <param name="strength">強度</param>
		void SetStrength(float strength) { strength_ = strength; }
		// ==========================================

	private:
		//==========================================
		// フィールド
		//==========================================
		bool active_ = true; // モーションブラーの有効/無効
		float strength_ = 0.12f; // モーションブラーの強度 (0.0f〜0.95f程度。あまり高くしすぎると画面が真っ黒になる)
	};
}