#include "UIController.h"
#include "TextureManager.h"
#include <algorithm>
#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	static float Clamp01_(float a) {
		if (a < 0.0f) return 0.0f;
		if (a > 1.0f) return 1.0f;
		return a;
	}

	void UIController::SetHudAlpha(float a) {
		hudAlpha_ = Clamp01_(a);
	}

	void UIController::SetRightUiScale(float s) {
		if (s < 0.001f) s = 0.001f;
		rightUiScale_ = s;
		ApplyRightUiSizes_();
		ApplyRightUiPositions_();
	}

	void UIController::SetRightUiMargin(float px) {
		rightUiMargin_ = px;
		ApplyRightUiPositions_();
	}

	void UIController::SetRightUiSpacing(float px) {
		rightUiSpacing_ = px;
		ApplyRightUiPositions_();
	}

	std::unique_ptr<Sprite> UIController::CreateSprite_(const std::string& texPath, const Vector2& anchor, Vector2* outTexSize) {
		auto sp = std::make_unique<Sprite>();
		sp->Initialize(spriteCommon_, dxCommon_, texPath);

		// サイズはこっちで数値管理したいので、自動調整はOFF
		sp->SetAutoAdjustTextureSize(false);

		// テクスチャ切り出しは「画像そのまま」
		const auto& m = TextureManager::GetInstance()->GetMetadata(texPath);
		const Vector2 texSize{ (float)m.width, (float)m.height };
		sp->SetTextureLeftTop({ 0.0f, 0.0f });
		sp->SetTextureSize(texSize);

		sp->SetAnchorPoint(anchor);

		if (outTexSize) {
			*outTexSize = texSize;
		}
		return sp;
	}

	void UIController::ApplyRightUiSizes_() {
		lbDrawSize_ = { lbTexSize_.x * lbScale_, lbTexSize_.y * lbScale_ };
		rbDrawSize_ = { rbTexSize_.x * rbScale_, rbTexSize_.y * rbScale_ };
		xDrawSize_ = { xTexSize_.x * xScale_, xTexSize_.y * xScale_ };
		lsDrawSize_ = { lsTexSize_.x * lsScale_, lsTexSize_.y * lsScale_ };

		if (uiLB_) uiLB_->SetSize(lbDrawSize_);
		if (uiRB_) uiRB_->SetSize(rbDrawSize_);
		if (uiX_) uiX_->SetSize(xDrawSize_);
		if (uiLS_) uiLS_->SetSize(lsDrawSize_);
	}

	void UIController::ApplyRightUiPositions_() {
		const float baseX = screenW_ - rightUiMargin_;
		const float baseY = screenH_ - rightUiMargin_;

		// RB
		Vector2 rbPos{ baseX, baseY };
		rbPos.x += rbOffset_.x;
		rbPos.y += rbOffset_.y;

		// LB（RBの上に積む）
		Vector2 lbPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) };
		lbPos.x += lbOffset_.x;
		lbPos.y += lbOffset_.y;

		// X（LBの上に積む）
		Vector2 xPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) };
		xPos.x += xOffset_.x;
		xPos.y += xOffset_.y;

		// LS（Xの上に積む）
		Vector2 lsPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) - (xDrawSize_.y + rightUiSpacing_) };
		lsPos.x += lsOffset_.x;
		lsPos.y += lsOffset_.y;

		// 基準座標を保存（ここがないとシェイク戻し先が分からない）
		basePosRB_ = rbPos;
		basePosLB_ = lbPos;
		basePosX_ = xPos;
		basePosLS_ = lsPos;

		// ひとまず基準位置で配置
		if (uiRB_) uiRB_->SetPosition(basePosRB_);
		if (uiLB_) uiLB_->SetPosition(basePosLB_);
		if (uiX_)  uiX_->SetPosition(basePosX_);
		if (uiLS_) uiLS_->SetPosition(basePosLS_);
	}

	void UIController::ApplyShake_(Sprite* sp, const Vector2& basePos, bool down, float& t) {
		if (!sp) { return; }

		if (!down) {
			t = 0.0f;
			sp->SetPosition(basePos);
			return;
		}

		// 持続時間0.5秒のシンプルな揺れアニメーション
		float r1 = MyMath::Rand01() * 2.0f - 1.0f; // -1..1
		float r2 = MyMath::Rand01() * 2.0f - 1.0f;

		// 揺れ幅をほんの少し変動させて“震え感”を強める
		float amp = shakeAmpPx_;
		float sx = r1 * amp;
		float sy = r2 * amp;

		sp->SetPosition({ basePos.x + sx, basePos.y + sy });
	}

	void UIController::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;

		screenW_ = screenW;
		screenH_ = screenH;

		hudAlpha_ = 1.0f;

		idleCol_ = { 1,1,1,1.0f }; // 押されてないときは通常の色
		onCol_ = { 1,0.25f,0.25f,1.0f }; // 押されたときは赤みが強くなるように

		colLB_ = { 1,1,1,1 };
		colRB_ = { 1,1,1,1 };
		colX_ = { 1,1,1,1 };
		colLS_ = { 1,1,1,1 };

		// 右側UI（差し替えたい画像パスはここだけ）
		lbTex_ = "./resources/LB_ui.png";
		rbTex_ = "./resources/RB_ui.png";
		xTex_ = "./resources/X_ui.png";
		lsTex_ = "./resources/LS_ui.png";

		uiLB_ = CreateSprite_(lbTex_, { 1.0f, 1.0f }, &lbTexSize_);
		uiRB_ = CreateSprite_(rbTex_, { 1.0f, 1.0f }, &rbTexSize_);
		uiX_ = CreateSprite_(xTex_, { 1.0f, 1.0f }, &xTexSize_);
		uiLS_ = CreateSprite_(lsTex_, { 1.0f, 1.0f }, &lsTexSize_);

		ApplyRightUiSizes_();
		ApplyRightUiPositions_();

		// 弾UI
		rbGaugeUI_ = std::make_unique<TKM::RBGaugeUI>();
		TKM::RBGaugeUI::Desc d{};
		rbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, d);

		// HPバー
		hpFrame_ = std::make_unique<Sprite>();
		hpFill_ = std::make_unique<Sprite>();

		const std::string hpFrameTex = "./resources/uvChecker.png";
		const std::string hpFillTex = "./resources/circle.png";

		hpFrame_->Initialize(spriteCommon_, dxCommon_, hpFrameTex);
		hpFill_->Initialize(spriteCommon_, dxCommon_, hpFillTex);

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

		hpFrame_->SetAnchorPoint({ 0.5f, 0.5f });
		hpFill_->SetAnchorPoint({ 0.5f, 0.5f });

		hpFrame_->SetSize(hpSize_);
		hpFill_->SetSize(hpSize_);

		colHPFrame_ = { 1.0f, 1.0f, 1.0f, 0.90f };
		colHPFill_ = { 0.25f, 1.0f, 0.35f, 0.90f };

		UpdateLayout(screenW, screenH);
	}

	void UIController::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;

		ApplyRightUiPositions_();

		if (rbGaugeUI_) {
			auto desc = rbGaugeUI_->GetDesc();
			desc.center_ = { screenW_ * 0.5f, screenH_ - 60.0f - ammoUiRaiseY_ };
			rbGaugeUI_->SetDesc(desc);
		}

		hpCenter_ = { screenW_ * 0.5f, screenH_ - 60.0f };
		if (hpFrame_) hpFrame_->SetPosition(hpCenter_);
		if (hpFill_)  hpFill_->SetPosition(hpCenter_);
	}

	void UIController::Update(float dt, Player* player) {
		if (rbGaugeUI_ && player) {
			rbGaugeUI_->Update(dt, player->GetRbAmmo(), player->GetRbAmmoMax(), player->IsRbRefilling());
		}

		Input* in = Input::GetInstance();
		const bool rbDown = in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER);
		const bool lbDown = in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER);
		const bool xDown = in->PushButton(XINPUT_GAMEPAD_X);
		const bool lsDown = in->PushButton(XINPUT_GAMEPAD_LEFT_THUMB);

		colRB_ = rbDown ? onCol_ : idleCol_;
		colLB_ = lbDown ? onCol_ : idleCol_;
		colX_ = xDown ? onCol_ : idleCol_;
		colLS_ = lsDown ? onCol_ : idleCol_;

		// UIの更新はここで行う。シェイクもここで行う（押されてるフレームだけシェイクさせるため）。
		if (uiLB_) uiLB_->Update();
		if (uiRB_) uiRB_->Update();
		if (uiX_) uiX_->Update();
		if (uiLS_) uiLS_->Update();
		// シェイクは押されてるフレームだけ適用して、離されたら即座に元の位置に戻す
		if (uiRB_) ApplyShake_(uiRB_.get(), basePosRB_, rbDown, shakeT_RB_); // 押されたフレームだけシェイクさせる
		if (uiLB_) ApplyShake_(uiLB_.get(), basePosLB_, lbDown, shakeT_LB_); // 押されたフレームだけシェイクさせる
		if (uiX_)  ApplyShake_(uiX_.get(), basePosX_, xDown, shakeT_X_); // 押されたフレームだけシェイクさせる

		if (player && hpFill_) {
			float rate = player->GetHPRate();
			if (rate < 0.0f) rate = 0.0f;
			if (rate > 1.0f) rate = 1.0f;

			Vector2 s = hpSize_;
			s.x *= rate;
			hpFill_->SetSize(s);
		}
		if (hpFrame_) hpFrame_->Update();
		if (hpFill_)  hpFill_->Update();

		DrawImGui(); // デバッグ用のImGui表示
	}

	void UIController::Draw() {
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 o = c;
			o.w *= hudAlpha_;
			return o;
			};

		if (hpFrame_) { hpFrame_->SetColor(mulAlpha(colHPFrame_)); hpFrame_->Draw(); }
		if (hpFill_) { hpFill_->SetColor(mulAlpha(colHPFill_));  hpFill_->Draw(); }

		if (uiLB_) { uiLB_->SetColor(mulAlpha(colLB_)); uiLB_->Draw(); }
		if (uiRB_) { uiRB_->SetColor(mulAlpha(colRB_)); uiRB_->Draw(); }
		if (uiX_) { uiX_->SetColor(mulAlpha(colX_)); uiX_->Draw(); }
		if (uiLS_) { uiLS_->SetColor(mulAlpha(colLS_)); uiLS_->Draw(); }

		if (rbGaugeUI_) rbGaugeUI_->Draw();
	}

