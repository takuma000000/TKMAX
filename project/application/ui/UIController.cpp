#include "UIController.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>
#include "Easing.h"

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
		rbGaugeIconDrawSize_ = { rbGaugeIconTexSize_.x * rbGaugeIconScale_, rbGaugeIconTexSize_.y * rbGaugeIconScale_ };

		if (uiLB_) uiLB_->SetSize(lbDrawSize_);
		if (uiRB_) uiRB_->SetSize(rbDrawSize_);
		if (uiX_) uiX_->SetSize(xDrawSize_);
		if (uiLS_) uiLS_->SetSize(lsDrawSize_);
		if (uiRBGaugeIcon_) uiRBGaugeIcon_->SetSize(rbGaugeIconDrawSize_);
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

	void UIController::ApplyHudPositions_() {
		// 左下HUDは「左側に確保する縦ゲージ幅」を避けた位置を基準にする
		float leftEdgeX = hudLeftMargin_ + hudReserveLeftW_ + hudReserveGap_;

		// RB残弾ゲージ（中心座標）
		if (rbGaugeUI_) {
			auto d = rbGaugeUI_->GetDesc();

			Vector2 rbCenter{
				leftEdgeX + d.size_.x * 0.5f,
				screenH_ - hudBottomMargin_ - ammoUiRaiseY_
			};

			d.center_ = rbCenter;
			rbGaugeUI_->SetDesc(d);

			// RBゲージアイコン（RBゲージの右に置く）※anchor={1,1}なので右下基準
			if (uiRBGaugeIcon_) {
				const float iconW = rbGaugeIconDrawSize_.x;
				const float iconH = rbGaugeIconDrawSize_.y;

				const float gaugeRightX = rbCenter.x + d.size_.x * 0.5f;

				Vector2 iconPos{
					gaugeRightX + rbGaugeIconPadX_ + iconW,
					rbCenter.y + iconH * 0.5f
				};

				// 微調整
				iconPos.x += rbGaugeIconOffset_.x;
				iconPos.y += rbGaugeIconOffset_.y;

				basePosRBGaugeIcon_ = iconPos;
				uiRBGaugeIcon_->SetPosition(iconPos);
			}
		}

		// 縦HPゲージ：左の確保スペース(hudReserveLeftW_)の中央に置く（下基準）
		{
			const float hpX = hudLeftMargin_ + (hudReserveLeftW_ * 0.5f);
			const float hpY = screenH_ - hudBottomMargin_; // 「下からの余白」をそのまま使う

			Vector2 hpPos{ hpX + hpVertOffset_.x, hpY + hpVertOffset_.y }; // 微調整

			// HPフレームはそのまま（中心基準）
			basePosHPFrame_ = hpPos;
			if (hpFrame_) {
				hpFrame_->SetPosition(basePosHPFrame_);
			}

			// HPフィルは「下基準」なので中心→下端座標に変換して保存
			basePosHPFill_ = {
				hpPos.x,
				hpPos.y + hpVertSize_.y * 0.5f
			};

			if (hpFill_) {
				hpFill_->SetPosition(basePosHPFill_);
			}

			// HPアイコン：HPゲージの下に置く（basePosHPFill_ は下端座標）
			if (hpIcon_) {
				// アイコンのサイズを考慮して、HPゲージの下に配置するための座標を計算
				const float iconW = hpIconDrawSize_.x;
				const float iconH = hpIconDrawSize_.y;
				// アイコンの上端がHPゲージの下端にくるように配置
				Vector2 iconPos{
					basePosHPFill_.x,
					basePosHPFill_.y + (iconH * 0.5f) + hpIconOffset_.y
				};
				iconPos.x += hpIconOffset_.x; // 微調整
				// アイコンの位置を保存
				hpIcon_->SetPosition(iconPos);
			}
		}
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
		colRBGaugeIcon_ = { 1,1,1,1 };

		// 右側UI（差し替えたい画像パスはここだけ）
		lbTex_ = "./resources/texture/LB_ui.png";
		rbTex_ = "./resources/texture/RB_ui.png";
		xTex_ = "./resources/texture/X_ui.png";
		lsTex_ = "./resources/texture/LS_ui.png";
		rbGaugeIconTex_ = "./resources/texture/RB_gauge_ui.png";

		uiLB_ = CreateSprite_(lbTex_, { 1.0f, 1.0f }, &lbTexSize_);
		uiRB_ = CreateSprite_(rbTex_, { 1.0f, 1.0f }, &rbTexSize_);
		uiX_ = CreateSprite_(xTex_, { 1.0f, 1.0f }, &xTexSize_);
		uiLS_ = CreateSprite_(lsTex_, { 1.0f, 1.0f }, &lsTexSize_);
		uiRBGaugeIcon_ = CreateSprite_(rbGaugeIconTex_, { 1.0f, 1.0f }, &rbGaugeIconTexSize_);

		ApplyRightUiSizes_();
		ApplyRightUiPositions_();

		// 弾UI
		rbGaugeUI_ = std::make_unique<TKM::RBGaugeUI>();
		TKM::RBGaugeUI::Desc d{};
		rbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, d);

		// HPバー
		hpFrame_ = std::make_unique<Sprite>();
		hpFill_ = std::make_unique<Sprite>();
		// HPアイコン（player_hp.png）
		hpIcon_ = std::make_unique<Sprite>();
		// 画像を指定
		const std::string hpFrameTex = "./resources/texture/player_hp_frame.jpg";
		const std::string hpFillTex = "./resources/texture/player_hp.jpg";
		const std::string hpIconTex = "./resources/texture/player_hp.png";
		// スプライトを初期化
		hpFrame_->Initialize(spriteCommon_, dxCommon_, hpFrameTex);
		hpFill_->Initialize(spriteCommon_, dxCommon_, hpFillTex);
		hpIcon_->Initialize(spriteCommon_, dxCommon_, hpIconTex);
		// サイズはこっちで数値管理したいので、自動調整はOFF
		hpFrame_->SetAutoAdjustTextureSize(false);
		hpFill_->SetAutoAdjustTextureSize(false);
		hpIcon_->SetAutoAdjustTextureSize(false);
		// テクスチャ切り出しは「画像そのまま」
		{
			const auto& metaF = TextureManager::GetInstance()->GetMetadata(hpFrameTex);
			hpFrame_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpFrame_->SetTextureSize({ (float)metaF.width, (float)metaF.height });

			const auto& metaFi = TextureManager::GetInstance()->GetMetadata(hpFillTex);
			hpFill_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpFill_->SetTextureSize({ (float)metaFi.width, (float)metaFi.height });

			const auto& metaI = TextureManager::GetInstance()->GetMetadata(hpIconTex);
			hpIcon_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpIcon_->SetTextureSize({ (float)metaI.width, (float)metaI.height });
			hpIconTexSize_ = { (float)metaI.width, (float)metaI.height };
		}

		// 縦ゲージなので「下基準」にする（高さを縮めても下に張り付く）
		hpFrame_->SetAnchorPoint({ 0.5f, 0.5f }); // 中心
		hpFill_->SetAnchorPoint({ 0.5f, 1.0f }); // 下基準
		// アイコンは中心基準
		if (hpIcon_) {
			hpIcon_->SetAnchorPoint({ 0.5f, 0.5f }); // アイコンは中心基準
			hpIconDrawSize_ = { hpIconTexSize_.x * hpIconScale_, hpIconTexSize_.y * hpIconScale_ }; // アイコンは縮小して表示する前提なので、テクスチャサイズとスケールから描画サイズを計算して保存
			hpIcon_->SetSize(hpIconDrawSize_); // アイコンは縮小して表示する前提なので、テクスチャサイズとスケールから描画サイズを計算して適用
		}

		// 初期サイズ（縦ゲージ）
		hpFrame_->SetSize({ hpVertSize_.x + hpFramePad_, hpVertSize_.y + hpFramePad_ });
		hpFill_->SetSize(hpVertSize_);

		colHPFrame_ = { 1.0f, 1.0f, 1.0f, 0.90f };
		colHPFill_ = { 0.25f, 1.0f, 0.35f, 0.90f };

		UpdateLayout(screenW, screenH); // 画面サイズを元に初期レイアウトを適用
	}

	void UIController::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;

		// 右側UI（LB/RB/X/LS）は今まで通り
		ApplyRightUiPositions_();

		// 縦HPゲージのサイズを適用
		if (hpFill_)  hpFill_->SetSize(hpVertSize_); // HPは減るのでサイズ変更の必要があるのはhpFill_の方だけ
		if (hpFrame_) hpFrame_->SetSize({ hpVertSize_.x + hpFramePad_, hpVertSize_.y + hpFramePad_ }); // フレームはHPより少し大きくしてる前提なので、hpVertSize_を元にサイズを計算して適用

		// 左下HUD（HP/RB/アイコン）をまとめて配置
		ApplyHudPositions_();
	}

	void UIController::Update(float dt, Player* player) {
		Input* in = Input::GetInstance();
		const bool rbDown = in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER);
		const bool lbDown = in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER);
		const bool xDown = in->PushButton(XINPUT_GAMEPAD_X);

		if (rbGaugeUI_ && player) {
			rbGaugeUI_->Update(dt, player->GetRbAmmo(), player->GetRbAmmoMax(), player->IsRbRefilling(), rbDown); // RB残弾UIはRBの状態とプレイヤーの弾情報を渡して更新
		}

		// ---- 左スティック入力（倒し量で判定）----
		const SHORT rawX = in->GetLeftStickX();
		const SHORT rawY = in->GetLeftStickY();

		auto normAxis = [&](SHORT v)->float {
			// -32768 対策（負側だけ分母が違う）
			float f = (v >= 0) ? (float)v / 32767.0f : (float)v / 32768.0f;
			if (f < -1.0f) f = -1.0f;
			if (f > 1.0f) f = 1.0f;
			return f;
			};

		auto applyDeadzone = [&](float a)->float {
			float absA = (a < 0.0f) ? -a : a;
			if (absA <= lsDeadzone_) {
				return 0.0f;
			}
			// 0..1 に再マッピング
			float t = (absA - lsDeadzone_) / (1.0f - lsDeadzone_);
			return (a < 0.0f) ? -t : t;
			};

		float lsX = applyDeadzone(normAxis(rawX));
		float lsY = applyDeadzone(normAxis(rawY));

		// 「動かしてる間ずっと赤」判定
		const bool lsMoving = (lsX != 0.0f) || (lsY != 0.0f);

		// ---- 色 ----
		colRB_ = rbDown ? onCol_ : idleCol_;
		colLB_ = lbDown ? onCol_ : idleCol_;
		colX_ = xDown ? onCol_ : idleCol_;
		colLS_ = lsMoving ? onCol_ : idleCol_;

		// ---- UI Update ----
		if (uiLB_) uiLB_->Update();
		if (uiRB_) uiRB_->Update();
		if (uiX_)  uiX_->Update();
		if (uiLS_) uiLS_->Update();
		if (uiRBGaugeIcon_) uiRBGaugeIcon_->Update();
		if (hpIcon_)  hpIcon_->Update();

		// ---- 押下中シェイク（必要な分だけ）----
		if (uiRB_) ApplyShake_(uiRB_.get(), basePosRB_, rbDown, shakeT_RB_);
		if (uiLB_) ApplyShake_(uiLB_.get(), basePosLB_, lbDown, shakeT_LB_);
		if (uiX_)  ApplyShake_(uiX_.get(), basePosX_, xDown, shakeT_X_);
		if (uiRBGaugeIcon_) ApplyShake_(uiRBGaugeIcon_.get(), basePosRBGaugeIcon_, rbDown, shakeT_RBGaugeIcon_);

		// ---- LS：倒し方向に同期して動かす ----
		if (uiLS_) {
			// 画面Yは下が+なので、スティック上方向(+)はYをマイナスへ
			Vector2 ofs{
				lsX * lsMoveRangePx_,
				-lsY * lsMoveRangePx_
			};
			uiLS_->SetPosition({ basePosLS_.x + ofs.x, basePosLS_.y + ofs.y });
		}

		// ---- HP（減るときアニメ＆被弾感） ----
		if (player && hpFill_) {
			// ターゲット（本当のHP）
			hpTargetRate_ = player->GetHPRate();
			if (hpTargetRate_ < 0.0f) hpTargetRate_ = 0.0f;
			if (hpTargetRate_ > 1.0f) hpTargetRate_ = 1.0f;

			// 被弾検出（整数HPで見る）
			const int curHp = player->GetHP();
			if (prevHp_ < 0) {
				prevHp_ = curHp;
				hpAnimRate_ = hpTargetRate_;
				hpTweenActive_ = false;
			}

			bool damaged = (curHp < prevHp_);
			bool healed = (curHp > prevHp_);

			if (damaged) {
				hpHitFlashT_ = hpHitFlashSec_;
				hpShakeT_ = hpShakeSec_;

				// 減少はイージングで“演出”
				hpTween_.Reset(hpAnimRate_, hpTargetRate_, hpDrainEaseSec_, hpDrainEaseType_);
				hpTweenActive_ = true;
			}

			prevHp_ = curHp;

			// 追従：Tweenが動いてる間はTween、そうでなければ即 or 軽い追従
			if (hpTweenActive_) {
				hpAnimRate_ = hpTween_.Update(dt);
				if (hpTween_.Finished()) {
					hpAnimRate_ = hpTargetRate_;
					hpTweenActive_ = false;
				}
			} else {
				// ここは好み：常にイージングにしたいなら削ってOK
				hpAnimRate_ = hpTargetRate_;
			}

			// サイズ反映（下基準）
			Vector2 s = hpVertSize_;
			s.y *= hpAnimRate_;
			hpFill_->SetSize(s);

			// シェイク（HPフレーム/フィル両方）
			if (hpShakeT_ > 0.0f) {
				hpShakeT_ -= dt;
				if (hpShakeT_ < 0.0f) hpShakeT_ = 0.0f;

				float r1 = MyMath::Rand01() * 2.0f - 1.0f;
				float r2 = MyMath::Rand01() * 2.0f - 1.0f;
				Vector2 ofs{ r1 * hpShakeAmpPx_, r2 * hpShakeAmpPx_ };
				if (hpFrame_) hpFrame_->SetPosition({ basePosHPFrame_.x + ofs.x, basePosHPFrame_.y + ofs.y });
				hpFill_->SetPosition({ basePosHPFill_.x + ofs.x, basePosHPFill_.y + ofs.y });
			} else {
				if (hpFrame_) hpFrame_->SetPosition(basePosHPFrame_);
				hpFill_->SetPosition(basePosHPFill_);
			}

			// フラッシュタイマー
			if (hpHitFlashT_ > 0.0f) {
				hpHitFlashT_ -= dt;
				if (hpHitFlashT_ < 0.0f) hpHitFlashT_ = 0.0f;
			}
		}

		if (hpFrame_) hpFrame_->Update();
		if (hpFill_)  hpFill_->Update();

		DrawImGui();
	}

	void UIController::Draw() {
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 o = c;
			o.w *= hudAlpha_;
			return o;
			};

		// ---- HP ----
		if (hpFrame_) {
			hpFrame_->SetColor(mulAlpha(colHPFrame_));
			hpFrame_->Draw();
		}
		if (hpIcon_) {
			hpIcon_->SetColor(mulAlpha(colHPIcon_));
			hpIcon_->Draw();
		}
		if (hpFill_) {
			// フラッシュ割合（0..1）
			float t = 0.0f;
			if (hpHitFlashSec_ > 0.0f) {
				t = hpHitFlashT_ / hpHitFlashSec_;
				t = std::clamp(t, 0.0f, 1.0f);
			}

			// 被弾時のフラッシュ色（赤寄り）
			Vector4 flashCol{
				1.0f,   // R
				0.0f,  // G
				0.0f,  // B
				colHPFill_.w // Alphaは元のまま
			};

			// MyMathのVector4Lerpを使用
			Vector4 c = MyMath::Vector4Lerp(colHPFill_, flashCol, t);

			// HUD全体アルファを適用
			hpFill_->SetColor(mulAlpha(c));
			hpFill_->Draw();
		}

		if (uiLB_) { uiLB_->SetColor(mulAlpha(colLB_)); uiLB_->Draw(); }
		if (uiRB_) { uiRB_->SetColor(mulAlpha(colRB_)); uiRB_->Draw(); }
		if (uiX_) { uiX_->SetColor(mulAlpha(colX_)); uiX_->Draw(); }
		if (uiLS_) { uiLS_->SetColor(mulAlpha(colLS_)); uiLS_->Draw(); }
		if (uiRBGaugeIcon_) { uiRBGaugeIcon_->SetColor(mulAlpha(colRBGaugeIcon_)); uiRBGaugeIcon_->Draw(); }

		if (rbGaugeUI_) rbGaugeUI_->Draw();
	}

	void TKM::UIController::DrawImGui() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("UIController")) {
			ImGui::End();
			return;
		}

		bool changed = false;

		ImGui::Text("右側UI：個別調整");

		changed |= ImGui::DragFloat("右下余白(px)", &rightUiMargin_, 0.5f, 0.0f, 300.0f);
		changed |= ImGui::DragFloat("縦間隔(px)", &rightUiSpacing_, 0.5f, 0.0f, 200.0f);

		ImGui::Separator();

		// RB
		if (ImGui::TreeNode("RB（右バンパー）")) {
			changed |= ImGui::DragFloat("サイズ##rb", &rbScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("位置オフセット##rb", &rbOffset_.x, 0.5f, -500.0f, 500.0f);
			ImGui::TreePop();
		}

		// LB
		if (ImGui::TreeNode("LB（左バンパー）")) {
			changed |= ImGui::DragFloat("サイズ##lb", &lbScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("位置オフセット##lb", &lbOffset_.x, 0.5f, -500.0f, 500.0f);
			ImGui::TreePop();
		}

		// X
		if (ImGui::TreeNode("Xボタン")) {
			changed |= ImGui::DragFloat("サイズ##x", &xScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("位置オフセット##x", &xOffset_.x, 0.5f, -500.0f, 500.0f);
			ImGui::TreePop();
		}

		// LS
		if (ImGui::TreeNode("左スティック（LS）")) {
			changed |= ImGui::DragFloat("サイズ##ls", &lsScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("位置オフセット##ls", &lsOffset_.x, 0.5f, -500.0f, 500.0f);
			changed |= ImGui::DragFloat("移動量(px)##ls", &lsMoveRangePx_, 0.1f, 0.0f, 50.0f);
			changed |= ImGui::DragFloat("デッドゾーン##ls", &lsDeadzone_, 0.01f, 0.0f, 0.95f);
			ImGui::TreePop();
		}

		// RBゲージアイコン
		if (ImGui::TreeNode("RBゲージアイコン")) {
			changed |= ImGui::DragFloat("サイズ##rbGaugeIcon", &rbGaugeIconScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat("右端余白(px)##rbGaugeIconPad", &rbGaugeIconPadX_, 0.5f, 0.0f, 200.0f);
			changed |= ImGui::DragFloat2("微調整オフセット##rbGaugeIcon", &rbGaugeIconOffset_.x, 0.5f, -1500.0f, 300.0f);
			ImGui::TreePop();
		}

		ImGui::Separator();

		ImGui::Text("HPアイコン（player_hp.png）");
		changed |= ImGui::DragFloat("HPアイコン scale", &hpIconScale_, 0.001f, 0.01f, 2.0f);
		changed |= ImGui::DragFloat2("HPアイコン offset(x,y)", &hpIconOffset_.x, 0.5f, -300.0f, 300.0f);

		ImGui::Separator();

		ImGui::Text("左下HUD：配置（縦ゲージの確保幅込み）");
		changed |= ImGui::DragFloat("左余白(px)", &hudLeftMargin_, 0.5f, 0.0f, 600.0f);
		changed |= ImGui::DragFloat("縦ゲージ確保幅(px)", &hudReserveLeftW_, 0.5f, 0.0f, 800.0f);
		changed |= ImGui::DragFloat("確保幅の右の間隔(px)", &hudReserveGap_, 0.5f, 0.0f, 300.0f);
		changed |= ImGui::DragFloat("下余白(px)", &hudBottomMargin_, 0.5f, 0.0f, 300.0f);
		changed |= ImGui::DragFloat("弾UI上げ量(px)", &ammoUiRaiseY_, 0.5f, 0.0f, 300.0f);

		ImGui::Separator();
		ImGui::Text("縦HPゲージ（左確保スペース内）");

		changed |= ImGui::DragFloat2("HP縦サイズ(w,h)", &hpVertSize_.x, 0.5f, 2.0f, 800.0f);
		changed |= ImGui::DragFloat2("HP縦オフセット(x,y)", &hpVertOffset_.x, 0.5f, -300.0f, 300.0f);
		changed |= ImGui::DragFloat("HPフレーム余白", &hpFramePad_, 0.5f, 0.0f, 80.0f);


		ImGui::Separator();
		if (ImGui::TreeNode("色設定")) {
			changed |= ImGui::ColorEdit4("通常色", &idleCol_.x);
			changed |= ImGui::ColorEdit4("押下色", &onCol_.x);
			ImGui::TreePop();
		}

		if (ImGui::Button("リセット")) {
			rightUiMargin_ = 20.0f;
			rightUiSpacing_ = 10.0f;

			idleCol_ = { 1,1,1,0.75f };
			onCol_ = { 1,0.25f,0.25f,1.0f };

			changed = true;
		}

		if (changed) {
			ApplyRightUiSizes_();
			ApplyRightUiPositions_();
			ApplyHudPositions_();
			if (hpFill_)  hpFill_->SetSize(hpVertSize_);
			if (hpFrame_) hpFrame_->SetSize({ hpVertSize_.x + hpFramePad_, hpVertSize_.y + hpFramePad_ });
			if (hpIcon_) {
				hpIconDrawSize_ = { hpIconTexSize_.x * hpIconScale_, hpIconTexSize_.y * hpIconScale_ };
				hpIcon_->SetSize(hpIconDrawSize_);
			}
		}

		ImGui::End();
#endif
	}
} // namespace TKM