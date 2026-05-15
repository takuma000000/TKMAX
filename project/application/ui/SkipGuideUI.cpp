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
		sprite_->SetAutoAdjustTextureSize(false);

		const auto& meta = TextureManager::GetInstance()->GetMetadata("./resources/texture/skip.png");
		baseSize_ = { (float)meta.width, (float)meta.height };

		sprite_->SetTextureLeftTop({ 0.0f, 0.0f });
		sprite_->SetTextureSize(baseSize_);

		sprite_->SetAnchorPoint({ 0.5f, 0.5f });
		sprite_->SetSize({
			baseSize_.x * normalScale_,
			baseSize_.y * normalScale_
			});

		// 右下基準の配置位置を計算する
		basePos_ = {
			screenW_ + offset_.x,
			screenH_ + offset_.y
		};

		// 初期位置を反映する
		sprite_->SetPosition(basePos_);

		// ゲージ用スプライトを生成する
		gaugeSprite_ = std::make_unique<Sprite>();
		gaugeSprite_->Initialize(spriteCommon, dxCommon, "./resources/texture/skip_gauge.png");
		gaugeSprite_->SetAutoAdjustTextureSize(false);

		const auto& gaugeMeta = TextureManager::GetInstance()->GetMetadata("./resources/texture/skip_gauge.png");
		gaugeTexSize_ = { (float)gaugeMeta.width, (float)gaugeMeta.height };

		gaugeSprite_->SetTextureLeftTop({ 0.0f, 0.0f });
		gaugeSprite_->SetTextureSize(gaugeTexSize_);

		gaugeSprite_->SetAnchorPoint({ 0.0f, 0.5f });
		gaugeSprite_->SetSize({ 0.0f, gaugeMaxSize_.y });
		gaugeSprite_->SetColor(gaugeColor_);
		// ゲージの発光を設定する
		gaugeSprite_->SetGlowParams(
			true,
			gaugeGlowColor_,
			gaugeGlowIntensity_,
			gaugeGlowWidth_,
			gaugeGlowThreshold_,
			gaugeGlowSoftness_
		);
	}

	void SkipGuideUI::Update(float dt, bool canSkip) {
		// スプライトが未生成なら更新しない
		if (!sprite_) {
			return;
		}

		// ImGuiなどでオフセットが変わっても追従できるように、毎フレーム右下基準位置を再計算する
		basePos_ = {
			screenW_ + offset_.x,
			screenH_ + offset_.y
		};

		// スキップ不可の間は、見た目とホールド時間を通常状態へ戻す
		if (!canSkip) {
			holdTimer_ = 0.0f;
			skipCompleted_ = false;

			// skip.png本体は色もサイズも変えない
			sprite_->SetSize({
				baseSize_.x * normalScale_,
				baseSize_.y * normalScale_
				});

			sprite_->SetPosition(basePos_);
			sprite_->Update();

			// ゲージは横幅0で非表示にする
			if (gaugeSprite_) {
				const Vector2 gaugePos = {
					basePos_.x + gaugeOffset_.x,
					basePos_.y + gaugeOffset_.y
				};

				gaugeSprite_->SetPosition(gaugePos);
				gaugeSprite_->SetSize({ 0.0f, gaugeMaxSize_.y });
				gaugeSprite_->SetColor(gaugeColor_);
				// ゲージの発光を設定する
				gaugeSprite_->SetGlowParams(
					true,
					gaugeGlowColor_,
					gaugeGlowIntensity_,
					gaugeGlowWidth_,
					gaugeGlowThreshold_,
					gaugeGlowSoftness_
				);
				gaugeSprite_->Update();
			}

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

		// ゲージが最大まで溜まったらスキップ成立
		skipCompleted_ = t >= 1.0f;

		// skip.png本体は色もサイズも変えない
		sprite_->SetSize({
			baseSize_.x * normalScale_,
			baseSize_.y * normalScale_
			});


		sprite_->SetPosition(basePos_);
		sprite_->Update();

		// ゲージの左端位置を計算する
		if (gaugeSprite_) {
			const Vector2 gaugePos = {
				basePos_.x + gaugeOffset_.x,
				basePos_.y + gaugeOffset_.y
			};

			// ゲージを左から切り取りながら表示する
			const float gaugeWidth = gaugeMaxSize_.x * t;
			const float gaugeTextureWidth = gaugeTexSize_.x * t;

			gaugeSprite_->SetPosition(gaugePos);

			// 画像の使用範囲も左から伸ばす
			gaugeSprite_->SetTextureLeftTop({ 0.0f, 0.0f });
			gaugeSprite_->SetTextureSize({
				gaugeTextureWidth,
				gaugeTexSize_.y
				});

			// 表示サイズは横だけ伸ばして、縦は固定
			gaugeSprite_->SetSize({
				gaugeWidth,
				gaugeMaxSize_.y
				});

			gaugeSprite_->SetColor(gaugeColor_);
			gaugeSprite_->Update();
		}
	}

	void SkipGuideUI::Draw(float alpha) {
		// スプライトが未生成なら描画しない
		if (!sprite_) {
			return;
		}

		// ゲージを先に描画する
		if (gaugeSprite_) {
			Vector4 gaugeCol = gaugeSprite_->GetColor();
			const float originalGaugeAlpha = gaugeCol.w;

			gaugeCol.w = originalGaugeAlpha * alpha;
			gaugeSprite_->SetColor(gaugeCol);
			gaugeSprite_->Draw();

			gaugeCol.w = originalGaugeAlpha;
			gaugeSprite_->SetColor(gaugeCol);
		}

		// skip.png本体を前面に描画する
		Vector4 col = sprite_->GetColor();
		const float originalAlpha = col.w;

		col.w = originalAlpha * alpha;
		sprite_->SetColor(col);
		sprite_->Draw();

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

		// 現在のホールド進行状況を確認する
		ImGui::Text("HoldTimer: %.2f / %.2f", holdTimer_, kHoldTime_);

		ImGui::TreePop();

		ImGui::DragFloat2("ゲージ左端オフセット", &gaugeOffset_.x, 1.0f, -500.0f, 500.0f);
		ImGui::DragFloat2("ゲージ最大サイズ", &gaugeMaxSize_.x, 1.0f, 0.0f, 500.0f);
		ImGui::ColorEdit4("ゲージ色", &gaugeColor_.x);
		ImGui::Text("SkipCompleted: %s", skipCompleted_ ? "true" : "false");
#endif
	}

} // namespace TKM