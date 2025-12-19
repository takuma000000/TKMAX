#include "TimeScaleController.h"

void TimeScaleController::Initialize() {
	currentScale_ = 1.0f;
	targetScale_ = 1.0f;
	hold_ = 0.0f;
	timer_ = 0.0f;
	blendIn_ = 0.0f;
	blendOut_ = 0.0f;
	phase_ = Phase::None;
}

void TimeScaleController::RequestSlow(float scale, float duration) {

	float dur = std::max(0.0f, duration);

	float blendIn = std::min(0.06f, dur * 0.25f);
	float blendOut = std::clamp(dur * 0.80f, 0.10f, 0.40f);

	RequestSlowAdvanced(scale, duration, blendIn, blendOut);
}

void TimeScaleController::RequestSlowAdvanced(float scale, float duration, float blendIn, float blendOut) {

	targetScale_ = std::clamp(scale, 0.0f, 1.0f);
	hold_ = std::max(0.0f, duration);
	blendIn_ = std::max(0.0f, blendIn);
	blendOut_ = std::max(0.0f, blendOut);

	timer_ = 0.0f;
	phase_ = Phase::BlendIn;
}

void TimeScaleController::Update(float dt) {
	switch (phase_) {
	case Phase::None: {
		currentScale_ = 1.0f;
		break;
	}
	case Phase::BlendIn: {
		timer_ += dt;
		float t = (blendIn_ <= 0.0f) ? 1.0f : std::clamp(timer_ / blendIn_, 0.0f, 1.0f);
		currentScale_ = MyMath::Lerp(1.0f, targetScale_, t);
		if (t >= 1.0f) {
			phase_ = Phase::Hold;
			timer_ = 0.0f;
		}
		break;
	}
	case Phase::Hold: {
		timer_ += dt;
		currentScale_ = targetScale_;
		if (timer_ >= hold_) {
			phase_ = Phase::BlendOut;
			timer_ = 0.0f;
		}
		break;
	}
	case Phase::BlendOut: {
		timer_ += dt;
		float t = (blendOut_ <= 0.0f) ? 1.0f : std::clamp(timer_ / blendOut_, 0.0f, 1.0f);
		currentScale_ = MyMath::Lerp(targetScale_, 1.0f, t);
		if (t >= 1.0f) {
			phase_ = Phase::None;
			timer_ = 0.0f;
			currentScale_ = 1.0f;
		}
		break;
	}
	}
}