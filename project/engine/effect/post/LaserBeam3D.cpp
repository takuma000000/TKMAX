#include "LaserBeam3D.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void LaserBeam3D::Initialize(DirectXCommon* dx) {
		dxCommon_ = dx;
		time_ = 0.0f;
		desc_.active = false;
		desc_.telegraph = false;
	}

	void LaserBeam3D::Update(float dt) {
		time_ += dt;
	}

	void LaserBeam3D::Draw(const Matrix4x4& viewProj,
		const Vector3& camRightWS,
		const Vector3& camUpWS,
		const Vector3& camFwdWS) {

		if (!dxCommon_) { return; }
		if (!desc_.active) { return; }

		dxCommon_->DrawLaserBeamVolume(
			viewProj,
			desc_.startWS,
			desc_.endWS,
			desc_.radius,
			camRightWS,
			camUpWS,
			camFwdWS,
			desc_.sliceCount,
			time_,
			desc_.color,
			desc_.intensity,
			desc_.coreSharpness,
			desc_.edgeSoftness,
			desc_.noiseScale,
			desc_.noiseSpeed,
			desc_.telegraph ? 1u : 0u
		);
	}

#ifdef USE_IMGUI
	void LaserBeam3D::ImGuiDebug() {
		ImGui::Begin("LaserBeam3D");
		ImGui::Checkbox("Active", &desc_.active);
		ImGui::Checkbox("Telegraph", &desc_.telegraph);

		ImGui::DragFloat3("StartWS", &desc_.startWS.x, 0.2f);
		ImGui::DragFloat3("EndWS", &desc_.endWS.x, 0.2f);

		ImGui::DragFloat("Radius", &desc_.radius, 0.01f, 0.01f, 30.0f);
		ImGui::ColorEdit3("Color", &desc_.color.x);
		ImGui::DragFloat("Intensity", &desc_.intensity, 0.05f, 0.0f, 50.0f);
		ImGui::DragFloat("CoreSharpness", &desc_.coreSharpness, 0.1f, 0.1f, 30.0f);
		ImGui::DragFloat("EdgeSoftness", &desc_.edgeSoftness, 0.05f, 0.1f, 10.0f);

		ImGui::DragInt("SliceCount", (int*)&desc_.sliceCount, 1.0f, 1, 256);
		ImGui::DragFloat("NoiseScale", &desc_.noiseScale, 0.01f, 0.0f, 20.0f);
		ImGui::DragFloat("NoiseSpeed", &desc_.noiseSpeed, 0.01f, 0.0f, 20.0f);

		ImGui::End();
	}
#endif
}