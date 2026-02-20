#pragma once
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// Transform（共通）
	// スケール、回転、平行移動の情報をまとめた構造体。
	//=============================================================
	struct Transform {
		Vector3 scale_{ 1.0f, 1.0f, 1.0f };
		Vector3 rotate_{ 0.0f, 0.0f, 0.0f };
		Vector3 translate_{ 0.0f, 0.0f, 0.0f };

		Transform() = default;

		// 3引数版（C2661対策）
		Transform(const Vector3& s, const Vector3& r, const Vector3& t)
			: scale_(s), rotate_(r), translate_(t) {
		}

		void Reset() {
			scale_ = { 1.0f, 1.0f, 1.0f };
			rotate_ = { 0.0f, 0.0f, 0.0f };
			translate_ = { 0.0f, 0.0f, 0.0f };
		}
	};

} // namespace TKM