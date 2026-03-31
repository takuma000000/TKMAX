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
		screenW_ = screenW;
		screenH_ = screenH;

		sprite_ = std::make_unique<Sprite>();
		sprite_->Initialize(spriteCommon, dxCommon, "./resources/texture/skip.png");

		sprite_->SetAnchorPoint({ 0.5f,0.5f }); // 中心基準

		// サイズ取得
		const auto& meta = TextureManager::GetInstance()->GetMetadata("./resources/texture/skip.png");
		baseSize_ = { (float)meta.width, (float)meta.height }; // 元サイズを保存
		sprite_->SetSize(baseSize_); // 元サイズでセット

		// 右下基準で配置
		basePos_ = {
			screenW_ + offset_.x,
			screenH_ + offset_.y
		};
		// 初期位置をセット
		sprite_->SetPosition(basePos_);
	}

	void SkipGuideUI::Update(float dt, bool canSkip) {
		if (!sprite_) {
			return;
		}

		if (!canSkip) {
			holdTimer_ = 0.0f;

			// スキップ不可時は通常状態に戻しておく
			sprite_->SetSize({
				baseSize_.x * normalScale_,
				baseSize_.y * normalScale_
				});

			sprite_->SetColor(normalColor_);

			basePos_ = {
				screenW_ + offset_.x,
				screenH_ + offset_.y
			};
			sprite_->SetPosition(basePos_);
			sprite_->Update();
			return;
		}

		const bool pressed =
			Input::GetInstance()->PushKey(DIK_SPACE) ||
			Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A);

		if (pressed) {
			holdTimer_ += dt;
			if (holdTimer_ > kHoldTime_) {
				holdTimer_ = kHoldTime_;
			}
		} else {
			holdTimer_ = 0.0f;
		}

		const float t = holdTimer_ / kHoldTime_;

		// スケール補間
		const float scale = normalScale_ + (pressScale_ - normalScale_) * t;
		sprite_->SetSize({
			baseSize_.x * scale,
			baseSize_.y * scale
			});

		// 色補間
		Vector4 col = {
			normalColor_.x + (pressColor_.x - normalColor_.x) * t,
			normalColor_.y + (pressColor_.y - normalColor_.y) * t,
			normalColor_.z + (pressColor_.z - normalColor_.z) * t,
			normalColor_.w + (pressColor_.w - normalColor_.w) * t
		};
		sprite_->SetColor(col);

		// 右下基準位置を毎フレーム反映
		basePos_ = {
			screenW_ + offset_.x,
			screenH_ + offset_.y
		};
		sprite_->SetPosition(basePos_);
		sprite_->Update();
	}

	void SkipGuideUI::Draw(float alpha) {
		if (!sprite_) {
			return;
		}

		Vector4 col = sprite_->GetColor();
		const float originalAlpha = col.w;
		col.w = originalAlpha * alpha;
		sprite_->SetColor(col);
		sprite_->Draw();

		// 描画後に元へ戻す
		col.w = originalAlpha;
		sprite_->SetColor(col);
	}

	void SkipGuideUI::DrawImGui() {
#ifdef USE_IMGUI
		if (!ImGui::TreeNode("SkipGuideUI")) {
			return;
		}

		ImGui::DragFloat2("右下オフセット", &offset_.x, 1.0f, -1000.0f, 1000.0f);
		ImGui::DragFloat("通常スケール", &normalScale_, 0.01f, 0.1f, 3.0f);
		ImGui::DragFloat("押下スケール", &pressScale_, 0.01f, 0.1f, 3.0f);

		ImGui::ColorEdit4("通常色", &normalColor_.x);
		ImGui::ColorEdit4("押下色", &pressColor_.x);

		ImGui::Text("HoldTimer: %.2f / %.2f", holdTimer_, kHoldTime_);

		ImGui::TreePop();
#endif
	}
}