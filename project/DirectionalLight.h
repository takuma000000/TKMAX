#pragma once
#include "engine/func/math/Vector3.h"
#include "engine/func/math/Vector4.h"

class DirectionalLight {

public:
	DirectionalLight() = default;
	~DirectionalLight() = default;

	void Initialize(const Vector4& color, const Vector3& direction, float intensity);
	void Update();

	Vector4 GetColor() const { return color_; }
	Vector3 GetDirection() const { return direction_; }
	float GetIntensity() const { return intensity_; }

	void SetDirection(const Vector3& direction) { direction_ = direction; }

private:
	Vector4 color_;
	Vector3 direction_;
	float intensity_;
};