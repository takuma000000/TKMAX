#include "RadialBlurEffect.h"
#include "DirectXCommon.h"

void RadialBlurEffect::Initialize(DirectXCommon* dx) {
	BaseEffect::Initialize(dx);

	if (dxCommon_) {
		// コピー用パイプラインがまだなら初期化
		dxCommon_->InitializeCopyImagePipeline();
		// ラジアルブラー用パイプラインも初期化
		dxCommon_->InitializeRadialBlurPipeline();
	}
}

void RadialBlurEffect::BulrStartShock(float strength, float duration) {
	active_ = true;
	timer_ = 0.0f;
	maxStrength_ = strength;
	duration_ = duration;
}

void RadialBlurEffect::Update(float dt) {
	if (!active_) { return; }

	timer_ += dt;
	if (timer_ >= duration_) {
		active_ = false;
	}
}

void RadialBlurEffect::Draw() {
	if (!dxCommon_) { return; }

	if (active_) {
		// ラジアルブラーでコピー
		dxCommon_->DrawRadialBlurToSwapchain();
	} else {
		// 通常のコピー
		dxCommon_->DrawRenderTextureToSwapchain();
	}
}