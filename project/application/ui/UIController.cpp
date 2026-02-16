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
		// テクスチャパスを変数化
		const std::string ltTex = "./resources/LT.png";
		const std::string lbTex = "./resources/LB_ui.png";
		const std::string rbTex = "./resources/RB_ui.png";

		uiLT_->Initialize(spriteCommon_, dxCommon_, ltTex);
		uiLT_->SetAutoAdjustTextureSize(false);
		uiLB_->Initialize(spriteCommon_, dxCommon_, lbTex);
		uiLB_->SetAutoAdjustTextureSize(false);
		uiRB_->Initialize(spriteCommon_, dxCommon_, rbTex);
		uiRB_->SetAutoAdjustTextureSize(false);
		{
			const auto& m = TextureManager::GetInstance()->GetMetadata(ltTex);
			uiLT_->SetTextureLeftTop({ 0.0f, 0.0f });
			uiLT_->SetTextureSize({ (float)m.width, (float)m.height });
		}
		{
			const auto& m = TextureManager::GetInstance()->GetMetadata(lbTex);
			uiLB_->SetTextureLeftTop({ 0.0f, 0.0f });
			uiLB_->SetTextureSize({ (float)m.width, (float)m.height });
		}
		{
			const auto& m = TextureManager::GetInstance()->GetMetadata(rbTex);
			uiRB_->SetTextureLeftTop({ 0.0f, 0.0f });
			uiRB_->SetTextureSize({ (float)m.width, (float)m.height }); // ここはテクスチャサイズに合わせる
		}

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

		// 先に弾UIを作る（Layoutで位置をいじりたいので）
		rbGaugeUI_ = std::make_unique<TKM::RBGaugeUI>();
		TKM::RBGaugeUI::Desc d{};
		rbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, d);

		// --- HPバー ---
		hpFrame_ = std::make_unique<Sprite>();
		hpFill_ = std::make_unique<Sprite>();

		// ここは仮パス（後で好きな画像に差し替えOK）
		const std::string hpFrameTex = "./resources/uvChecker.png";
		const std::string hpFillTex = "./resources/circle.png";

		hpFrame_->Initialize(spriteCommon_, dxCommon_, hpFrameTex);
		hpFill_->Initialize(spriteCommon_, dxCommon_, hpFillTex);

		// サイズは自前で指定するので autoAdjust は切る。
		// ただし textureSize_ がデフォルト64のままだとUVがバグるので、metadataから正しい切り出しサイズを入れる。
		hpFrame_->SetAutoAdjustTextureSize(false);
		hpFill_->SetAutoAdjustTextureSize(false);

		{
			const auto& metaF = TextureManager::GetInstance()->GetMetadata(hpFrameTex);
			hpFrame_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpFrame_->SetTextureSize({ (float)metaF.width, (float)metaF.height });

			const auto& metaFi = TextureManager::GetInstance()->GetMetadata(hpFillTex);
			hpFill_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpFill_->SetTextureSize({ (float)metaFi.width, (float)metaFi.height });
		}

		// 中央基準で置く（ゲージ枠に入れやすい）
		hpFrame_->SetAnchorPoint({ 0.5f, 0.5f });
		hpFill_->SetAnchorPoint({ 0.5f, 0.5f });

		hpFrame_->SetSize(hpSize_);
		hpFill_->SetSize(hpSize_);

		colHPFrame_ = { 1.0f, 1.0f, 1.0f, 0.90f };
		colHPFill_ = { 0.25f, 1.0f, 0.35f, 0.90f }; // とりあえず緑（嫌なら変えてOK）

		// 最後にレイアウト確定（ここで弾UIとHPの位置を決める）
		UpdateLayout(screenW, screenH);
	}

	void UIController::UpdateLayout(float screenW, float screenH) {
		const float margin = 20.0f;
		const float spacing = 10.0f;
		const Vector2 uiSize = { 100.0f, 100.0f };

		if (uiRB_) uiRB_->SetPosition({ screenW - margin, screenH - margin });
		if (uiLB_) uiLB_->SetPosition({ screenW - margin, screenH - margin - (uiSize.y + spacing) * 1.0f });
		if (uiLT_) uiLT_->SetPosition({ screenW - margin, screenH - margin - (uiSize.y + spacing) * 2.0f });

		// --- 弾UIを少し上に上げる ---
		if (rbGaugeUI_) {
			auto desc = rbGaugeUI_->GetDesc(); // コピーを取得
			desc.center_ = { screenW * 0.5f, screenH - 60.0f - ammoUiRaiseY_ };
			rbGaugeUI_->SetDesc(desc);         // まとめて反映
		}

		// --- HPバーは“元の弾UIの場所”に置く（その枠にHPを入れる） ---
		hpCenter_ = { screenW * 0.5f, screenH - 60.0f };

		if (hpFrame_) hpFrame_->SetPosition(hpCenter_);
		if (hpFill_)  hpFill_->SetPosition(hpCenter_);
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

		// 見た目色は保持しておく（Draw側で hudAlpha_ を掛ける）
		colRB_ = (rbDown ? on : idle);
		colLB_ = (lbDown ? on : idle);
		colLT_ = (ltDown ? on : idle);

		// ここではUpdateだけしておく（色はDrawで毎フレーム確定させる）
		if (uiLT_) uiLT_->Update();
		if (uiLB_) uiLB_->Update();
		if (uiRB_) uiRB_->Update();

		// --- HPバー更新 ---
		if (player && hpFill_) {
			const float rate = player->GetHPRate();
			Vector2 s = hpSize_;
			s.x *= rate;
			hpFill_->SetSize(s);
		}
		if (hpFrame_) hpFrame_->Update();
		if (hpFill_)  hpFill_->Update();
	}

	void UIController::Draw() {
		// HUDのαを掛けた色を毎フレーム確定
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 o = c;
			o.w *= hudAlpha_;
			return o;
			};

		if (hpFrame_) { hpFrame_->SetColor(mulAlpha(colHPFrame_)); hpFrame_->Draw(); }
		if (hpFill_) { hpFill_->SetColor(mulAlpha(colHPFill_));  hpFill_->Draw(); }
		if (uiLT_) { uiLT_->SetColor(mulAlpha(colLT_)); uiLT_->Draw(); }
		if (uiLB_) { uiLB_->SetColor(mulAlpha(colLB_)); uiLB_->Draw(); }
		if (uiRB_) { uiRB_->SetColor(mulAlpha(colRB_)); uiRB_->Draw(); }

		// RBGaugeUIは外部からαを掛けるAPIが無い前提で、そのまま描画
		// （もしここも薄くしたいなら、RBGaugeUI側に SetGlobalAlpha を足すのが綺麗）
		if (rbGaugeUI_) rbGaugeUI_->Draw();
	}
}