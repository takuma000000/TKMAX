#define NOMINMAX
#include "WaterRippleEffect.h"
#include "DirectXCommon.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {
	void WaterRippleEffect::Update(float dt) {
		time_ += dt;

		if (!active_) {
			dxCommon_->SetWaterRippleParam(
				centerUV_,
				0.0f,
				0.0f,
				currentDesc_.frequency_,
				currentDesc_.width_,
				currentDesc_.color_,
				0.0f
			);
			return;
		}

		float dur = std::max(0.0001f, currentDesc_.duration_);
		float t = time_ / dur;

		if (t >= 1.0f) {
			active_ = false;
			t = 1.0f;
		}

		float radius = currentDesc_.radiusMax_ * t;
		float amp = currentDesc_.amplitude_ * (1.0f - t);

		dxCommon_->SetWaterRippleParam(
			centerUV_,
			radius,
			amp,
			currentDesc_.frequency_,
			currentDesc_.width_,
			currentDesc_.color_,
			currentDesc_.colorIntensity_
		);
	}

	void WaterRippleEffect::Trigger(const Vector2& centerUV, const RippleDesc& desc) {
		centerUV_ = centerUV;
		currentDesc_ = desc;
		time_ = 0.0f;
		active_ = true;
	}
}