#include "AuraEffect.h"
#include "DirectXCommon.h"

void AuraEffect::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	active_ = false;
}

void AuraEffect::Update(float dt) {
	time_ += dt;
	if (!dxCommon_) { return; }
	if (!active_) { return; }
	PushToGpu();
}

void AuraEffect::PushToGpu() {
	if (!dxCommon_) { return; }

	dxCommon_->SetAuraParam(
		centerUV_,
		topUV_,
		bottomUV_,
		aspect_,
		time_,
		scale_,
		intensity_,
		useRing_ ? 1.0f : 0.0f,
		ringRadius_,
		ringWidth_,
		colorA_,
		colorB_,
		mix_,
		taper_,
		noiseScale_,
		noiseSpeed_,
		flameStrength_,
		edgePower_,
		verticalFade_
	);
}

#ifdef USE_IMGUI
void AuraEffect::ImGuiDebug() {
	ImGui::Begin("AuraEffect");

	ImGui::Checkbox("Active", &active_);
	ImGui::DragFloat2("CenterUV", &centerUV_.x, 0.001f, 0.0f, 1.0f);
	ImGui::DragFloat("Scale", &scale_, 0.001f, 0.01f, 1.0f);
	ImGui::DragFloat("Intensity", &intensity_, 0.01f, 0.0f, 10.0f);

	ImGui::Checkbox("UseRing", &useRing_);
	ImGui::DragFloat("RingRadius", &ringRadius_, 0.001f, 0.0f, 1.0f);
	ImGui::DragFloat("RingWidth", &ringWidth_, 0.1f, 1.0f, 80.0f);

	ImGui::ColorEdit3("ColorA", &colorA_.x);
	ImGui::ColorEdit3("ColorB", &colorB_.x);
	ImGui::DragFloat("Mix", &mix_, 0.01f, 0.0f, 1.0f);

	auraVolumeRenderer_.DrawImGui("Aura Volume");

	ImGui::End();
}
#endif