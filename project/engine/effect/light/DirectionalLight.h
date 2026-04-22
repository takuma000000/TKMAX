#pragma once
#include "MyMath.h"

//=============================================================
// DirectionalLightクラス
// 平行光源の色・方向・強度を管理するクラス。
//=============================================================
namespace TKM {
	class DirectionalLight {
	public:
		//=============================================================
		// 生成・破棄
		//=============================================================

		DirectionalLight() = default;
		~DirectionalLight() = default;

		//=============================================================
		// 初期化・更新
		//=============================================================

		/// <summary>
		/// 平行光源を初期化します。
		/// </summary>
		void Initialize(const Vector4& color, const Vector3& direction, float intensity);

		/// <summary>
		/// 平行光源を更新します。
		/// </summary>
		void Update();

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// 色を取得します。
		/// </summary>
		Vector4 GetColor() const { return color_; }

		/// <summary>
		/// 方向を取得します。
		/// </summary>
		Vector3 GetDirection() const { return direction_; }

		/// <summary>
		/// 強度を取得します。
		/// </summary>
		float GetIntensity() const { return intensity_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// 方向を設定します。
		/// </summary>
		void SetDirection(const Vector3& direction) { direction_ = direction; }

	private:
		//=============================================================
		// 光源パラメータ
		//=============================================================

		Vector4 color_{};        // 色
		Vector3 direction_{};    // 方向
		float intensity_ = 0.0f; // 強度
	};
}