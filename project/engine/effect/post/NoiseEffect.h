#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	//=============================================================
	// NoiseEffectクラス
	// 画面をグリッチノイズで崩すポストエフェクト管理クラス
	//=============================================================
	class NoiseEffect : public TKM::BaseEffect {
	public:
		NoiseEffect() = default;
		~NoiseEffect() override = default;

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
		void Draw() override {}
		/// <summary>
		/// ImGuiデバッグ表示
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// 有効かどうか
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		// Setter======================================================
		/// <summary>
		/// 有効・無効の切り替え
		/// </summary>
		/// <param name="active"></param>
		void SetActive(bool active) { active_ = active; }
		/// <summary>
		///	全体強度の設定
		/// </summary>
		/// <param name="intensity"></param>
		void SetIntensity(float intensity) { intensity_ = intensity; }
		/// <summary>
		/// 横線の本数感の設定
		/// </summary>
		/// <param name="flash"></param>
		void SetFlash(float flash) { flash_ = flash; }
		// ============================================================

	private:
		//=============================================================
		// 状態
		//=============================================================
		bool active_ = false; // エフェクト有効フラグ
		float time_ = 0.0f;   // エフェクト時間経過カウンタ
		//=============================================================
		// パラメータ
		//=============================================================
		float intensity_ = 0.85f;     // 全体強度
		float lineDensity_ = 180.0f;  // 横線の本数感
		float lineSpeed_ = 8.0f;      // 横線ノイズの流れ
		float blockScale_ = 32.0f;    // ブロック分割サイズ
		float blockShift_ = 0.030f;   // ブロック単位のUVずれ量
		float rgbShift_ = 0.006f;     // RGB分離量
		float flash_ = 0.08f;         // 白飛び感
	};
}