#include "UIController.h"
#include <algorithm>

namespace TKM {

	void UIController::SetHudAlpha(float a) {
		hudAlpha_ = std::max(0.0f, std::min(a, 1.0f));
	}

	void UIController::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;

		hudAlpha_ = 1.0f;
		colLT_ = { 1.0f,1.0f,1.0f,1.0f };
		colLB_ = { 1.0f,1.0f,1.0f,1.0f };
		colRB_ = { 1.0f,1.0f,1.0f,1.0f };

		uiLT_ = std::make_unique<Sprite>();
		uiLB_ = std::make_unique<Sprite>();
		uiRB_ = std::make_unique<Sprite>();

		uiLT_->Initialize(spriteCommon_, dxCommon_, "./resources/LT.png");
		uiLT_->SetAutoAdjustTextureSize(false);
		uiLB_->Initialize(spriteCommon_, dxCommon_, "./resources/LB.png");
		uiLB_->SetAutoAdjustTextureSize(false);
		uiRB_->Initialize(spriteCommon_, dxCommon_, "./resources/RB.png");
		uiRB_->SetAutoAdjustTextureSize(false);

		uiLT_->SetAnchorPoint({ 1.0f, 1.0f });
		uiLB_->SetAnchorPoint({ 1.0f, 1.0f });
		uiRB_->SetAnchorPoint({ 1.0f, 1.0f });

		const Vector2 uiSize = { 100.0f, 100.0f };
		uiLT_->SetSize(uiSize);
		uiLB_->SetSize(uiSize);
		uiRB_->SetSize(uiSize);

		const Vector4 idle = { 1.0f, 1.0f, 1.0f, 0.85f };
		uiLT_->SetColor(idle);
		uiLB_->SetColor(idle);
		uiRB_->SetColor(idle);

		UpdateLayout(screenW, screenH);

		rbGaugeUI_ = std::make_unique<TKM::RBGaugeUI>();
		TKM::RBGaugeUI::Desc d{};
		rbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, d);
	}

	void UIController::UpdateLayout(float screenW, float screenH) {
		const float margin = 20.0f;
		const float spacing = 10.0f;
		const Vector2 uiSize = { 100.0f, 100.0f };

		if (uiRB_) uiRB_->SetPosition({ screenW - margin, screenH - margin });
		if (uiLB_) uiLB_->SetPosition({ screenW - margin, screenH - margin - (uiSize.y + spacing) * 1.0f });
		if (uiLT_) uiLT_->SetPosition({ screenW - margin, screenH - margin - (uiSize.y + spacing) * 2.0f });
	}

	void UIController::Update(float dt, Player* player) {
		if (rbGaugeUI_ && player) {
			rbGaugeUI_->Update(
				dt,
				player->GetRbAmmo(),
				player->GetRbAmmoMax(),
				player->IsRbRefilling()
			);
		}

		Input* in = Input::GetInstance();
		bool rbDown = in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER);
		bool lbDown = in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER);
		bool ltDown = (in->GetLeftTrigger() > 30);

		const Vector4 idle = { 1.0f, 1.0f, 1.0f, 0.75f };
		const Vector4 on = { 1.0f, 0.25f, 0.25f, 1.0f };

		// ★見た目色は保持しておく（Draw側で hudAlpha_ を掛ける）
		colRB_ = (rbDown ? on : idle);
		colLB_ = (lbDown ? on : idle);
		colLT_ = (ltDown ? on : idle);

		// ここではUpdateだけしておく（色はDrawで毎フレーム確定させる）
		if (uiLT_) uiLT_->Update();
		if (uiLB_) uiLB_->Update();
		if (uiRB_) uiRB_->Update();
	}

	void UIController::Draw() {
		// HUDのαを掛けた色を毎フレーム確定
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 o = c;
			o.w *= hudAlpha_;
			return o;
			};

		if (uiLT_) { uiLT_->SetColor(mulAlpha(colLT_)); uiLT_->Draw(); }
		if (uiLB_) { uiLB_->SetColor(mulAlpha(colLB_)); uiLB_->Draw(); }
		if (uiRB_) { uiRB_->SetColor(mulAlpha(colRB_)); uiRB_->Draw(); }

		// RBGaugeUIは外部からαを掛けるAPIが無い前提で、そのまま描画
		// （もしここも薄くしたいなら、RBGaugeUI側に SetGlobalAlpha を足すのが綺麗）
		if (rbGaugeUI_) rbGaugeUI_->Draw();
	}
}