#ifdef USE_IMGUI
	void TKM::UIController::DrawImGui() {
		if (!ImGui::Begin("UIController")) {
			ImGui::End();
			return;
		}

		bool changed = false;

		ImGui::Text("右側UI：個別調整");

		changed |= ImGui::DragFloat("Margin(px)", &rightUiMargin_, 0.5f, 0.0f, 300.0f);
		changed |= ImGui::DragFloat("Spacing(px)", &rightUiSpacing_, 0.5f, 0.0f, 200.0f);

		ImGui::Separator();

		// RB
		if (ImGui::TreeNode("RB")) {
			changed |= ImGui::DragFloat("Scale##rb", &rbScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("Offset##rb", &rbOffset_.x, 0.5f, -500.0f, 500.0f);
			ImGui::TreePop();
		}

		// LB
		if (ImGui::TreeNode("LB")) {
			changed |= ImGui::DragFloat("Scale##lb", &lbScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("Offset##lb", &lbOffset_.x, 0.5f, -500.0f, 500.0f);
			ImGui::TreePop();
		}

		// X
		if (ImGui::TreeNode("X")) {
			changed |= ImGui::DragFloat("Scale##x", &xScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("Offset##x", &xOffset_.x, 0.5f, -500.0f, 500.0f);
			ImGui::TreePop();
		}

		// LS
		if (ImGui::TreeNode("LS")) {
			changed |= ImGui::DragFloat("Scale##ls", &lsScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("Offset##ls", &lsOffset_.x, 0.5f, -500.0f, 500.0f);
			ImGui::TreePop();
		}

		ImGui::Separator();
		if (ImGui::TreeNode("Colors")) {
			changed |= ImGui::ColorEdit4("Idle", &idleCol_.x);
			changed |= ImGui::ColorEdit4("On", &onCol_.x);
			ImGui::TreePop();
		}

		if (ImGui::Button("Reset")) {
			rightUiMargin_ = 20.0f;
			rightUiSpacing_ = 10.0f;

			idleCol_ = { 1,1,1,0.75f };
			onCol_ = { 1,0.25f,0.25f,1.0f };

			changed = true;
		}

		if (changed) {
			ApplyRightUiSizes_();
			ApplyRightUiPositions_();
		}

		ImGui::End();
	}
#endif
} // namespace TKM