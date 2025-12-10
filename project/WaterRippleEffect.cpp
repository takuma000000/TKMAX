#include "WaterRippleEffect.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void WaterRippleEffect::Trigger(const Vector2& centerUV, float durationSec) {
	centerUV_ = centerUV;
	duration_ = durationSec;
	time_ = 0.0f;
	active_ = true;
}

void WaterRippleEffect::Update(float dt) {
	if (!dxCommon_) { return; }

	if (!active_) {
		// オフのときは何もしない（DrawPostEffect側で描かれない）
		return;
	}

	time_ += dt;
	float t = (duration_ > 0.0f) ? (time_ / duration_) : 1.0f;

	if (t >= 1.0f) {
		active_ = false;
		t = 1.0f;
	}

	// 0 → radiusMax_ に広がる
	float radius = radiusMax_ * t;

	// 時間とともに弱くする
	float amp = amplitude_ * (1.0f - t);

	dxCommon_->SetWaterRippleParam(
		centerUV_,
		radius,
		amp,
		frequency_,
		width_,
		color_,
		colorIntensity_);
}

#ifdef USE_IMGUI
void WaterRippleEffect::ImGuiDebug() {
	if (ImGui::Begin("水の波紋")) {

		// ==========================
		// ■ 波紋テストボタン
		// ==========================
		if (ImGui::Button("レッツゴー波紋！！！")) {
			// 現在の中心UVと継続時間で波紋を発生
			Trigger(centerUV_, duration_);
		}
		ImGui::SameLine();
		ImGui::Text("テスト用");

		ImGui::Separator();

		ImGui::SliderFloat("継続時間(秒)", &duration_, 0.1f, 3.0f);
		ImGui::SliderFloat("最大半径", &radiusMax_, 0.1f, 3.0f);
		ImGui::SliderFloat("振幅(ゆがみ量)", &amplitude_, 0.0f, 0.1f);
		ImGui::SliderFloat("波の細かさ", &frequency_, 1.0f, 80.0f);
		ImGui::SliderFloat("帯の幅(シャープさ)", &width_, 1.0f, 80.0f);

		float col[3] = { color_.x, color_.y, color_.z };
		if (ImGui::ColorEdit3("縁に乗せる色", col)) {
			color_.x = col[0];
			color_.y = col[1];
			color_.z = col[2];
		}
		ImGui::SliderFloat("色の強さ", &colorIntensity_, 0.0f, 2.0f);

		ImGui::SliderFloat2("中心座標UV (0〜1)", &centerUV_.x, 0.0f, 1.0f);
	}
	ImGui::End();
}
#endif
