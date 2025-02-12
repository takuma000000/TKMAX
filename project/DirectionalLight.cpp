#include "DirectionalLight.h"

void DirectionalLight::Initialize(const Vector4& color, const Vector3& direction, float intensity) {
	color_ = color;
	direction_ = direction;
	intensity_ = intensity;
}

void DirectionalLight::Update() {
	// ライト方向や強度を動的に更新する場合はここに記述
}
