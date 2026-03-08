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

	void RadialBlurEffect::Update(float dt) {
		if (!active_) { return; }

		timer_ += dt;

		// エフェクトの強度は、経過時間に応じて0からmaxStrength_まで変化させる（例: 線形に減衰させる）
		if (timer_ >= duration_) {
			active_ = false; // エフェクトの持続時間を超えたら無効にする
		}
	}

	void RadialBlurEffect::Draw() {

	}
}