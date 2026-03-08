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

		desc_.worldPos_ = desc_.centerWS_;

		/// DirectXCommonのDrawFogVolume関数を呼び出して、空間霧を描画します。
		/// これらのパラメータを渡すことで、DirectXCommon側で空間霧の描画処理が行われます。
		// - viewProj: カメラのビュー射影行列
		// - desc_.centerWS_: 霧の中心座標（ワールド空間）
		// - desc_.halfSizeWS_: 霧の範囲サイズ（半径）
		// - camRightWS, camUpWS, camFwdWS: カメラの右、上、前方向ベクトル（ワールド空間）
		// - desc_.sliceCount_: 霧の重なり枚数（スライス数）
		// - time_: 経過時間
		// - desc_.color_: 霧の色
		// - desc_.density_: 霧の濃さ
		// - desc_.noiseScale_: ノイズの細かさ
		// - desc_.noiseSpeed_: ノイズの速さ
		// - desc_.softness_: 端のぼかし具合
		// - desc_.fogStart_: 霧開始の高さ
		// - desc_.fogEnd_: 霧最大の高さ
		// - desc_.noiseStrength_: ノイズのムラの強さ
		// - desc_.worldScale_: 霧パターンの「世界空間スケール」
		// - desc_.worldPos_: ノイズの基準座標（通常はカメラ位置や霧の中心に同期）
		dxCommon_->DrawFogVolume(
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
			desc_.noiseScale_,
			desc_.noiseSpeed_,
			desc_.softness_,
			desc_.fogStart_,
			desc_.fogEnd_,
			desc_.noiseStrength_,
			desc_.worldScale_,
			desc_.worldPos_
		);
	}

#ifdef USE_IMGUI
	void FogVolume3D::ImGuiDebug() {
		if (!active_) { return; }

		ImGui::Begin("霧(空間)");
		if (ImGui::CollapsingHeader("空間霧（FogVolume3D）", ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::Checkbox("有効 / 無効", &active_);

			ImGui::DragFloat3("中心座標（ワールド）", &desc_.centerWS_.x, 0.5f);
			ImGui::DragFloat3("範囲サイズ（半径）", &desc_.halfSizeWS_.x, 1.0f, 1.0f, 3000.0f);

			ImGui::ColorEdit3("霧の色", &desc_.color_.x);
			ImGui::DragFloat("霧の濃さ", &desc_.density_, 0.001f, 0.0f, 1.0f);

			ImGui::DragInt("重なり枚数（スライス数）", (int*)&desc_.sliceCount_, 1.0f, 1, 256);

			ImGui::DragFloat("ノイズスケール", &desc_.noiseScale_, 0.001f, 0.0f, 1.0f);
			ImGui::DragFloat("ノイズ速度", &desc_.noiseSpeed_, 0.01f, 0.0f, 5.0f);
			ImGui::DragFloat("端のぼかし具合", &desc_.softness_, 0.05f, 0.1f, 10.0f);
		}
		ImGui::End();
	}
#endif
}