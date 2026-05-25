#pragma once
#include "BaseEffect.h"

// =============================================================
// RadialBlurEffectクラス
// 画面中心から放射状にぼかすエフェクトを扱うクラス。
// =============================================================
namespace TKM {
	class RadialBlurEffect : public TKM::BaseEffect {
	public:
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
		void Draw() override;

		/// <summary>
		/// ショック用放射ブラー開始
		/// </summary>
		/// <param name="strength"></param>
		/// <param name="duration"></param>
		void BlurStartShock(float strength = 1.0f, float duration = 0.35f);

		/// <summary>
		/// 放射ブラーの有効・無効
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		/// <summary>
		/// 手動制御を解除します。
		/// </summary>
		void ClearManualBlur();

		// Setters===================================
		/// <summary>
		/// 手動で放射ブラーの有効・無効を設定します。
		/// </summary>
		/// <param name="enable"></param>
		/// <param name="strength"></param>
		void SetManualBlur(bool enable, float strength = 1.0f);
		// ==========================================

	private:	
		//==============================================
		// 状態
		//==============================================
		bool active_ = false; // エフェクトが有効かどうか
		//==============================================
		// タイマー
		//==============================================
		float timer_ = 0.0f;     // 経過時間
		float duration_ = 0.35f; // エフェクトの持続時間
		//==============================================
		// 設定
		//==============================================
		float maxStrength_ = 1.0f; // エフェクトの最大強度
		//==============================================
		// 手動制御
		//==============================================
		bool manualControl_ = false;   // 手動制御中か
		float manualStrength_ = 1.0f;  // 手動時の強度
	};
}