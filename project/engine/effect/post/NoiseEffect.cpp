#include "NoiseEffect.h"
#include "DirectXCommon.h"

namespace TKM {

	void NoiseEffect::Initialize(TKM::DirectXCommon* dx) {
		BaseEffect::Initialize(dx);

		// パイプライン初期化
		dxCommon_->InitializeNoisePipeline();

		// DX 側に自分を登録
		dxCommon_->SetNoiseEffect(this);
	}

	void NoiseEffect::Update(float dt) {
		if (!dxCommon_) { return; }

		time_ += dt;

		if (!active_) {
			// 無効時は強度0だけ送っておく
			dxCommon_->SetNoiseParam(
				time_,
				0.0f,
				lineDensity_,
				lineSpeed_,
				blockScale_,
				blockShift_,
				rgbShift_,
				flash_,
				Vector2(
					static_cast<float>(WindowsAPI::kClientWidth_),
					static_cast<float>(WindowsAPI::kClientHeight_)
				)
			);
			return;
		}

		dxCommon_->SetNoiseParam(
			time_,
			intensity_,
			lineDensity_,
			lineSpeed_,
			blockScale_,
			blockShift_,
			rgbShift_,
			flash_,
			Vector2(
				static_cast<float>(WindowsAPI::kClientWidth_),
				static_cast<float>(WindowsAPI::kClientHeight_)
			)
		);
	}

#ifdef USE_IMGUI
	void NoiseEffect::ImGuiDebug() {
		ImGui::SetNextWindowPos(ImVec2(10.0f, 520.0f), ImGuiCond_Once);
		ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_Once);

		if (ImGui::Begin("ノイズ（Noise）", nullptr, ImGuiWindowFlags_NoCollapse)) {
			ImGui::Checkbox("有効", &active_);
			ImGui::SliderFloat("強度", &intensity_, 0.0f, 2.0f);
			ImGui::SliderFloat("横線密度", &lineDensity_, 10.0f, 400.0f);
			ImGui::SliderFloat("横線速度", &lineSpeed_, 0.0f, 30.0f);
			ImGui::SliderFloat("ブロックサイズ", &blockScale_, 4.0f, 128.0f);
			ImGui::SliderFloat("ブロックずれ", &blockShift_, 0.0f, 0.15f);
			ImGui::SliderFloat("RGBずれ", &rgbShift_, 0.0f, 0.03f);
			ImGui::SliderFloat("フラッシュ", &flash_, 0.0f, 0.5f);
		}
		ImGui::End();
	}
#endif
}