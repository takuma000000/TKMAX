#include "DirectionalLight.h"
#include <MyMath.h>

namespace TKM {
	void DirectionalLight::Initialize(const Vector4& color, const Vector3& direction, float intensity) {
		// メンバ変数に値をセット
		color_ = color;
		direction_ = direction;
		intensity_ = intensity;
	}

	void DirectionalLight::Update() {
		direction_ = MyMath::Normalize(direction_); // 方向ベクトルを正規化
	}
}