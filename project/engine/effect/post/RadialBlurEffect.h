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
		void BulrStartShock(float strength = 1.0f, float duration = 0.35f);

		/// <summary>
		/// 放射ブラーの有効・無効
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

	private:
		bool  active_ = false;
		float timer_ = 0.0f;
		float duration_ = 0.35f;
		float maxStrength_ = 1.0f;   // 今は未使用だが拡張用
	};
}