#include "RadialBlurEffect.h"
#include "DirectXCommon.h"

namespace TKM {
	void RadialBlurEffect::Initialize(TKM::DirectXCommon* dx) {
		BaseEffect::Initialize(dx); // 基底クラスの初期化を呼び出す

		if (dxCommon_) {
			// コピー用パイプラインがまだなら初期化
			dxCommon_->InitializeCopyImagePipeline();
			// ラジアルブラー用パイプラインも初期化
			dxCommon_->InitializeRadialBlurPipeline();
		}
	}

	void RadialBlurEffect::BulrStartShock(float strength, float duration) {
		active_ = true; // エフェクトを有効にする
		timer_ = 0.0f; // タイマーをリセット
		maxStrength_ = strength; // エフェクトの最大強度を設定
		duration_ = duration; // エフェクトの持続時間を設定
	}

	void RadialBlurEffect::SetManualBlur(bool enable, float strength) {
		// 手動制御を有効にして、エフェクトの状態を設定
		manualControl_ = true;
		active_ = enable;
		manualStrength_ = strength;
		maxStrength_ = strength;

		if (!enable) {
			timer_ = 0.0f; // エフェクトを無効にする場合はタイマーもリセット
		}
	}

	void RadialBlurEffect::ClearManualBlur() {
		manualControl_ = false; // 手動制御を解除
		active_ = false; // エフェクトを無効にする
		timer_ = 0.0f; // タイマーもリセット
	}

	void RadialBlurEffect::Update(float dt) {
		// 手動制御中は timer_ で切らない
		if (manualControl_) {
			maxStrength_ = manualStrength_;
			return;
		}

		if (!active_) { return; }

		timer_ += dt;

		if (timer_ >= duration_) {
			active_ = false;
		}
	}

	void RadialBlurEffect::Draw() {

	}
}