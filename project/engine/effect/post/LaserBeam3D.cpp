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

		/// LaserBeamVolume用の定数バッファをセットして描画
		/// ビームの始点と終点、カメラの向きから、ビームの四隅のワールド座標を計算して描画します。
		///  内部で、これらのパラメータをもとにビームの四隅のワールド座標を計算し、専用のシェーダーで描画します。
		// - viewProj: カメラのビュー射影行列
		// - desc_.startWS_: ビームの始点のワールド座標
		// - desc_.endWS_: ビームの終点のワールド座標
		// - desc_.radius_: ビームの半径（太さ）
		// - camRightWS, camUpWS, camFwdWS: カメラの右、上、前方向のワールドベクトル
		// - desc_.sliceCount_: ビームを何枚のスライスで描くか
		// - time_: 経過時間（ノイズのアニメーションに使用）
		// - desc_.color_: ビームの色
		// - desc_.intensity_: ビームの発光強さ
		// - desc_.coreSharpness_: ビームの中心コアの締まり具合（大きいほど細く強い）
		// - desc_.edgeSoftness_: ビームの外側の落ち方
		// - desc_.noiseScale_: ビームのゆらぎのスケール
		// - desc_.noiseSpeed_: ビームのゆらぎの時間変化速度
		// - desc_.telegraph_: 予告モードかどうか（点滅弱めなどの効果を切り替えるためのフラグ）
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