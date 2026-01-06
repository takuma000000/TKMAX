#include "FogVolume3D.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void FogVolume3D::Initialize(DirectXCommon* dx) {
		dxCommon_ = dx;
		time_ = 0.0f;
		active_ = true;
	}

	void FogVolume3D::Update(float dt) {
		if (!active_) { return; }
		time_ += dt;
	}

	void FogVolume3D::Draw(const Matrix4x4& viewProj, const Vector3& camRightWS, const Vector3& camUpWS, const Vector3& camFwdWS) {
		if (!active_) { return; }
		if (!dxCommon_) { return; }

		desc_.worldPos = desc_.centerWS;

		dxCommon_->DrawFogVolume(
			viewProj,
			desc_.centerWS,
			desc_.halfSizeWS,
			camRightWS,
			camUpWS,
			camFwdWS,
			desc_.sliceCount,
			time_,
			desc_.color,
			desc_.density,
			desc_.noiseScale,
			desc_.noiseSpeed,
			desc_.softness,
			desc_.fogStart,
			desc_.fogEnd,
			desc_.noiseStrength,
			desc_.worldScale,
			desc_.worldPos
		);
	}

#ifdef USE_IMGUI
	void FogVolume3D::ImGuiDebug() {
		if (!active_) { return; }

		ImGui::Begin("霧(空間)");
		if (ImGui::CollapsingHeader("空間霧（FogVolume3D）", ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::Checkbox("有効 / 無効", &active_);

			ImGui::DragFloat3("中心座標（ワールド）", &desc_.centerWS.x, 0.5f);
			ImGui::DragFloat3("範囲サイズ（半径）", &desc_.halfSizeWS.x, 1.0f, 1.0f, 3000.0f);

			ImGui::ColorEdit3("霧の色", &desc_.color.x);
			ImGui::DragFloat("霧の濃さ", &desc_.density, 0.001f, 0.0f, 1.0f);

			ImGui::DragInt("重なり枚数（スライス数）", (int*)&desc_.sliceCount, 1.0f, 1, 256);

			ImGui::DragFloat("ノイズスケール", &desc_.noiseScale, 0.001f, 0.0f, 1.0f);
			ImGui::DragFloat("ノイズ速度", &desc_.noiseSpeed, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("端のぼかし具合", &desc_.softness, 0.05f, 0.1f, 10.0f);
		}
		ImGui::End();
	}
#endif
}