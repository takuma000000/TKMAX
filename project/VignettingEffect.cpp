#include "VignettingEffect.h"
#include "DirectXCommon.h"
#include <cmath>

void VignettingEffect::Initialize(DirectXCommon* dx) {
	BaseEffect::Initialize(dx);
	// パイプライン初期化
	dxCommon_->InitializeVignettingPipeline();
	// DX 側に自分を登録
	dxCommon_->SetVignettingEffect(this);
}

void VignettingEffect::Update(float dt) {
	if (!dxCommon_) { return; }

	// ボス Wave 中なら 1.0 までフェードイン、終わったら 0 までフェードアウト
	const float fadeSpeed = 2.0f; // 早さはお好み

	float target = inBossWave_ ? 1.0f : 0.0f;
	float delta = target - currentIntensity_;
	float step = fadeSpeed * dt;

	if (fabsf(delta) <= step) {
		currentIntensity_ = target;
	} else {
		currentIntensity_ += (delta > 0 ? step : -step);
	}

	active_ = (currentIntensity_ > 0.01f);

	// ボス Wave 中は radius を 0.25〜0.62 でうねらせる
	if (inBossWave_) {
		// アニメ用タイマーを進める（radiusAnimSpeed_ 往復/秒）
		const float twoPi = 6.28318530718f;
		radiusAnimT_ += dt * radiusAnimSpeed_ * twoPi;

		// -1〜+1 を 0〜1 にマップ
		float s = (std::sinf(radiusAnimT_) + 1.0f) * 0.5f;

		// 0.25〜0.62 の範囲に変換
		float rMin = radiusMin_;
		float rMax = radiusMax_;
		if (rMin > rMax) std::swap(rMin, rMax); // 一応の保険

		radius_ = rMin + (rMax - rMin) * s;
	}

	// DX 側にパラメータを送る（強度はフェード込）
	Vector4 col = color_;
	dxCommon_->SetVignettingParam(col,
		intensity_ * currentIntensity_,
		radius_,
		softness_);

#ifdef USE_IMGUI
	ImGuiDebug();
#endif
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
		// ■ 基本設定
		// =========================
		if (ImGui::CollapsingHeader("基本設定", ImGuiTreeNodeFlags_DefaultOpen)) {

			ImGui::Checkbox("ボス戦中扱い（デバッグ用）", &inBossWave_);

			float col[3] = { color_.x, color_.y, color_.z };
			if (ImGui::ColorEdit3("縁の色（Color）", col)) {
				color_.x = col[0];
				color_.y = col[1];
				color_.z = col[2];
			}

			ImGui::SliderFloat("強さ（Intensity）", &intensity_, 0.0f, 2.0f);
		}

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