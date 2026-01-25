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

		desc_.worldPos_ = desc_.centerWS_;

		dxCommon_->DrawSmokeVolume(
			viewProj,
			desc_.centerWS_,
			desc_.halfSizeWS_,
			camRightWS,
			camUpWS,
			camFwdWS,
			desc_.sliceCount_,
			time_,
			desc_.color_,
			desc_.density_,
			desc_.baseScale_,
			desc_.detailScale_,
			desc_.detailStrength_,
			desc_.threshold_,
			desc_.softness_,
			desc_.flowSpeed_,
			desc_.riseSpeed_,
			desc_.alphaMax_,
			desc_.worldScale_,
			desc_.worldPos_
		);
	}

#ifdef USE_IMGUI
	void SmokeVolume3D::ImGuiDebug() {
		if (ImGui::Begin("煙ボリューム 3D")) {
			ImGui::Checkbox("有効", &active_);

			ImGui::DragFloat3("中心座標 (WS)", &desc_.centerWS_.x, 1.0f);
			ImGui::DragFloat3("半径サイズ (WS)", &desc_.halfSizeWS_.x, 1.0f, 1.0f, 5000.0f);

			ImGui::ColorEdit3("色", &desc_.color_.x);
			ImGui::DragFloat("密度", &desc_.density_, 0.001f, 0.0f, 2.0f);
			ImGui::DragInt("スライス数", (int*)&desc_.sliceCount_, 1.0f, 1, 256);

			ImGui::Separator();
			ImGui::DragFloat("ベーススケール", &desc_.baseScale_, 0.001f, 0.001f, 3.0f);
			ImGui::DragFloat("ディテールスケール", &desc_.detailScale_, 0.001f, 0.001f, 10.0f);
			ImGui::DragFloat("ディテール強度", &desc_.detailStrength_, 0.01f, 0.0f, 1.0f);

			ImGui::DragFloat("しきい値", &desc_.threshold_, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("ぼかし", &desc_.softness_, 0.005f, 0.0f, 1.0f);

			ImGui::Separator();
			ImGui::DragFloat("流れ速度（カメラ方向）", &desc_.flowSpeed_, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("上昇速度", &desc_.riseSpeed_, 0.01f, 0.0f, 10.0f);

			ImGui::DragFloat("最大アルファ", &desc_.alphaMax_, 0.01f, 0.0f, 1.0f);
		}
		ImGui::End();
	}
#endif
}