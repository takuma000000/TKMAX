#include "DirectionalLight.h"

void DirectionalLight::Initialize(const Vector4& color, const Vector3& direction, float intensity) {
	color_ = color;
	direction_ = direction;
	intensity_ = intensity;
}

void DirectionalLight::Update() {

}
