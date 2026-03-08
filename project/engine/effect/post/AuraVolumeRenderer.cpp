#include "AuraVolumeRenderer.h"
#include "DirectXCommon.h"

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace TKM {
	void AuraVolumeRenderer::Initialize(TKM::DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;
		if (!dxCommon_) { return; }
		dxCommon_->InitializeAuraVolumePipeline();

		//alwaysOn_ = true; // デバッグ用：常にONにする
	}

	void AuraVolumeRenderer::Draw(
		const Matrix4x4& viewProj,
		const Vector3& bossCenter,
		const Vector3& collider,
		bool active)
	{
		if (!dxCommon_) { return; }

		//  強制ONなら active を無視する
		if (!alwaysOn_ && !active) { return; }

		drawCallsThisFrame_++;

		time_ += 1.0f / 60.0f;

		float radius = (collider.x * 0.5f) * radiusMul_;
		float height = (collider.y * 0.5f) * heightMul_;

		Vector3 basePos = bossCenter - Vector3{ 0.0f, height * 0.5f, 0.0f }; // ボスの中心から下半分移動した位置を基準にする

		/// AuraVolume用の定数バッファをセットして描画
		/// これらのパラメータを組み合わせて、炎のような動きのあるオーラを表現する
		// - viewProj --- カメラのビュー射影行列
		// - basePos --- ワールド空間でのオーラの中心位置
		// - radius --- オーラの半径
		// - height --- オーラの高さ
		// - sliceCount --- オーラを何枚の板で構成するか（多いほど丸く見えるが重い）
		// - time --- 時間（ノイズの動きに使用）
		// - color --- オーラの色
		// - intensity --- オーラの明るさ
		// - noiseScale --- ノイズの細かさ
		// - noiseSpeed --- ノイズの速さ
		// - rimPower --- 外周のキレ
		// - alphaBase --- 全体の透明度のベース（0.0で完全に透明、1.0で通常の不透明）
		dxCommon_->DrawAuraVolume(
			viewProj,
			basePos,
			radius,
			height,
			sliceCount_,
			time_,
			color_,
			intensity_,
			noiseScale_,
			noiseSpeed_,
			rimPower_,
			alphaBase_
		);
	}

#ifdef USE_IMGUI
	void AuraVolumeRenderer::DrawImGui(const char* label) {
		if (ImGui::Begin(label)) {

			//  これが 2 以上なら「同フレームに2回呼ばれてる」= 二重描画
			ImGui::Text("DrawCallsThisFrame: %u", drawCallsThisFrame_);

			ImGui::Checkbox("Always On", &alwaysOn_);

			// フレームごとに表示後リセット（このUIを毎フレーム呼ぶ前提）
			drawCallsThisFrame_ = 0;

			// ---- 見た目調整 ----
			float col[3] = { color_.x, color_.y, color_.z };
			if (ImGui::ColorEdit3("Color", col)) {
				color_ = { col[0], col[1], col[2] };
			}

			ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 10.0f);
			ImGui::SliderFloat("AlphaBase", &alphaBase_, 0.0f, 1.0f);

			ImGui::SliderFloat("RadiusMul", &radiusMul_, 0.6f, 2.5f);
			ImGui::SliderFloat("HeightMul", &heightMul_, 0.6f, 3.0f);

			int slices = (int)sliceCount_;
			if (ImGui::SliderInt("SliceCount", &slices, 4, 32)) {
				sliceCount_ = (uint32_t)slices;
			}

			ImGui::Separator();
			ImGui::SliderFloat("NoiseScale", &noiseScale_, 0.1f, 30.0f);
			ImGui::SliderFloat("NoiseSpeed", &noiseSpeed_, 0.0f, 6.0f);
			ImGui::SliderFloat("RimPower", &rimPower_, 0.1f, 8.0f);

			ImGui::Separator();
			ImGui::Text("Tip: If it looks like 2 auras, try SliceCount down (8-12) and AlphaBase <= 0.6");
		}
		ImGui::End();
	}
#endif
}