#pragma once
#include "BaseEffect.h"
#include "MyMath.h"

namespace TKM {

	//==========================================================
	// SpeedLineEffectクラス
	// 画面にスピード線を表示するエフェクト。
	//==========================================================
	class SpeedLineEffect : public TKM::BaseEffect {
	public:
		SpeedLineEffect() = default;
		~SpeedLineEffect() override = default;

		/// <summary>
		/// スピード線エフェクトを初期化します。
		/// </summary>
		/// <param name="dx">DirectX共通管理</param>
		void Initialize(TKM::DirectXCommon* dx) override;
		/// <summary>
		/// スピード線エフェクトを更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		void Update(float dt) override;
		/// <summary>
		/// スピード線エフェクトを描画します。
		/// </summary>
		void Draw() override {}

		/// <summary>
		/// スピード線エフェクトが現在アクティブかどうか。
		/// </summary>
		/// <returns>アクティブならtrue</returns>
		bool IsActive() const { return active_; }

		// Setter====================================
		/// <summary>
		/// スピード線エフェクトをアクティブにするかどうか。
		/// </summary>
		/// <param name="active">アクティブにするならtrue</param>
		void SetActive(bool active) { requestActive_ = active; }
		/// <summary>
		/// スピード線エフェクトの方向を設定します。
		/// </summary>
		/// <param name="direction">スピード線の方向</param>
		void SetDirection(const Vector2& direction) { direction_ = direction; }
		// ==========================================

	private:
		/// <summary>
		/// スピード線の方向を正規化して返します。
		/// </summary>
		/// <param name="direction">正規化前の方向</param>
		/// <returns>正規化された方向</returns>
		Vector2 NormalizeDirection_(const Vector2& direction) const;

		//==========================================================
		// 内部状態
		//==========================================================
		bool active_ = false;        // エフェクトが現在アクティブか
		bool requestActive_ = false; // エフェクトをアクティブにするよう要求されているか

		// スピード線の方向。正規化して使用する。
		Vector2 direction_{ 1.0f, 0.0f };

		// エフェクトの現在の強さ。0.0f で見えない、1.0f で最大。
		float currentIntensity_ = 0.0f;
		float time_ = 0.0f;

		// エフェクトのパラメータ
		float intensity_ = 1.0f;   // エフェクトの強さ。0.0f で見えない、1.0f で最大。
		float fadeSpeed_ = 18.0f;   // エフェクトのフェードイン・アウト速度。大きいほど速く変化する。
		float lineDensity_ = 42.0f; // スピード線の密度。大きいほど線が多くなる。
		float lineSpeed_ = 18.0f;   // スピード線の移動速度。大きいほど速く流れる。
		float lineWidth_ = 0.035f;  //	
	};
}