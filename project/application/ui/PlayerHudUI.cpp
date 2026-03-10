#include "PlayerHudUI.h"
#include "TextureManager.h"
#include "Input.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {
	void PlayerHudUI::ApplyHudPositions_() {
		// 左端のX座標 = 左マージン + 左予約領域幅 + 予約領域とゲージの間隔
		float leftEdgeX = hudLeftMargin_ + hudReserveLeftW_ + hudReserveGap_;

		// RBゲージとLBゲージの位置を計算して適用
		if (rbGaugeUI_) {
			auto rbDesc = rbGaugeUI_->GetDesc(); // サイズを取得

			// 中心座標を計算
			Vector2 rbCenter{
				leftEdgeX + rbDesc.size_.x * 0.5f, // X座標 = 左端 + ゲージ幅の半分
				screenH_ - hudBottomMargin_ - ammoUiRaiseY_ // Y座標 = 画面下端 - 下マージン - 上昇量
			};
			// オフセットを加算
			rbDesc.center_ = rbCenter;
			rbGaugeUI_->SetDesc(rbDesc);

			// LBゲージはRBゲージの下に配置するので、RBゲージの中心座標とサイズをもとに位置を計算
			if (lbGaugeUI_) {
				auto lbDesc = lbGaugeUI_->GetDesc();
				lbDesc.size_ = rbDesc.size_; // サイズはRBゲージと同じにする
				// LBゲージの中心座標 = RBゲージの中心座標 + (0, RBゲージの高さ/2 + LBゲージの高さ/2 + 間隔) + オフセット
				lbDesc.center_ = {
					rbCenter.x + lbGaugeOffset_.x,
					rbCenter.y + rbDesc.size_.y + lbGaugeSpacingY_ + lbGaugeOffset_.y
				};
				lbDesc.segments_ = 5; // LBゲージは5分割で表示する
				lbGaugeUI_->SetDesc(lbDesc); // 設定を適用して位置を更新
			}
			// RBゲージアイコンはRBゲージの右端に配置するので、RBゲージの中心座標とサイズをもとに位置を計算
			if (rbGaugeIcon_) {
				// アイコンの描画サイズ = テクスチャサイズ * スケール
				float iconW = rbGaugeIconDrawSize_.x;
				float iconH = rbGaugeIconDrawSize_.y;
				float gaugeRightX = rbCenter.x + rbDesc.size_.x * 0.5f;
				// アイコンの位置 = RBゲージの右端 + アイコンとゲージの間隔 + アイコンの幅（アイコンのアンカーが右端なので） , RBゲージの中心Y + アイコンの高さの半分（アイコンのアンカーが下端なので））
				Vector2 iconPos{
					gaugeRightX + rbGaugeIconPadX_ + iconW,
					rbCenter.y + iconH * 0.5f
				};
				// オフセットを加算
				iconPos.x += rbGaugeIconOffset_.x;
				iconPos.y += rbGaugeIconOffset_.y;
				// 位置を保存してスプライトに適用
				basePosRBGaugeIcon_ = iconPos;
				rbGaugeIcon_->SetPosition(basePosRBGaugeIcon_);
			}
		}

		// HPゲージは画面左下に配置する
		float hpX = hudLeftMargin_ + (hudReserveLeftW_ * 0.5f);
		float hpY = screenH_ - hudBottomMargin_;
		// HPゲージの位置 = 左端 + 予約領域幅の半分 , 画面下端 - 下マージン
		Vector2 hpPos{ hpX + hpVertOffset_.x, hpY + hpVertOffset_.y };
		// 位置を保存してスプライトに適用
		basePosHPFrame_ = hpPos;
		// HPフレームの位置 = HPゲージの位置
		hpFrame_->SetPosition(basePosHPFrame_);
		basePosHPFill_ = { // HPフレームの位置 + フレームのアンカーを考慮したオフセット
			hpPos.x,
			hpPos.y + hpVertSize_.y * 0.5f
		};
		// HPフレームのアンカーは(0.5, 0.5)、HPフレームの位置はhpPosなので、HPフレームの中心はhpPosにある。
		hpFill_->SetPosition(basePosHPFill_);

		// HPアイコンの位置 = HPフレームの位置 + アイコンのオフセット + アイコンの高さの半分（アイコンのアンカーが中心なので）
		float iconH = hpIconDrawSize_.y;
		Vector2 iconPos{ // HPフレームの位置 + アイコンのオフセット + アイコンの高さの半分（アイコンのアンカーが中心なので）
			basePosHPFill_.x + hpIconOffset_.x,
			basePosHPFill_.y + (iconH * 0.5f) + hpIconOffset_.y
		};
		// 位置を保存してスプライトに適用
		hpIcon_->SetPosition(iconPos);

	}

	void PlayerHudUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;

		// ゲームパッド接続状態を初期化
		isGamepadConnected_ = Input::GetInstance()->IsGamepadConnected();
		// RBゲージUIとLBゲージUIを生成して初期化
		rbGaugeUI_ = std::make_unique<RBGaugeUI>();
		RBGaugeUI::Desc rbDesc{};
		rbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, rbDesc);
		// LBゲージはRBゲージと同じサイズ・位置で、分割数だけ異なる設定で初期化する
		lbGaugeUI_ = std::make_unique<LBGaugeUI>();
		LBGaugeUI::Desc lbDesc{};
		lbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, lbDesc);
		// RBゲージアイコンを生成して初期化
		rbGaugeIcon_ = std::make_unique<Sprite>();
		rbGaugeIcon_->Initialize(spriteCommon_, dxCommon_, rbGaugeIconTex_);
		rbGaugeIcon_->SetAutoAdjustTextureSize(false);
		rbGaugeIcon_->SetAnchorPoint({ 1.0f, 1.0f });

		{ // RBゲージアイコンのテクスチャサイズを取得して描画サイズを計算・設定する
			const auto& meta = TextureManager::GetInstance()->GetMetadata(rbGaugeIconTex_); // メタデータを取得
			rbGaugeIconTexSize_ = { (float)meta.width, (float)meta.height }; // テクスチャサイズを保存
			rbGaugeIcon_->SetTextureLeftTop({ 0.0f, 0.0f }); // 左上を(0,0)に設定
			rbGaugeIcon_->SetTextureSize(rbGaugeIconTexSize_); // スプライトに設定
			// 描画サイズ = テクスチャサイズ * スケールで計算して保存
			rbGaugeIconDrawSize_ = {
				rbGaugeIconTexSize_.x * rbGaugeIconScale_,
				rbGaugeIconTexSize_.y * rbGaugeIconScale_
			};
			rbGaugeIcon_->SetSize(rbGaugeIconDrawSize_); // スプライトに描画サイズを設定
		}

		// HPゲージのスプライトを生成して初期化
		hpFrame_ = std::make_unique<Sprite>();
		hpFill_ = std::make_unique<Sprite>();
		hpIcon_ = std::make_unique<Sprite>();
		// HPゲージのテクスチャパス
		const std::string hpFrameTex = "./resources/texture/player_hp_frame.jpg"; // フレームのテクスチャ
		const std::string hpFillTex = "./resources/texture/player_hp.jpg"; // 塗りのテクスチャ
		const std::string hpIconTex = "./resources/texture/player_hp.png"; // アイコンのテクスチャ
		// スプライトを初期化
		hpFrame_->Initialize(spriteCommon_, dxCommon_, hpFrameTex);
		hpFill_->Initialize(spriteCommon_, dxCommon_, hpFillTex);
		hpIcon_->Initialize(spriteCommon_, dxCommon_, hpIconTex);
		// テクスチャサイズを取得してスプライトに設定するため、自動調整をオフにする
		hpFrame_->SetAutoAdjustTextureSize(false);
		hpFill_->SetAutoAdjustTextureSize(false);
		hpIcon_->SetAutoAdjustTextureSize(false);

		{ // HPゲージのテクスチャサイズを取得してスプライトに設定する
			const auto& frameMeta = TextureManager::GetInstance()->GetMetadata(hpFrameTex);
			hpFrame_->SetTextureLeftTop({ 0.0f, 0.0f }); // 左上を(0,0)に設定
			hpFrame_->SetTextureSize({ (float)frameMeta.width, (float)frameMeta.height }); // スプライトに設定

			const auto& fillMeta = TextureManager::GetInstance()->GetMetadata(hpFillTex);
			hpFill_->SetTextureLeftTop({ 0.0f, 0.0f }); // 左上を(0,0)に設定
			hpFill_->SetTextureSize({ (float)fillMeta.width, (float)fillMeta.height }); // スプライトに設定

			const auto& iconMeta = TextureManager::GetInstance()->GetMetadata(hpIconTex);
			hpIconTexSize_ = { (float)iconMeta.width, (float)iconMeta.height }; // テクスチャサイズを保存
			hpIcon_->SetTextureLeftTop({ 0.0f, 0.0f }); // 左上を(0,0)に設定
			hpIcon_->SetTextureSize(hpIconTexSize_); // スプライトに設定
		}
		// アンカーポイントを設定
		hpFrame_->SetAnchorPoint({ 0.5f, 0.5f });
		hpFill_->SetAnchorPoint({ 0.5f, 1.0f });
		hpIcon_->SetAnchorPoint({ 0.5f, 0.5f });
		// 描画サイズを計算して設定
		hpFrame_->SetSize({ hpVertSize_.x + hpFramePad_, hpVertSize_.y + hpFramePad_ });
		hpFill_->SetSize(hpVertSize_);

		hpIconDrawSize_ = { // 描画サイズ = テクスチャサイズ * スケールで計算
			hpIconTexSize_.x * hpIconScale_,
			hpIconTexSize_.y * hpIconScale_
		};
		hpIcon_->SetSize(hpIconDrawSize_); // スプライトに描画サイズを設定
		// 画面サイズに基づいて位置を計算して適用
		UpdateLayout(screenW_, screenH_);
	}

	void PlayerHudUI::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;
		// 画面サイズに基づいて位置を再計算して適用
		hpFill_->SetSize(hpVertSize_);
		hpFrame_->SetSize({ hpVertSize_.x + hpFramePad_, hpVertSize_.y + hpFramePad_ });
		// HPアイコンの描画サイズ = テクスチャサイズ * スケールで計算して保存・設定
		hpIconDrawSize_ = {
			hpIconTexSize_.x * hpIconScale_,
			hpIconTexSize_.y * hpIconScale_
		};
		hpIcon_->SetSize(hpIconDrawSize_);
		// RBゲージアイコンの描画サイズ = テクスチャサイズ * スケールで計算して保存・設定
		rbGaugeIconDrawSize_ = {
			rbGaugeIconTexSize_.x * rbGaugeIconScale_,
			rbGaugeIconTexSize_.y * rbGaugeIconScale_
		};
		rbGaugeIcon_->SetSize(rbGaugeIconDrawSize_);

		// HUDの位置を再計算して適用
		ApplyHudPositions_();
	}

	void PlayerHudUI::Update(float dt, Player* player) {
		Input* in = Input::GetInstance();
		isGamepadConnected_ = in->IsGamepadConnected();

		// RBボタンとLBボタンの押下状態を取得（ゲームパッド接続時はゲームパッドのボタン、非接続時はキーボードのキーで判定）
		const bool rbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)
			: in->PushKey(DIK_K);
		// LBボタンはゲームパッドの左肩ボタン、キーボードのLキーで判定
		const bool lbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER)
			: in->PushKey(DIK_L);


		// プレイヤー情報をもとにゲージUIを更新
		rbGaugeUI_->Update(dt, player->GetRbAmmo(), player->GetRbAmmoMax(), player->IsRbRefilling(), rbDown); // RBゲージUIを更新
		lbGaugeUI_->Update(dt, player->GetLbAmmo(), player->GetLbAmmoMax(), lbDown); // LBゲージUIを更新
		rbGaugeIcon_->Update(); // RBゲージアイコンは常に更新して位置を反映させる
		hpFrame_->Update(); // HPフレームは常に更新して位置を反映させる
		hpIcon_->Update(); // HPアイコンは常に更新して位置を反映させる

		// RBゲージアイコンはRBボタンが押されている間揺らす
		if (rbDown) {
			float r1 = MyMath::Rand01() * 2.0f - 1.0f; // -1.0～1.0の乱数
			float r2 = MyMath::Rand01() * 2.0f - 1.0f; // -1.0～1.0の乱数
			// アイコンの位置 = 基準位置 + 乱数 * 揺れの強さ
			rbGaugeIcon_->SetPosition({
				basePosRBGaugeIcon_.x + r1 * shakeAmpPx_,
				basePosRBGaugeIcon_.y + r2 * shakeAmpPx_
				});
		} else { // 押されていないときは基準位置に戻す
			rbGaugeIcon_->SetPosition(basePosRBGaugeIcon_);
			shakeT_RBGaugeIcon_ = 0.0f;
		}

		// HPゲージの更新
		hpTargetRate_ = player->GetHPRate();
		hpTargetRate_ = std::clamp(hpTargetRate_, 0.0f, 1.0f);

		// HPの現在値を取得
		int curHp = player->GetHP();

		// 前回のHPが未初期化（-1）なら、現在のHPを基準にしてアニメーション率を設定しておく
		if (prevHp_ < 0) {
			prevHp_ = curHp;
			hpAnimRate_ = hpTargetRate_;
			hpTweenActive_ = false;
		}

		// HPが減少していたらダメージを受けたと判断して、ヒットフラッシュとシェイクを開始し、HPアニメーションのトゥイーンをリセットする
		bool damaged = (curHp < prevHp_);

		// ダメージを受けたときの処理
		if (damaged) {
			hpHitFlashT_ = hpHitFlashSec_;
			hpShakeT_ = hpShakeSec_;
			hpTween_.Reset(hpAnimRate_, hpTargetRate_, hpDrainEaseSec_, hpDrainEaseType_);
			hpTweenActive_ = true;
		}

		prevHp_ = curHp; // 現在のHPを保存して次回の更新で比較できるようにする

		// HPアニメーションの更新
		if (hpTweenActive_) {
			hpAnimRate_ = hpTween_.Update(dt); // トゥイーンを更新してHPアニメーション率を取得
			// トゥイーンが終了していたら、HPアニメーション率を目標値に直接設定してトゥイーンを非アクティブにする
			if (hpTween_.Finished()) {
				hpAnimRate_ = hpTargetRate_;
				hpTweenActive_ = false;
			}
		} else { // トゥイーンがアクティブでないときは、HPアニメーション率を目標値に直接設定しておく（この場合はHPの増加なども即座に反映される）
			hpAnimRate_ = hpTargetRate_;
		}

		// HPアニメーション率をもとにHPゲージの塗りのサイズを更新する
		Vector2 fillSize = hpVertSize_;
		fillSize.y *= hpAnimRate_;
		hpFill_->SetSize(fillSize);

		// HPシェイクの更新
		if (hpShakeT_ > 0.0f) {
			hpShakeT_ -= dt;
			// シェイク時間が終了していたら0にする
			if (hpShakeT_ < 0.0f) {
				hpShakeT_ = 0.0f;
			}

			// シェイクのオフセット = (-1.0～1.0の乱数) * 揺れの強さ
			float r1 = MyMath::Rand01() * 2.0f - 1.0f;
			float r2 = MyMath::Rand01() * 2.0f - 1.0f;
			Vector2 ofs{ r1 * hpShakeAmpPx_, r2 * hpShakeAmpPx_ };
			// HPフレームとHP塗りの位置 = 基準位置 + シェイクのオフセット
			hpFrame_->SetPosition({ basePosHPFrame_.x + ofs.x, basePosHPFrame_.y + ofs.y });
			hpFill_->SetPosition({ basePosHPFill_.x + ofs.x, basePosHPFill_.y + ofs.y });
		} else { // シェイク時間が終了しているときは基準位置に戻す		{
			hpFrame_->SetPosition(basePosHPFrame_);
			hpFill_->SetPosition(basePosHPFill_);
		}

		// HPヒットフラッシュの更新
		if (hpHitFlashT_ > 0.0f) {
			hpHitFlashT_ -= dt;
			if (hpHitFlashT_ < 0.0f) {
				hpHitFlashT_ = 0.0f;
			}
		}

		// HPフレームとHPアイコンは常に更新して位置を反映させる
		hpFill_->Update();

		// ImGui調整
		DrawImGui();
	}

	void PlayerHudUI::Draw(float hudAlpha) {
		// HUD全体のアルファを乗算する関数
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 out = c;
			out.w *= hudAlpha;
			return out;
			};
		// HPゲージのフレームとアイコンを描画
		hpFrame_->SetColor(mulAlpha(colHPFrame_));
		hpFrame_->Draw();
		// HPアイコンはヒットフラッシュの影響を受けないように、フレームと同じアルファで描画する
		hpIcon_->SetColor(mulAlpha(colHPIcon_));
		hpIcon_->Draw();
		// HPゲージの塗りを描画（ヒットフラッシュの影響を受ける）
		float t = 0.0f;
		// ヒットフラッシュの経過時間をもとに、フラッシュの色と通常の色を線形補間するための割合を計算する
		if (hpHitFlashSec_ > 0.0f) {
			t = hpHitFlashT_ / hpHitFlashSec_;
			t = std::clamp(t, 0.0f, 1.0f);
		}
		Vector4 flashCol{ // フラッシュの色は赤で、アルファはHPゲージの通常の色と同じにする
			1.0f,
			0.0f,
			0.0f,
			colHPFill_.w
		};
		// 描画色 = 通常の色 * (1 - t) + フラッシュの色 * t で線形補間する
		Vector4 drawCol = MyMath::Vector4Lerp(colHPFill_, flashCol, t);
		hpFill_->SetColor(mulAlpha(drawCol));
		hpFill_->Draw();
		// RBゲージアイコンとゲージを描画
		rbGaugeIcon_->SetColor(mulAlpha(colRBGaugeIcon_));
		rbGaugeIcon_->Draw();
		rbGaugeUI_->Draw();
		lbGaugeUI_->Draw();
	}

	void PlayerHudUI::DrawImGui() {
#ifdef USE_IMGUI
		if (ImGui::TreeNode("プレイヤーHUD")) {
			bool changed = false;

			if (ImGui::TreeNode("RBゲージアイコン")) {
				changed |= ImGui::DragFloat("サイズ##rbGaugeIcon", &rbGaugeIconScale_, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat("右端余白(px)##rbGaugeIconPad", &rbGaugeIconPadX_, 0.5f, 0.0f, 200.0f);
				changed |= ImGui::DragFloat2("微調整オフセット##rbGaugeIcon", &rbGaugeIconOffset_.x, 0.5f, -1500.0f, 300.0f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("LBゲージ（残弾5分割）")) {
				changed |= ImGui::DragFloat("RBの下の間隔Y(px)##lbGaugeSpace", &lbGaugeSpacingY_, 0.5f, 0.0f, 200.0f);
				changed |= ImGui::DragFloat2("LBゲージ微調整(x,y)##lbGaugeOfs", &lbGaugeOffset_.x, 0.5f, -500.0f, 500.0f);

				if (lbGaugeUI_) {
					auto ld = lbGaugeUI_->GetDesc();
					bool localChanged = false;

					localChanged |= ImGui::DragFloat("内側余白 pad(px)##lbPad", &ld.pad_, 0.1f, 0.0f, 20.0f);
					localChanged |= ImGui::DragFloat("分割の隙間 gap(px)##lbGap", &ld.gap_, 0.1f, 0.0f, 20.0f);
					localChanged |= ImGui::ColorEdit4("通常色##lbBase", &ld.baseColor_.x);
					localChanged |= ImGui::ColorEdit4("消費色##lbDrain", &ld.drainColor_.x);
					localChanged |= ImGui::ColorEdit4("回復色##lbRefill", &ld.refillColor_.x);

					if (localChanged) {
						lbGaugeUI_->SetDesc(ld);
					}
				}

				ImGui::TreePop();
			}

			ImGui::Separator();
			ImGui::Text("HPアイコン");
			changed |= ImGui::DragFloat("HPアイコン scale", &hpIconScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("HPアイコン offset(x,y)", &hpIconOffset_.x, 0.5f, -300.0f, 300.0f);

			ImGui::Separator();
			ImGui::Text("左下HUD：配置");
			changed |= ImGui::DragFloat("左余白(px)", &hudLeftMargin_, 0.5f, 0.0f, 600.0f);
			changed |= ImGui::DragFloat("縦ゲージ確保幅(px)", &hudReserveLeftW_, 0.5f, 0.0f, 800.0f);
			changed |= ImGui::DragFloat("確保幅の右の間隔(px)", &hudReserveGap_, 0.5f, 0.0f, 300.0f);
			changed |= ImGui::DragFloat("下余白(px)", &hudBottomMargin_, 0.5f, 0.0f, 300.0f);
			changed |= ImGui::DragFloat("弾UI上げ量(px)", &ammoUiRaiseY_, 0.5f, 0.0f, 300.0f);

			ImGui::Separator();
			ImGui::Text("縦HPゲージ");
			changed |= ImGui::DragFloat2("HP縦サイズ(w,h)", &hpVertSize_.x, 0.5f, 2.0f, 800.0f);
			changed |= ImGui::DragFloat2("HP縦オフセット(x,y)", &hpVertOffset_.x, 0.5f, -300.0f, 300.0f);
			changed |= ImGui::DragFloat("HPフレーム余白", &hpFramePad_, 0.5f, 0.0f, 80.0f);

			if (changed) {
				UpdateLayout(screenW_, screenH_);
			}

			ImGui::TreePop();
		}
#endif
	}
} // namespace TKM