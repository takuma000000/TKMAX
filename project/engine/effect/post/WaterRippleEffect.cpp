#define NOMINMAX
#include "WaterRippleEffect.h"
#include "DirectXCommon.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void WaterRippleEffect::Update(float dt) {
	time_ += dt;

	if (!active_) {
		dxCommon_->SetWaterRippleParam(
			centerUV_,
			0.0f,
			0.0f,
			currentDesc_.frequency,
			currentDesc_.width,
			currentDesc_.color,
			0.0f
		);
		return;
	}

	float dur = std::max(0.0001f, currentDesc_.duration);
	float t = time_ / dur;

	if (t >= 1.0f) {
		active_ = false;
		t = 1.0f;
	}

	float radius = currentDesc_.radiusMax * t;
	float amp = currentDesc_.amplitude * (1.0f - t);

	dxCommon_->SetWaterRippleParam(
		centerUV_,
		radius,
		amp,
		currentDesc_.frequency,
		currentDesc_.width,
		currentDesc_.color,
		currentDesc_.colorIntensity
	);
}

void WaterRippleEffect::Trigger(const Vector2& centerUV, const RippleDesc& desc) {
	centerUV_ = centerUV;
	currentDesc_ = desc;
	time_ = 0.0f;
	active_ = true;
}