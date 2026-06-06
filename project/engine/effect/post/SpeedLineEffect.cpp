#include "SpeedLineEffect.h"
#include "DirectXCommon.h"
#include <cmath>

namespace TKM {

	void SpeedLineEffect::Initialize(TKM::DirectXCommon* dx) {
		// BaseEffectの初期化を呼び出す
		BaseEffect::Initialize(dx);
		// スピード線エフェクト用のパイプラインをDirectX共通管理に登録する
		dxCommon_->InitializeSpeedLinePipeline();
		dxCommon_->SetSpeedLineEffect(this);
	}

	void SpeedLineEffect::Update(float dt) {
		if (!dxCommon_) { return; }

		time_ += dt; // 経過時間を更新

		// エフェクトの強さをフェードイン・アウトさせる
		const float target = requestActive_ ? 1.0f : 0.0f; // 目標の強さ。アクティブなら1.0f、非アクティブなら0.0f
		const float diff = target - currentIntensity_; // 現在の強さと目標の強さの差
		const float step = fadeSpeed_ * dt; // 1フレームで変化させる強さの量

		// 目標に近づくように強さを更新する。差が小さい場合は目標に直接設定する。
		if (std::fabs(diff) <= step) {
			currentIntensity_ = target;
		} else { // 目標に向かって段階的に強さを変化させる
			currentIntensity_ += (diff > 0.0f) ? step : -step;
		}
		// 強さが0.01f以上ならアクティブとみなす
		active_ = currentIntensity_ > 0.01f;

		// スピード線の方向を正規化してDirectX共通管理に渡す
		Vector2 dir = NormalizeDirection_(direction_);
		// DirectX共通管理にスピード線エフェクトのパラメータを設定する
		dxCommon_->SetSpeedLineParam(
			dir,
			intensity_ * currentIntensity_,
			time_,
			lineDensity_,
			lineSpeed_,
			lineWidth_
		);
	}

	Vector2 SpeedLineEffect::NormalizeDirection_(const Vector2& direction) const {
		// ベクトルの長さを計算する
		float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
		// 長さが非常に小さい場合は、デフォルトの方向 (1, 0) を返す
		if (length <= 0.001f) {
			return { 1.0f, 0.0f };
		}
		// ベクトルを長さで割って正規化する
		return {
			direction.x / length,
			direction.y / length
		};
	}
}