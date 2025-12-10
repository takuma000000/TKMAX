#include "FogEffect.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void FogEffect::Update(float dt) {
	if (!dxCommon_) return;

	time_ += dt;

	// 無効なら density だけ 0 を送る
	float sendDensity = active_ ? density_ : 0.0f;

	dxCommon_->SetFogParam(
		color_,
		sendDensity,
		start_,
		end_,
		noiseScale_,
		noiseStrength_,
		time_
	);
}

#ifdef USE_IMGUI
void FogEffect::ImGuiDebug() {
	if (ImGui::Begin("霧 (Fog)")) {

		float col[3] = { color_.x, color_.y, color_.z };
		if (ImGui::ColorEdit3("霧の色", col)) {
			color_.x = col[0];
			color_.y = col[1];
			color_.z = col[2];
		}

		ImGui::SliderFloat("濃さ", &density_, 0.0f, 3.0f);
		ImGui::SliderFloat("開始位置(画面下から)", &start_, 0.0f, 1.0f);
		ImGui::SliderFloat("最大位置(画面下から)", &end_, 0.0f, 1.0f);

		ImGui::SliderFloat("ノイズスケール", &noiseScale_, 0.5f, 10.0f);
		ImGui::SliderFloat("ノイズ強さ", &noiseStrength_, 0.0f, 1.0f);

		ImGui::Text("Time: %.2f", time_);
	}
	ImGui::End();
}
#endif