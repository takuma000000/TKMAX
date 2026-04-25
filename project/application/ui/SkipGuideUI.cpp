#include "SkipGuideUI.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void SkipGuideUI::Initialize(
		SpriteCommon* spriteCommon,
		DirectXCommon* dxCommon,
		float screenW,
		float screenH
	) {
		// 画面サイズを保存する
		screenW_ = screenW;
		screenH_ = screenH;

		// スキップ案内用スプライトを生成する
		sprite_ = std::make_unique<Sprite>();
		sprite_->Initialize(spriteCommon, dxCommon, "./resources/texture/skip.png");

		// 拡大縮小時に中心を基準にする
		sprite_->SetAnchorPoint({ 0.5f,0.5f });

		// テクスチャサイズを取得して、元サイズとして保存する
		const auto& meta = TextureManager::GetInstance()->GetMetadata("./resources/texture/skip.png");
		baseSize_ = { (float)meta.width, (float)meta.height };

		// 初期状態では元サイズで表示する
		sprite_->SetSize(baseSize_);

		// 右下基準の配置位置を計算する
		basePos_ = {
			screenW_ + offset_.x,
			screenH_ + offset_.y
		};

		// 初期位置を反映する
		sprite_->SetPosition(basePos_);
	}

	void SkipGuideUI::Update(float dt, bool canSkip) {
		// スプライトが未生成なら更新しない
		if (!sprite_) {
			return;
		}

		// スキップ不可の間は、見た目とホールド時間を通常状態へ戻す
		if (!canSkip) {
			holdTimer_ = 0.0f;

			// 通常スケールへ戻す
			sprite_->SetSize({
				baseSize_.x * normalScale_,
				baseSize_.y * normalScale_
				});

			// 通常色へ戻す
			sprite_->SetColor(normalColor_);

			// 右下基準位置を再計算して反映する
			basePos_ = {
				screenW_ + offset_.x,
				screenH_ + offset_.y
			};

			sprite_->SetPosition(basePos_);
			sprite_->Update();
			return;
		}

		// キーボードSPACE、またはゲームパッドAでスキップ入力を判定する
		const bool pressed =
			Input::GetInstance()->PushKey(DIK_SPACE) ||
			Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A);

		// 押している間だけホールド時間を進める
		if (pressed) {
			holdTimer_ += dt;

			// 必要ホールド時間を超えないように丸める
			if (holdTimer_ > kHoldTime_) {
				holdTimer_ = kHoldTime_;
			}
		} else {
			// 離したらホールドを最初からやり直す
			holdTimer_ = 0.0f;
		}

		// ホールド進行率を0.0f～1.0fで扱う
		const float t = holdTimer_ / kHoldTime_;

		// ホールド進行率に応じて、通常スケールから押下スケールへ補間する
		const float scale = normalScale_ + (pressScale_ - normalScale_) * t;
		sprite_->SetSize({
			baseSize_.x * scale,
			baseSize_.y * scale
			});

		// ホールド進行率に応じて、通常色から押下色へ補間する
		Vector4 col = {
			normalColor_.x + (pressColor_.x - normalColor_.x) * t,
			normalColor_.y + (pressColor_.y - normalColor_.y) * t,
			normalColor_.z + (pressColor_.z - normalColor_.z) * t,
			normalColor_.w + (pressColor_.w - normalColor_.w) * t
		};

		sprite_->SetColor(col);

		// ImGuiなどでオフセットが変わっても追従できるように、毎フレーム右下基準位置を再計算する
		basePos_ = {
			screenW_ + offset_.x,
			screenH_ + offset_.y
		};

		// 位置と内部状態を更新する
		sprite_->SetPosition(basePos_);
		sprite_->Update();
	}

	void SkipGuideUI::Draw(float alpha) {
		// スプライトが未生成なら描画しない
		if (!sprite_) {
			return;
		}

		// 現在色を取得し、HUD全体アルファを一時的に掛ける
		Vector4 col = sprite_->GetColor();
		const float originalAlpha = col.w;
		col.w = originalAlpha * alpha;

		// アルファ適用後の色で描画する
		sprite_->SetColor(col);
		sprite_->Draw();

		// 描画後は元のアルファへ戻して、次フレーム以降へ影響を残さない
		col.w = originalAlpha;
		sprite_->SetColor(col);
	}

	void SkipGuideUI::DrawImGui() {
#ifdef USE_IMGUI
		// SkipGuideUIの調整項目を開いていない場合は表示しない
		if (!ImGui::TreeNode("SkipGuideUI")) {
			return;
		}

		// 右下基準からの表示位置を調整する
		ImGui::DragFloat2("右下オフセット", &offset_.x, 1.0f, -1000.0f, 1000.0f);

		// 通常時と押下時のサイズを調整する
		ImGui::DragFloat("通常スケール", &normalScale_, 0.01f, 0.1f, 3.0f);
		ImGui::DragFloat("押下スケール", &pressScale_, 0.01f, 0.1f, 3.0f);

		// 通常時と押下時の色を調整する
		ImGui::ColorEdit4("通常色", &normalColor_.x);
		ImGui::ColorEdit4("押下色", &pressColor_.x);

		// 現在のホールド進行状況を確認する
		ImGui::Text("HoldTimer: %.2f / %.2f", holdTimer_, kHoldTime_);

		ImGui::TreePop();
#endif
	}

} // namespace TKM