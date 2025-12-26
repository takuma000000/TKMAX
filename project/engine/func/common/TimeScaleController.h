#pragma once
#include <algorithm>
#include "MyMath.h"

namespace TKM {
	class TimeScaleController {
	public:
		/// <summary>
		/// 初期化
		/// </summary>
		void Initialize();
		/// <summary>
		/// 更新
		/// </summary>
		/// <param name="dt"></param>
		void Update(float dt);

		/// <summary>
		/// スロー要求
		/// </summary>
		/// <param name="scale"></param>
		/// <param name="duration"></param>
		void RequestSlow(float scale, float duration); // 引数: スケール、維持時間
		/// <summary>
		/// スロー要求（詳細指定版）
		/// </summary>
		/// <param name="scale"></param>
		/// <param name="duration"></param>
		/// <param name="blendIn"></param>
		/// <param name="blendOut"></param>
		void RequestSlowAdvanced(float scale, float duration, float blendIn, float blendOut); // 引数: スケール、維持時間、入りの速さ、戻りの速さ

		// Getter===================================
		float GetScale() const { return currentScale_; }
		// =========================================1

	private:
		enum class Phase {
			None,
			BlendIn,
			Hold,
			BlendOut
		};

		float currentScale_ = 1.0f;
		float targetScale_ = 1.0f;

		float hold_ = 0.0f;
		float timer_ = 0.0f;

		float blendIn_ = 0.0f;
		float blendOut_ = 0.0f;

		Phase phase_ = Phase::None;
	};
}