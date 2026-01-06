#include "SmokeVolume3D.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void SmokeVolume3D::Initialize(DirectXCommon* dx) {
		dxCommon_ = dx;
		time_ = 0.0f;
		active_ = true;
	}

	void SmokeVolume3D::Update(float dt) {
		time_ += dt;
	}

	void SmokeVolume3D::Draw(const Matrix4x4& viewProj, const Vector3& camRightWS, const Vector3& camUpWS, const Vector3& camFwdWS) {
		if (!active_) { return; }
		if (!dxCommon_) { return; }

		desc_.worldPos = desc_.centerWS;

		dxCommon_->DrawSmokeVolume(
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
			desc_.baseScale,
			desc_.detailScale,
			desc_.detailStrength,
			desc_.threshold,
			desc_.softness,
			desc_.flowSpeed,
			desc_.riseSpeed,
			desc_.alphaMax,
			desc_.worldScale,
			desc_.worldPos
		);
	}

#ifdef USE_IMGUI
	void SmokeVolume3D::ImGuiDebug() {
		if (ImGui::Begin("煙ボリューム 3D")) {
			ImGui::Checkbox("有効", &active_);

			ImGui::DragFloat3("中心座標 (WS)", &desc_.centerWS.x, 1.0f);
			ImGui::DragFloat3("半径サイズ (WS)", &desc_.halfSizeWS.x, 1.0f, 1.0f, 5000.0f);

			ImGui::ColorEdit3("色", &desc_.color.x);
			ImGui::DragFloat("密度", &desc_.density, 0.001f, 0.0f, 2.0f);
			ImGui::DragInt("スライス数", (int*)&desc_.sliceCount, 1.0f, 1, 256);

			ImGui::Separator();
			ImGui::DragFloat("ベーススケール", &desc_.baseScale, 0.001f, 0.001f, 3.0f);
			ImGui::DragFloat("ディテールスケール", &desc_.detailScale, 0.001f, 0.001f, 10.0f);
			ImGui::DragFloat("ディテール強度", &desc_.detailStrength, 0.01f, 0.0f, 1.0f);

			ImGui::DragFloat("しきい値", &desc_.threshold, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("ぼかし", &desc_.softness, 0.005f, 0.0f, 1.0f);

			ImGui::Separator();
			ImGui::DragFloat("流れ速度（カメラ方向）", &desc_.flowSpeed, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("上昇速度", &desc_.riseSpeed, 0.01f, 0.0f, 10.0f);

			ImGui::DragFloat("最大アルファ", &desc_.alphaMax, 0.01f, 0.0f, 1.0f);
		}
		ImGui::End();
	}
#endif
}