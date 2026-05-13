#include "MotionBlurEffect.h"
#include "DirectXCommon.h"

namespace TKM {
	void MotionBlurEffect::Initialize(TKM::DirectXCommon* dx) {
		BaseEffect::Initialize(dx); // BaseEffectのInitializeを呼び出してdxCommon_にセット

		// モーションブラー用のパイプラインをDirectXCommonに登録
		dxCommon_->InitializeMotionBlurPipeline();
		dxCommon_->SetMotionBlurEffect(this);
	}

	void MotionBlurEffect::Update(float dt) {
		dt;
		if (!dxCommon_) { return; }

		dxCommon_->SetMotionBlurParam(strength_);
	}

	void MotionBlurEffect::ImGuiDebug() {
#ifdef USE_IMGUI
		if (ImGui::Begin("モーションブラー")) {
			ImGui::Checkbox("有効", &active_);
			ImGui::SliderFloat("強度", &strength_, 0.0f, 0.95f);
		}
		ImGui::End();
#endif
	}
}