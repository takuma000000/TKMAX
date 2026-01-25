#include "LaserBeam3D.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void LaserBeam3D::Initialize(DirectXCommon* dx) {
		dxCommon_ = dx;
		time_ = 0.0f;
		desc_.active_ = false;
		desc_.telegraph_ = false;
	}

	void LaserBeam3D::Update(float dt) {
		time_ += dt;
	}

	void LaserBeam3D::Draw(const Matrix4x4& viewProj,
		const Vector3& camRightWS,
		const Vector3& camUpWS,
		const Vector3& camFwdWS) {

		if (!dxCommon_) { return; }
		if (!desc_.active_) { return; }

		dxCommon_->DrawLaserBeamVolume(
			viewProj,
			desc_.startWS_,
			desc_.endWS_,
			desc_.radius_,
			camRightWS,
			camUpWS,
			camFwdWS,
			desc_.sliceCount_,
			time_,
			desc_.color_,
			desc_.intensity_,
			desc_.coreSharpness_,
			desc_.edgeSoftness_,
			desc_.noiseScale_,
			desc_.noiseSpeed_,
			desc_.telegraph_ ? 1u : 0u
		);
	}

#ifdef USE_IMGUI
	void LaserBeam3D::ImGuiDebug() {
		ImGui::Begin("LaserBeam3D");
		ImGui::Checkbox("Active", &desc_.active_);
		ImGui::Checkbox("Telegraph", &desc_.telegraph_);

		ImGui::DragFloat3("StartWS", &desc_.startWS_.x, 0.2f);
		ImGui::DragFloat3("EndWS", &desc_.endWS_.x, 0.2f);

		ImGui::DragFloat("Radius", &desc_.radius_, 0.01f, 0.01f, 30.0f);
		ImGui::ColorEdit3("Color", &desc_.color_.x);
		ImGui::DragFloat("Intensity", &desc_.intensity_, 0.05f, 0.0f, 50.0f);
		ImGui::DragFloat("CoreSharpness", &desc_.coreSharpness_, 0.1f, 0.1f, 30.0f);
		ImGui::DragFloat("EdgeSoftness", &desc_.edgeSoftness_, 0.05f, 0.1f, 10.0f);

		ImGui::DragInt("SliceCount", (int*)&desc_.sliceCount_, 1.0f, 1, 256);
		ImGui::DragFloat("NoiseScale", &desc_.noiseScale_, 0.01f, 0.0f, 20.0f);
		ImGui::DragFloat("NoiseSpeed", &desc_.noiseSpeed_, 0.01f, 0.0f, 20.0f);

		ImGui::End();
	}
#endif
}