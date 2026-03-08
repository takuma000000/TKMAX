#define NOMINMAX
#include "WaterRippleEffect.h"
#include "DirectXCommon.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {
	void WaterRippleEffect::Initialize(TKM::DirectXCommon* dx) {
		TKM::BaseEffect::Initialize(dx); // 基底クラスの初期化
	}

	void WaterRippleEffect::Update(float dt) {
		time_ += dt;

		// 波紋がアクティブでない場合は、パラメータをリセットして終了
		if (!active_) {
			dxCommon_->SetWaterRippleParam(
				centerUV_,
				0.0f,
				0.0f,
				currentDesc_.frequency_,
				currentDesc_.width_,
				currentDesc_.color_,
				0.0f
			);
			return;
		}

		float dur = std::max(0.0001f, currentDesc_.duration_); // duration_ が0のときに割り算で落ちないようにする
		float t = time_ / dur; // tは0から1に変化する値。1を超えたらエフェクト終了

		// tが1以上になったらエフェクトを終了させる
		if (t >= 1.0f) {
			active_ = false; // エフェクト終了
			t = 1.0f; // tを1にクランプして、最後の状態を描画する
		}

		float radius = currentDesc_.radiusMax_ * t; // 波紋の半径は時間とともに大きくなる
		float amp = currentDesc_.amplitude_ * (1.0f - t); // ゆがみ量は時間とともに減少する

		/// DirectXCommonに波紋のパラメータを送る
		dxCommon_->SetWaterRippleParam(
			centerUV_,
			radius,
			amp,
			currentDesc_.frequency_,
			currentDesc_.width_,
			currentDesc_.color_,
			currentDesc_.colorIntensity_
		);
	}

	void WaterRippleEffect::Trigger(const Vector2& centerUV, const RippleDesc& desc) {
		centerUV_ = centerUV;
		currentDesc_ = desc; 
		time_ = 0.0f; // 経過時間をリセットして、エフェクト開始
		active_ = true; // エフェクトをアクティブにする
	}
}