#pragma once
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// Transform構造体
	// スケール、回転、平行移動をまとめた構造体
	//=============================================================
	struct Transform {
		Vector3 scale_{ 1.0f, 1.0f, 1.0f };       // スケール
		Vector3 rotate_{ 0.0f, 0.0f, 0.0f };      // 回転
		Vector3 translate_{ 0.0f, 0.0f, 0.0f };   // 平行移動

		//=============================================================
		// 生成
		//=============================================================

		Transform() = default;

		/// <summary>
		/// 各値を指定してTransformを生成します。
		/// </summary>
		/// <param name="s">スケール</param>
		/// <param name="r">回転</param>
		/// <param name="t">平行移動</param>
		Transform(const Vector3& s, const Vector3& r, const Vector3& t)
			: scale_(s), rotate_(r), translate_(t) {
		}

		//=============================================================
		// リセット
		//=============================================================

		/// <summary>
		/// Transformを初期状態に戻します。
		/// </summary>
		void Reset() {
			scale_ = { 1.0f, 1.0f, 1.0f };
			rotate_ = { 0.0f, 0.0f, 0.0f };
			translate_ = { 0.0f, 0.0f, 0.0f };
		}
	};

} // namespace TKM