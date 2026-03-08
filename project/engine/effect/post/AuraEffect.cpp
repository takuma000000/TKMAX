#include "AuraEffect.h"
#include "DirectXCommon.h"

namespace TKM {
	void AuraEffect::Initialize(TKM::DirectXCommon* dxCommon) {
		BaseEffect::Initialize(dxCommon); // 基底クラスの初期化を呼び出す
		active_ = false; // エフェクトは初期状態では非アクティブ
		// エフェクトの初期パラメータを設定
		auraVolumeRenderer_.Initialize(dxCommon_); // AuraVolumeRenderer の初期化
	}

	void AuraEffect::Update(float dt) {
		time_ += dt;
		if (!dxCommon_) { return; }
		if (!active_) { return; }
		PushToGpu(); // GPUにパラメータを送る
	}

	void AuraEffect::PushToGpu() {
		if (!dxCommon_) { return; }

		/// AuraCBにパラメータをセットしてGPUに送る
		/// ここでは、AuraCB構造体にエフェクトのパラメータをセットして、DirectXCommonのSetAuraParam関数を呼び出してGPUに送ります。
		/// 具体的には、以下のようなパラメータをセットしています。
		// - centerUV_: エフェクトの中心のUV座標
		// - topUV_: ボス頭のUV座標
		// - bottomUV_: ボス足のUV座標
		// - aspect_: 画面のアスペクト比
		// - time_: 経過時間
		// - scale_: エフェクトのスケール
		// - intensity_: エフェクトの明るさ
		// - useRing_: リングを使うかどうか
		// -ringRadius_: リングの半径
		// - ringWidth_: リングの幅
		// - colorA_: 色A
		// - colorB_: 色B
		// - mix_: 色の混ぜ具合
		// - taper_: 上に行くほど細くする度合い
		// - noiseScale_: 炎のノイズの細かさ
		// - noiseSpeed_: 炎のノイズの速さ
		// - flameStrength_: 炎の立ち上がりの強さ
		// - edgePower_: 外周のキレ
		// - verticalFade_: 上下のフェードの強さ
		dxCommon_->SetAuraParam(
			centerUV_,
			topUV_,
			bottomUV_,
			aspect_,
			time_,
			scale_,
			intensity_,
			useRing_ ? 1.0f : 0.0f,
			ringRadius_,
			ringWidth_,
			colorA_,
			colorB_,
			mix_,
			taper_,
			noiseScale_,
			noiseSpeed_,
			flameStrength_,
			edgePower_,
			verticalFade_
		);
	}

#ifdef USE_IMGUI
	void AuraEffect::ImGuiDebug() {
		ImGui::Begin("AuraEffect");

		ImGui::Checkbox("Active", &active_);
		ImGui::DragFloat2("CenterUV", &centerUV_.x, 0.001f, 0.0f, 1.0f);
		ImGui::DragFloat("Scale", &scale_, 0.001f, 0.01f, 1.0f);
		ImGui::DragFloat("Intensity", &intensity_, 0.01f, 0.0f, 10.0f);

		ImGui::Checkbox("UseRing", &useRing_);
		ImGui::DragFloat("RingRadius", &ringRadius_, 0.001f, 0.0f, 1.0f);
		ImGui::DragFloat("RingWidth", &ringWidth_, 0.1f, 1.0f, 80.0f);

		ImGui::ColorEdit3("ColorA", &colorA_.x);
		ImGui::ColorEdit3("ColorB", &colorB_.x);
		ImGui::DragFloat("Mix", &mix_, 0.01f, 0.0f, 1.0f);

		ImGui::End();
	}
#endif
} // namespace TKM