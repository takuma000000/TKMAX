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

	dxCommon_->SetWaterRippleParam(centerUV_, radius, amp, frequency_, width_);
}

#ifdef USE_IMGUI
void WaterRippleEffect::ImGuiDebug() {
	if (ImGui::Begin("水の波紋")) {
		ImGui::Checkbox("エフェクト有効", &active_);
		ImGui::SliderFloat("継続時間(秒)", &duration_, 0.1f, 3.0f);
		ImGui::SliderFloat("最大半径", &radiusMax_, 0.1f, 3.0f);
		ImGui::SliderFloat("振幅(ゆがみ量)", &amplitude_, 0.0f, 0.1f);
		ImGui::SliderFloat("波の細かさ", &frequency_, 1.0f, 80.0f);
		ImGui::SliderFloat("帯の幅(シャープさ)", &width_, 1.0f, 80.0f);
		ImGui::SliderFloat2("中心座標UV (0〜1)", &centerUV_.x, 0.0f, 1.0f);
	}
	ImGui::End();
}
#endif
