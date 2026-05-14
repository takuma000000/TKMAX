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
		if (!dxCommon_) { return; }

		if (!active_) {
			return;
		}

		if (burstActive_) {
			burstTimer_ += dt;

			float t = burstTimer_ / burstDuration_;
			if (t >= 1.0f) {
				Stop();
				return;
			}

			float ease = 1.0f - t;
			strength_ = burstStartStrength_ * ease;
		}

		dxCommon_->SetMotionBlurParam(strength_);
	}

	void MotionBlurEffect::StartBurst(float strength, float duration) {
		active_ = true; // バースト開始と同時に有効化
		burstActive_ = true; // バースト状態にする

		burstTimer_ = 0.0f; // バーストタイマーをリセット
		burstDuration_ = duration; // バーストの持続時間を設定
		burstStartStrength_ = strength; // バースト開始時の強度を設定
		strength_ = strength; // 現在の強度を設定
	}

	void MotionBlurEffect::Stop() {
		active_ = false;
		burstActive_ = false;

		burstTimer_ = 0.0f;
		burstDuration_ = 0.0f;
		burstStartStrength_ = 0.0f;
		strength_ = 0.0f;

		// DirectXCommonに強度0をセットしてモーションブラーを停止
		if (dxCommon_) {
			dxCommon_->SetMotionBlurParam(0.0f);
		}
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