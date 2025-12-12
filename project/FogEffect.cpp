#include "FogEffect.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void FogEffect::Update(float dt) {
	if (!dxCommon_) { return; }

	// active_ が false でも「完全停止」ではなく、濃さ0で送るのはアリ
	//（デバッグでON/OFF切り替えたい時に便利）
	if (!freezeTime_) {
		time_ += dt * timeScale_;
	}

	// ★ 自動ドリフト（カメラが動かなくても霧が流れる）
	driftOffsetXZ_.x += driftSpeedXZ_.x * dt;
	driftOffsetXZ_.y += driftSpeedXZ_.y * dt;

	Vector3 sendWorldPos = worldPos_;
	sendWorldPos.x += driftOffsetXZ_.x;
	sendWorldPos.z += driftOffsetXZ_.y;

	float sendDensity = active_ ? density_ : 0.0f;

	dxCommon_->SetFogParam(
		color_,
		sendDensity,
		start_,
		end_,
		noiseScale_,
		noiseStrength_,
		time_,
		sendWorldPos,
		worldScale_
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

		// もっと極端に振れるようにレンジ拡大
		ImGui::SliderFloat("濃さ", &density_, 0.0f, 8.0f);
		ImGui::SliderFloat("開始位置(画面下から)", &start_, 0.0f, 1.0f);
		ImGui::SliderFloat("最大位置(画面下から)", &end_, 0.0f, 1.0f);

		ImGui::SliderFloat("ノイズスケール", &noiseScale_, 0.2f, 30.0f);
		ImGui::SliderFloat("ノイズ強さ", &noiseStrength_, 0.0f, 2.0f);

		// 速度系（今回追加）
		ImGui::SliderFloat("アニメ速度(Time倍率)", &timeScale_, 0.0f, 6.0f);

		ImGui::SliderFloat2("霧の移動速度 XZ(ワールド/秒)", &driftSpeedXZ_.x, -30.0f, 30.0f);
		if (ImGui::Button("移動オフセット リセット")) {
			driftOffsetXZ_ = { 0.0f, 0.0f };
		}

		ImGui::SliderFloat("空間反応(WorldScale)", &worldScale_, 0.0005f, 0.10f);

		ImGui::Text("Time: %.2f", time_);
		ImGui::Text("DriftOffsetXZ: (%.2f, %.2f)", driftOffsetXZ_.x, driftOffsetXZ_.y);
	}
	ImGui::End();
}
#endif