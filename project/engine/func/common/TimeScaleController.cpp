#include "TimeScaleController.h"

namespace TKM {

	void TimeScaleController::Initialize() {
		// 通常速度で初期化する
		currentScale_ = 1.0f;
		targetScale_ = 1.0f;

		// スロー維持時間を初期化する
		hold_ = 0.0f;

		// フェーズ内タイマーを初期化する
		timer_ = 0.0f;

		// スロー入り・スロー戻りの補間時間を初期化する
		blendIn_ = 0.0f;
		blendOut_ = 0.0f;

		// 初期状態ではスロー処理なしにする
		phase_ = Phase::None;
	}

	void TimeScaleController::RequestSlow(float scale, float duration) {
		// durationが負にならないように補正する
		float dur = std::max(0.0f, duration);

		// 短く入り、少し長めに戻すスロー補間時間を自動計算する
		float blendIn = std::min(0.06f, dur * 0.25f);
		float blendOut = std::clamp(dur * 0.80f, 0.10f, 0.40f);

		// 詳細指定版へ流して、実際のスロー開始処理を行う
		RequestSlowAdvanced(scale, duration, blendIn, blendOut);
	}

	void TimeScaleController::RequestSlowAdvanced(float scale, float duration, float blendIn, float blendOut) {
		// 目標スケールを0.0f～1.0fに収める
		targetScale_ = std::clamp(scale, 0.0f, 1.0f);

		// スロー維持時間を負にならないように保存する
		hold_ = std::max(0.0f, duration);

		// 補間時間を負にならないように保存する
		blendIn_ = std::max(0.0f, blendIn);
		blendOut_ = std::max(0.0f, blendOut);

		// スロー開始用にタイマーをリセットする
		timer_ = 0.0f;

		// まずは通常速度から目標スロー倍率へ入っていく
		phase_ = Phase::BlendIn;
	}

	void TimeScaleController::Update(float dt) {
		switch (phase_) {
		case Phase::None: {
			// スロー演出がない時は通常速度を維持する
			currentScale_ = 1.0f;
			break;
		}
		case Phase::BlendIn: {
			// スローへ入る補間時間を進める
			timer_ += dt;

			// blendInが0以下なら即座に目標スケールへ到達させる
			float t = (blendIn_ <= 0.0f) ? 1.0f : std::clamp(timer_ / blendIn_, 0.0f, 1.0f);

			// 通常速度から目標スロー倍率へ補間する
			currentScale_ = MyMath::Lerp(1.0f, targetScale_, t);

			// スロー入りが完了したら、維持フェーズへ進む
			if (t >= 1.0f) {
				phase_ = Phase::Hold;
				timer_ = 0.0f;
			}
			break;
		}
		case Phase::Hold: {
			// スロー維持時間を進める
			timer_ += dt;

			// 維持中は目標スロー倍率をそのまま使う
			currentScale_ = targetScale_;

			// 維持時間が終わったら、通常速度へ戻すフェーズへ進む
			if (timer_ >= hold_) {
				phase_ = Phase::BlendOut;
				timer_ = 0.0f;
			}
			break;
		}
		case Phase::BlendOut: {
			// 通常速度へ戻る補間時間を進める
			timer_ += dt;

			// blendOutが0以下なら即座に通常速度へ戻す
			float t = (blendOut_ <= 0.0f) ? 1.0f : std::clamp(timer_ / blendOut_, 0.0f, 1.0f);

			// 目標スロー倍率から通常速度へ補間する
			currentScale_ = MyMath::Lerp(targetScale_, 1.0f, t);

			// 通常速度へ戻り切ったら、スローなし状態へ戻す
			if (t >= 1.0f) {
				phase_ = Phase::None;
				timer_ = 0.0f;
				currentScale_ = 1.0f;
			}
			break;
		}
		}
	}

} // namespace TKM