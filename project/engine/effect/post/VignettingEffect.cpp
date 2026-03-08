#include "VignettingEffect.h"
#include "DirectXCommon.h"
#include <cmath>

namespace TKM {
	void VignettingEffect::Initialize(TKM::DirectXCommon* dx) {
		BaseEffect::Initialize(dx);
		// パイプライン初期化
		dxCommon_->InitializeVignettingPipeline();
		// DX 側に自分を登録
		dxCommon_->SetVignettingEffect(this);
	}

	void VignettingEffect::Update(float dt) {
		if (!dxCommon_) { return; }

		// 低HP 状態なら 1.0 までフェードイン、終わったら 0 までフェードアウト
		const float fadeSpeed = 2.0f; // 早さはお好み

		float target = (inLowHP_) ? 1.0f : 0.0f; // 目標値
		float delta = target - currentIntensity_; // 目標との差
		float step = fadeSpeed * dt; // 今フレームで変化させる量

		// 目標に近づくように currentIntensity_ を更新
		if (fabsf(delta) <= step) {
			currentIntensity_ = target;
		} else { // 目標にまだ遠い場合は、step 分だけ近づける
			currentIntensity_ += (delta > 0 ? step : -step);
		}
		// 強度が十分にあるかどうかで有効・無効を切り替える
		active_ = (currentIntensity_ > 0.01f);

		// DX 側にパラメータを送る（強度はフェード込）
		Vector4 col = color_;
		float intensity = intensity_;
		float radius = radius_;
		float softness = softness_;

		// 低HP（危険）時は赤いビネットに切り替える
		if (inLowHP_) {
			col = lowHPColor_;
			intensity = lowHPIntensity_;
			softness = lowHPSoftness_;

			// 低HP中：覆う範囲（radius）を Min〜Max で常時往復させる
			const float twoPi = 6.28318530718f;
			lowHPPulseT_ += dt * lowHPPulseSpeed_ * twoPi;

			// 0..1 にしてから Min〜Max に変換（常に行ったり来たり）
			float t = (std::sinf(lowHPPulseT_) + 1.0f) * 0.5f; // 0..1
			radius = lowHPRadiusMin_ + (lowHPRadiusMax_ - lowHPRadiusMin_) * t;
		}

		// ボス戦中は半径を Min〜Max で往復させる
		dxCommon_->SetVignettingParam(col,
			intensity * currentIntensity_,
			radius,
			softness);
	}

	void VignettingEffect::ImGuiDebug() {
#ifdef USE_IMGUI
		// ◆ 1回だけ「位置」と「最初のサイズ」を指定（毎フレームじゃない）
		ImGui::SetNextWindowPos(ImVec2(10.0f, 380.0f), ImGuiCond_Once);   // 画面左下寄りとか、好みで
		ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_Once);   // 幅だけ決めて高さは自動

		if (ImGui::Begin("ビネット（Vignetting）", nullptr,
			ImGuiWindowFlags_NoCollapse)) {

			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
				"ビネット設定");
			ImGui::Spacing();

			// 横幅をそろえて見やすく
			ImGui::PushItemWidth(220.0f);

			// =========================
			// ■ 通常時パラメータ
			// =========================
			if (ImGui::CollapsingHeader("通常時パラメータ", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderFloat("基準半径（Radius Base）", &radius_, 0.0f, 1.0f);
				ImGui::SliderFloat("ぼかし幅（Softness）", &softness_, 0.0f, 1.0f);
			}

			// =========================
			// ■ ボス戦中の半径ゆらぎ
			// =========================
			if (ImGui::CollapsingHeader("ボス戦中の半径ゆらぎ", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SliderFloat("最小半径（Min）", &radiusMin_, 0.0f, 1.0f);
				ImGui::SliderFloat("最大半径（Max）", &radiusMax_, 0.0f, 1.0f);
				ImGui::SliderFloat("揺れる速さ（Speed）", &radiusAnimSpeed_, 0.1f, 5.0f);
			}

			ImGui::PopItemWidth();

			ImGui::Separator();
			ImGui::Text("現在のフェード値：%.2f", currentIntensity_);
		}
		ImGui::End();
#endif
	}
}