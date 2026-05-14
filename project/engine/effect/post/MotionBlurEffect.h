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

		/// <summary>
		/// モーションブラーのバーストを開始します。一定時間だけ強いモーションブラーをかけることができます。
		/// </summary>
		/// <param name="strength">バーストの強度</param>
		/// <param name="duration">バーストの持続時間</param>
		void StartBurst(float strength, float duration);
		/// <summary>
		/// モーションブラーのバーストを停止します。バーストが終了し、強度が0になります。
		/// </summary>
		void Stop();

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
		bool active_ = false; // モーションブラーの有効/無効
		float strength_ = 0.0f; // モーションブラーの強度 (0.0f〜0.95f程度。あまり高くしすぎると画面が真っ黒になる)

		bool burstActive_ = false; // バースト中かどうか
		float burstTimer_ = 0.0f; // バーストの経過時間
		float burstDuration_ = 0.0f; // バーストの持続時間
		float burstStartStrength_ = 0.0f; // バースト開始時の強度
	};
}