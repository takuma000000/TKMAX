#include "VignettingEffect.h"
#include "DirectXCommon.h"

void VignettingEffect::Initialize(DirectXCommon* dx) {
	BaseEffect::Initialize(dx);
	// パイプライン初期化
	dxCommon_->InitializeVignettingPipeline();
	// DX 側に自分を登録
	dxCommon_->SetVignettingEffect(this);
}

void VignettingEffect::Update(float dt) {
	if (!dxCommon_) { return; }

	// ボス Wave 中なら 1.0 までフェードイン、終わったら 0 までフェードアウト
	const float fadeSpeed = 2.0f; // 早さはお好み

	float target = inBossWave_ ? 1.0f : 0.0f;
	float delta = target - currentIntensity_;
	float step = fadeSpeed * dt;

	if (fabsf(delta) <= step) {
		currentIntensity_ = target;
	} else {
		currentIntensity_ += (delta > 0 ? step : -step);
	}

	active_ = (currentIntensity_ > 0.01f);

	// DX 側にパラメータを送る（強度はフェード込）
	Vector4 col = color_;
	dxCommon_->SetVignettingParam(col,
		intensity_ * currentIntensity_,
		radius_,
		softness_);

#ifdef USE_IMGUI
	ImGuiControl();
#endif
}

void VignettingEffect::ImGuiControl() {
#ifdef USE_IMGUI
	if (ImGui::Begin("Vignetting")) {

		ImGui::Checkbox("Boss Wave中", &inBossWave_);

		float col[3] = { color_.x, color_.y, color_.z };
		if (ImGui::ColorEdit3("Color", col)) {
			color_.x = col[0];
			color_.y = col[1];
			color_.z = col[2];
		}

		ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 2.0f);
		ImGui::SliderFloat("Radius", &radius_, 0.0f, 1.0f);
		ImGui::SliderFloat("Softness", &softness_, 0.0f, 1.0f);

		ImGui::Text("CurrentIntensity: %.2f", currentIntensity_);
	}
	ImGui::End();
#endif
}