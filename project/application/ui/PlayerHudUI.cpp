#include "PlayerHudUI.h"
#include "TextureManager.h"
#include "Input.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void PlayerHudUI::ApplyHudPositions_() {
		// 左下のHP用予約領域を避けて、弾ゲージの左端X座標を決める
		float leftEdgeX = hudLeftMargin_ + hudReserveLeftW_ + hudReserveGap_;

		// RBゲージを配置する
		if (rbGaugeUI_) {
			auto rbDesc = rbGaugeUI_->GetDesc();

			// RBゲージの中心座標を計算する
			Vector2 rbCenter{
				leftEdgeX + rbDesc.size_.x * 0.5f,
				screenH_ - hudBottomMargin_ - ammoUiRaiseY_
			};

			// RBゲージの中心座標を反映する
			rbDesc.center_ = rbCenter;
			rbGaugeUI_->SetDesc(rbDesc);

			// LBゲージはRBゲージの下に配置する
			if (lbGaugeUI_) {
				auto lbDesc = lbGaugeUI_->GetDesc();

				// LBゲージのサイズはRBゲージと揃える
				lbDesc.size_ = rbDesc.size_;

				// RBゲージの下に、間隔と微調整オフセットを加えて配置する
				lbDesc.center_ = {
					rbCenter.x + lbGaugeOffset_.x,
					rbCenter.y + rbDesc.size_.y + lbGaugeSpacingY_ + lbGaugeOffset_.y
				};

				// LBゲージは弾数に合わせて5分割表示にする
				lbDesc.segments_ = 5;

				// LBゲージの設定を反映する
				lbGaugeUI_->SetDesc(lbDesc);
			}

			// RBゲージアイコンをRBゲージの右端に配置する
			if (rbGaugeIcon_) {
				// アイコンの描画サイズを取得する
				float iconW = rbGaugeIconDrawSize_.x;
				float iconH = rbGaugeIconDrawSize_.y;

				// RBゲージの右端X座標を求める
				float gaugeRightX = rbCenter.x + rbDesc.size_.x * 0.5f;

				// アイコンのアンカーが右下なので、幅と高さを考慮して位置を決める
				Vector2 iconPos{
					gaugeRightX + rbGaugeIconPadX_ + iconW,
					rbCenter.y + iconH * 0.5f
				};

				// ImGui調整用のオフセットを加える
				iconPos.x += rbGaugeIconOffset_.x;
				iconPos.y += rbGaugeIconOffset_.y;

				// 基準位置を保存して、スプライトに反映する
				basePosRBGaugeIcon_ = iconPos;
				rbGaugeIcon_->SetPosition(basePosRBGaugeIcon_);
			}
		}

		// HPゲージは左下の予約領域内に配置する
		float hpX = hudLeftMargin_ + (hudReserveLeftW_ * 0.5f);
		float hpY = screenH_ - hudBottomMargin_;

		// HPゲージ全体の基準位置を計算する
		Vector2 hpPos{ hpX + hpVertOffset_.x, hpY + hpVertOffset_.y };

		// HPフレームの基準位置を保存して反映する
		basePosHPFrame_ = hpPos;
		hpFrame_->SetPosition(basePosHPFrame_);
		// HP塗りは下端基準なので、フレーム位置から半分上へずらす
		basePosHPFill_ = {
			hpPos.x,
			hpPos.y + hpVertSize_.y * 0.5f
		};
		hpFill_->SetPosition(basePosHPFill_);

		// HPアイコンはHP塗り位置を基準にして配置する
		float iconH = hpIconDrawSize_.y;
		Vector2 iconPos{
			basePosHPFill_.x + hpIconOffset_.x,
			basePosHPFill_.y + (iconH * 0.5f) + hpIconOffset_.y
		};

		// HPアイコンの位置を反映する
		hpIcon_->SetPosition(iconPos);
	}

	void PlayerHudUI::ApplyHpSegmentPositions_(const Vector2& shakeOffset) {
		// セグメント用スプライトが全て揃っているか確認する
		if (hpOuterFrameSegments_.empty() ||
			hpBackSegments_.empty() ||
			hpFillSegments_.empty()) {
			return;
		}

		// セグメントが無い場合は何もしない
		if (hpBackSegments_.empty() || hpFillSegments_.empty()) { return; }

		// 分割数を安全な値にする
		const int segmentCount = std::max(1, hpSegmentCount_);

		// ゲージ全体の高さから、1セグメントの高さを計算する
		const float totalGap = hpSegmentGap_ * static_cast<float>(segmentCount - 1);
		const float segmentH = (hpVertSize_.y - totalGap) / static_cast<float>(segmentCount);

		// HPゲージの下端位置を基準にする
		const float bottomY = basePosHPFill_.y;

		for (int i = 0; i < segmentCount; ++i) {
			// 下が0、上が1になる割合
			const float t =
				(segmentCount <= 1)
				? 0.0f
				: static_cast<float>(i) / static_cast<float>(segmentCount - 1);

			// 下は細く、上は太くする
			const float segmentW = MyMath::Lerp(hpSegmentMinW_, hpSegmentMaxW_, t);

			// セグメントの中心Xは、全体の基準位置から幅の半分と揺れオフセットを加える
			const float offsetX =
				(segmentW - hpSegmentMaxW_) * 0.5f;

			// セグメントの中心Y
			const float centerY =
				bottomY
				- segmentH * 0.5f
				- static_cast<float>(i) * (segmentH + hpSegmentGap_);

			// 外枠セグメントは中心基準で、塗りセグメントより少し大きくする
			hpOuterFrameSegments_[i]->SetPosition({
				basePosHPFrame_.x + offsetX + shakeOffset.x,
				centerY + shakeOffset.y
				});
			// 外枠は塗りより少し大きくして、隙間を埋める
			hpOuterFrameSegments_[i]->SetSize({
				segmentW + hpOuterFramePad_.x,
				segmentH + hpSegmentGap_ + hpOuterFramePad_.y * 0.05f
				});

			// 背景セグメントは中心基準
			hpBackSegments_[i]->SetPosition({
				basePosHPFrame_.x + offsetX + shakeOffset.x,
				centerY + shakeOffset.y
				});
			// 背景はセグメントサイズぴったり
			hpBackSegments_[i]->SetSize({
				segmentW,
				segmentH
				});
			// 塗りセグメントは下端基準
			hpFillSegments_[i]->SetPosition({
				basePosHPFrame_.x + offsetX + shakeOffset.x,
				centerY + segmentH * 0.5f + shakeOffset.y
				});
		}
	}

	void PlayerHudUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		// 外部から受け取った描画・シーン情報を保存する
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;

		// 現在のゲームパッド接続状態を取得する
		isGamepadConnected_ = Input::GetInstance()->IsGamepadConnected();

		// RBゲージUIを生成して初期化する
		rbGaugeUI_ = std::make_unique<RBGaugeUI>();
		RBGaugeUI::Desc rbDesc{};
		rbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, rbDesc);

		// LBゲージUIを生成して初期化する
		lbGaugeUI_ = std::make_unique<LBGaugeUI>();
		LBGaugeUI::Desc lbDesc{};
		lbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, lbDesc);

		// RBゲージアイコンを生成して初期化する
		rbGaugeIcon_ = std::make_unique<Sprite>();
		rbGaugeIcon_->Initialize(spriteCommon_, dxCommon_, rbGaugeIconTex_);
		rbGaugeIcon_->SetAutoAdjustTextureSize(false);
		rbGaugeIcon_->SetAnchorPoint({ 1.0f, 1.0f });

		{
			// RBゲージアイコンのテクスチャサイズを取得する
			const auto& meta = TextureManager::GetInstance()->GetMetadata(rbGaugeIconTex_);

			// テクスチャサイズを保存する
			rbGaugeIconTexSize_ = { (float)meta.width, (float)meta.height };

			// 画像全体を使用する
			rbGaugeIcon_->SetTextureLeftTop({ 0.0f, 0.0f });
			rbGaugeIcon_->SetTextureSize(rbGaugeIconTexSize_);

			// テクスチャサイズとスケールから描画サイズを計算する
			rbGaugeIconDrawSize_ = {
				rbGaugeIconTexSize_.x * rbGaugeIconScale_,
				rbGaugeIconTexSize_.y * rbGaugeIconScale_
			};

			// 描画サイズを反映する
			rbGaugeIcon_->SetSize(rbGaugeIconDrawSize_);
		}

		// HPゲージ用スプライトを生成する
		hpFrame_ = std::make_unique<Sprite>();
		hpFill_ = std::make_unique<Sprite>();
		hpIcon_ = std::make_unique<Sprite>();

		// HPゲージ用のテクスチャパス
		const std::string hpFrameTex = "./resources/texture/player_hp_frame.jpg";
		const std::string hpFillTex = "./resources/texture/player_hp.jpg";
		const std::string hpIconTex = "./resources/texture/player_hp.png";

		// HPゲージ用スプライトを初期化する
		hpFrame_->Initialize(spriteCommon_, dxCommon_, hpFrameTex);
		hpFill_->Initialize(spriteCommon_, dxCommon_, hpFillTex);
		hpIcon_->Initialize(spriteCommon_, dxCommon_, hpIconTex);

		// 自動サイズ調整を使わず、こちらで明示的にサイズを設定する
		hpFrame_->SetAutoAdjustTextureSize(false);
		hpFill_->SetAutoAdjustTextureSize(false);
		hpIcon_->SetAutoAdjustTextureSize(false);

		{
			// HPフレームの画像全体を使用する
			const auto& frameMeta = TextureManager::GetInstance()->GetMetadata(hpFrameTex);
			hpFrame_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpFrame_->SetTextureSize({ (float)frameMeta.width, (float)frameMeta.height });

			// HP塗りの画像全体を使用する
			const auto& fillMeta = TextureManager::GetInstance()->GetMetadata(hpFillTex);
			hpFill_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpFill_->SetTextureSize({ (float)fillMeta.width, (float)fillMeta.height });

			// HPアイコンの画像全体を使用する
			const auto& iconMeta = TextureManager::GetInstance()->GetMetadata(hpIconTex);
			hpIconTexSize_ = { (float)iconMeta.width, (float)iconMeta.height };
			hpIcon_->SetTextureLeftTop({ 0.0f, 0.0f });
			hpIcon_->SetTextureSize(hpIconTexSize_);
		}

		// HPゲージ各パーツのアンカーを設定する
		hpFrame_->SetAnchorPoint({ 0.5f, 0.5f });
		hpFill_->SetAnchorPoint({ 0.5f, 1.0f });
		hpIcon_->SetAnchorPoint({ 0.5f, 0.5f });

		// HPフレームと塗りの描画サイズを設定する
		hpFrame_->SetSize({ hpVertSize_.x + hpFramePad_, hpVertSize_.y + hpFramePad_ });
		hpFill_->SetSize(hpVertSize_);

		// HPアイコンの描画サイズを計算して反映する
		hpIconDrawSize_ = {
			hpIconTexSize_.x * hpIconScale_,
			hpIconTexSize_.y * hpIconScale_
		};
		hpIcon_->SetSize(hpIconDrawSize_);

		// HPセグメントを生成する
		hpOuterFrameSegments_.clear();
		hpBackSegments_.clear();
		hpFillSegments_.clear();
		// セグメント数分ループして、背景と塗りのスプライトを生成する
		for (int i = 0; i < hpSegmentCount_; ++i) {
			// セグメント用のスプライトを生成して初期化する
			auto outer = std::make_unique<Sprite>();
			outer->Initialize(spriteCommon_, dxCommon_, hpFillTex);
			outer->SetAutoAdjustTextureSize(false);
			outer->SetAnchorPoint({ 0.5f, 0.5f });
			// 外枠セグメントは背景と同じ色で描画する
			auto back = std::make_unique<Sprite>();
			back->Initialize(spriteCommon_, dxCommon_, hpFillTex);
			back->SetAutoAdjustTextureSize(false);
			back->SetAnchorPoint({ 0.5f, 0.5f });
			// 塗りセグメントはHP塗りと同じ色で描画する
			auto fill = std::make_unique<Sprite>();
			fill->Initialize(spriteCommon_, dxCommon_, hpFillTex);
			fill->SetAutoAdjustTextureSize(false);
			fill->SetAnchorPoint({ 0.5f, 1.0f });
			// テクスチャは全て同じものを使用する
			hpOuterFrameSegments_.push_back(std::move(outer));
			hpBackSegments_.push_back(std::move(back));
			hpFillSegments_.push_back(std::move(fill));
		}

		// 画面サイズをもとにHUD全体の位置を決める
		UpdateLayout(screenW_, screenH_);
	}

	void PlayerHudUI::UpdateLayout(float screenW, float screenH) {
		// 画面サイズを更新する
		screenW_ = screenW;
		screenH_ = screenH;

		// HPゲージの描画サイズを再反映する
		hpFill_->SetSize(hpVertSize_);
		hpFrame_->SetSize({ hpVertSize_.x + hpFramePad_, hpVertSize_.y + hpFramePad_ });
		// HPアイコンの描画サイズを再計算する
		hpIconDrawSize_ = {
			hpIconTexSize_.x * hpIconScale_,
			hpIconTexSize_.y * hpIconScale_
		};
		hpIcon_->SetSize(hpIconDrawSize_);

		// RBゲージアイコンの描画サイズを再計算する
		rbGaugeIconDrawSize_ = {
			rbGaugeIconTexSize_.x * rbGaugeIconScale_,
			rbGaugeIconTexSize_.y * rbGaugeIconScale_
		};
		rbGaugeIcon_->SetSize(rbGaugeIconDrawSize_);

		// HUDの配置を再計算して反映する
		ApplyHudPositions_();
	}

	void PlayerHudUI::OnHudStateChanged(const Player::HudState& state) {
		// Playerから通知されたHUD表示用の状態を保存する
		hudState_ = state;
	}

	void PlayerHudUI::Update(float dt) {
		Input* in = Input::GetInstance();

		// 現在のゲームパッド接続状態を取得する
		isGamepadConnected_ = in->IsGamepadConnected();

		// RB/Kの押下状態を取得する
		const bool rbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)
			: in->PushKey(DIK_K);

		// LB/Lの押下状態を取得する
		const bool lbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER)
			: in->PushKey(DIK_L);

		// Playerから通知されたRB弾数情報をもとにRBゲージを更新する
		rbGaugeUI_->Update(
			dt,
			hudState_.rbAmmo_,
			hudState_.rbAmmoMax_,
			hudState_.rbRefilling_,
			rbDown
		);

		// Playerから通知されたLB弾数情報をもとにLBゲージを更新する
		lbGaugeUI_->Update(
			dt,
			hudState_.lbAmmo_,
			hudState_.lbAmmoMax_,
			lbDown
		);

		// 各スプライトの内部更新を行う
		rbGaugeIcon_->Update();
		hpFrame_->Update();
		hpIcon_->Update();

		// RBゲージアイコンはRB入力中だけ小刻みに揺らす
		if (rbDown) {
			float r1 = MyMath::Rand01() * 2.0f - 1.0f;
			float r2 = MyMath::Rand01() * 2.0f - 1.0f;

			rbGaugeIcon_->SetPosition({
				basePosRBGaugeIcon_.x + r1 * shakeAmpPx_,
				basePosRBGaugeIcon_.y + r2 * shakeAmpPx_
				});
		} else {
			// 入力していない場合は基準位置に戻す
			rbGaugeIcon_->SetPosition(basePosRBGaugeIcon_);
			shakeT_RBGaugeIcon_ = 0.0f;
		}

		// Playerから通知されたHP情報をもとに割合を計算する
		hpTargetRate_ = 0.0f;
		if (hudState_.maxHp_ > 0) {
			hpTargetRate_ =
				static_cast<float>(hudState_.currentHp_) /
				static_cast<float>(hudState_.maxHp_);
		}
		hpTargetRate_ = std::clamp(hpTargetRate_, 0.0f, 1.0f);

		// 現在HPを取得する
		int curHp = hudState_.currentHp_;

		// 初回だけ、前回HPとアニメーション率を現在値で初期化する
		if (prevHp_ < 0) {
			prevHp_ = curHp;
			hpAnimRate_ = hpTargetRate_;
			hpTweenActive_ = false;
		}

		// HPが減っていればダメージを受けたと判断する
		bool damaged = (curHp < prevHp_);

		// ダメージ時はフラッシュ・シェイク・HP減少Tweenを開始する
		if (damaged) {
			hpHitFlashT_ = hpHitFlashSec_;
			hpShakeT_ = hpShakeSec_;
			hpTween_.Reset(hpAnimRate_, hpTargetRate_, hpDrainEaseSec_, hpDrainEaseType_);
			hpTweenActive_ = true;
		}

		// 次フレームの比較用に現在HPを保存する
		prevHp_ = curHp;

		// HP減少アニメーションを更新する
		if (hpTweenActive_) {
			hpAnimRate_ = hpTween_.Update(dt);

			// 安全のため0～1にクランプする
			hpAnimRate_ = std::clamp(hpAnimRate_, 0.0f, 1.0f);

			// Tweenが終わったら目標値に固定する
			if (hpTween_.Finished()) {
				hpAnimRate_ = hpTargetRate_;
				hpTweenActive_ = false;
			}
		} else {
			hpAnimRate_ = hpTargetRate_;
		}

		// 被弾フラッシュ時間を減らす
		if (hpHitFlashT_ > 0.0f) {
			hpHitFlashT_ -= dt;
			if (hpHitFlashT_ < 0.0f) {
				hpHitFlashT_ = 0.0f;
			}
		}

		// HPシェイク時間を減らす
		if (hpShakeT_ > 0.0f) {
			hpShakeT_ -= dt;
			if (hpShakeT_ < 0.0f) {
				hpShakeT_ = 0.0f;
			}
		}

		// HPセグメントの塗り量を反映する
		const int segmentCount = std::max(1, hpSegmentCount_);
		const float totalGap = hpSegmentGap_ * static_cast<float>(segmentCount - 1);
		const float segmentH = (hpVertSize_.y - totalGap) / static_cast<float>(segmentCount);
		// シェイクオフセットを計算する
		for (int i = 0; i < segmentCount; ++i) {
			// 下が0、上が1になる割合
			const float t =
				(segmentCount <= 1)
				? 0.0f
				: static_cast<float>(i) / static_cast<float>(segmentCount - 1);
			// 下は細く、上は太くする
			const float segmentW = MyMath::Lerp(hpSegmentMinW_, hpSegmentMaxW_, t);
			const float segmentStart = static_cast<float>(i) / static_cast<float>(segmentCount);
			const float segmentEnd = static_cast<float>(i + 1) / static_cast<float>(segmentCount);
			// HPがこのセグメントまで残っているかを判定する
			const bool isFilled = hpAnimRate_ >= segmentEnd;
			// セグメントは途中で縮めず、表示するなら1ブロック丸ごと表示する
			hpFillSegments_[i]->SetSize({
				segmentW,
				segmentH
				});

			// セグメントの色を決める
			Vector4 fillColor = colHPFill_;
			// 残りHPが2以下なら、HPが残っているブロックだけ赤く点滅させる
			if (hudState_.currentHp_ <= 2) {

				// 0.0 ～ 1.0 を往復する値を作る
				float pulse =
					(static_cast<float>(std::sin(ImGui::GetTime() * 6.0)) + 1.0f) * 0.5f;

				// 通常色 → 赤色 を補間
				fillColor = MyMath::Vector4Lerp(
					colHPFill_,
					{ 1.0f, 0.0f, 0.0f, colHPFill_.w },
					pulse
				);
			}
			// HPが残っていないセグメントは最後に透明にする
			fillColor.w *= isFilled ? 1.0f : 0.0f;
			// 被弾フラッシュ中は赤みを強くする
			if (hpHitFlashT_ > 0.0f) {
				fillColor = { 1.0f, 0.25f, 0.25f, fillColor.w };
			}

			// セグメントの色を反映する
			hpOuterFrameSegments_[i]->SetColor(colHPOuterFrame_);
			hpBackSegments_[i]->SetColor(colHPBackSegment_);
			hpFillSegments_[i]->SetColor(fillColor);
			hpOuterFrameSegments_[i]->Update();
			hpBackSegments_[i]->Update();
			hpFillSegments_[i]->Update();
		}

		// HPゲージ色を反映する
		Vector4 hpColor = colHPFill_;
		if (hpHitFlashT_ > 0.0f) {
			hpColor = { 1.0f, 0.25f, 0.25f, colHPFill_.w };
		}
		hpFill_->SetColor(hpColor);

		// HPフレーム・アイコン色を反映する
		hpFrame_->SetColor(colHPFrame_);
		hpIcon_->SetColor(colHPIcon_);

		// HP被弾時だけ小刻みに揺らす
		if (hpShakeT_ > 0.0f) {
			float r1 = MyMath::Rand01() * 2.0f - 1.0f;
			float r2 = MyMath::Rand01() * 2.0f - 1.0f;

			ApplyHpSegmentPositions_({
				r1 * hpShakePower_,
				r2 * hpShakePower_
				});
		} else {
			// シェイクしていないときは基準位置に戻す
			ApplyHpSegmentPositions_();
		}

		// ImGui
		DrawImGui();
	}

	void PlayerHudUI::Draw(float hudAlpha) {

		// HUD全体のアルファを各UI色へ掛ける
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 out = c;
			out.w *= hudAlpha;
			return out;
			};

		for (auto& segment : hpOuterFrameSegments_) {
			segment->SetColor(mulAlpha(colHPOuterFrame_));
			segment->Draw();
		}

		// HP背景セグメントを描画する
		for (auto& segment : hpBackSegments_) {
			segment->SetColor(mulAlpha(colHPBackSegment_));
			segment->Draw();
		}
		// HP塗りセグメントを描画する
		for (auto& segment : hpFillSegments_) {
			segment->Draw();
		}

		// HPヒットフラッシュの割合を求める
		float t = 0.0f;
		if (hpHitFlashSec_ > 0.0f) {
			t = hpHitFlashT_ / hpHitFlashSec_;
			t = std::clamp(t, 0.0f, 1.0f);
		}

		// フラッシュ時の赤色を作る
		Vector4 flashCol{
			1.0f,
			0.0f,
			0.0f,
			colHPFill_.w
		};

		// 通常色から赤色へ補間して、ダメージ感を出す
		Vector4 drawCol = MyMath::Vector4Lerp(colHPFill_, flashCol, t);

		// RBゲージアイコンを描画する
		rbGaugeIcon_->SetColor(mulAlpha(colRBGaugeIcon_));
		rbGaugeIcon_->Draw();

		// RB/LBゲージを描画する
		rbGaugeUI_->Draw();
		lbGaugeUI_->Draw();
	}

	void PlayerHudUI::DrawImGui() {
#ifdef USE_IMGUI
		if (ImGui::TreeNode("プレイヤーHUD")) {
			bool changed = false;

			if (ImGui::TreeNode("RBゲージアイコン")) {
				// RBゲージ横のアイコンサイズと配置を調整する
				changed |= ImGui::DragFloat("サイズ##rbGaugeIcon", &rbGaugeIconScale_, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat("右端余白(px)##rbGaugeIconPad", &rbGaugeIconPadX_, 0.5f, 0.0f, 200.0f);
				changed |= ImGui::DragFloat2("微調整オフセット##rbGaugeIcon", &rbGaugeIconOffset_.x, 0.5f, -1500.0f, 300.0f);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("LBゲージ（残弾5分割）")) {
				// LBゲージの配置を調整する
				changed |= ImGui::DragFloat("RBの下の間隔Y(px)##lbGaugeSpace", &lbGaugeSpacingY_, 0.5f, 0.0f, 200.0f);
				changed |= ImGui::DragFloat2("LBゲージ微調整(x,y)##lbGaugeOfs", &lbGaugeOffset_.x, 0.5f, -500.0f, 500.0f);

				if (lbGaugeUI_) {
					auto ld = lbGaugeUI_->GetDesc();
					bool localChanged = false;

					// LBゲージ内部の表示設定を調整する
					localChanged |= ImGui::DragFloat("内側余白 pad(px)##lbPad", &ld.pad_, 0.1f, 0.0f, 20.0f);
					localChanged |= ImGui::DragFloat("分割の隙間 gap(px)##lbGap", &ld.gap_, 0.1f, 0.0f, 20.0f);
					localChanged |= ImGui::ColorEdit4("通常色##lbBase", &ld.baseColor_.x);
					localChanged |= ImGui::ColorEdit4("消費色##lbDrain", &ld.drainColor_.x);
					localChanged |= ImGui::ColorEdit4("回復色##lbRefill", &ld.refillColor_.x);

					// LBゲージ内部設定に変更があった場合のみ反映する
					if (localChanged) {
						lbGaugeUI_->SetDesc(ld);
					}
				}

				ImGui::TreePop();
			}

			ImGui::Separator();

			// HPアイコンのサイズと位置を調整する
			ImGui::Text("HPアイコン");
			changed |= ImGui::DragFloat("HPアイコン scale", &hpIconScale_, 0.001f, 0.01f, 2.0f);
			changed |= ImGui::DragFloat2("HPアイコン offset(x,y)", &hpIconOffset_.x, 0.5f, -300.0f, 300.0f);

			ImGui::Separator();

			// 左下HUD全体の配置を調整する
			ImGui::Text("左下HUD：配置");
			changed |= ImGui::DragFloat("左余白(px)", &hudLeftMargin_, 0.5f, 0.0f, 600.0f);
			changed |= ImGui::DragFloat("縦ゲージ確保幅(px)", &hudReserveLeftW_, 0.5f, 0.0f, 800.0f);
			changed |= ImGui::DragFloat("確保幅の右の間隔(px)", &hudReserveGap_, 0.5f, 0.0f, 300.0f);
			changed |= ImGui::DragFloat("下余白(px)", &hudBottomMargin_, 0.5f, 0.0f, 300.0f);
			changed |= ImGui::DragFloat("弾UI上げ量(px)", &ammoUiRaiseY_, 0.5f, 0.0f, 300.0f);

			ImGui::Separator();

			// 縦HPゲージのサイズと位置を調整する
			ImGui::Text("縦HPゲージ");
			changed |= ImGui::DragFloat2("HP縦サイズ(w,h)", &hpVertSize_.x, 0.5f, 2.0f, 800.0f);
			changed |= ImGui::DragFloat2("HP縦オフセット(x,y)", &hpVertOffset_.x, 0.5f, -300.0f, 300.0f);
			changed |= ImGui::DragFloat("HPフレーム余白", &hpFramePad_, 0.5f, 0.0f, 80.0f);

			// 何か変更があった場合はHUD配置を再計算する
			if (changed) {
				UpdateLayout(screenW_, screenH_);
			}

			ImGui::TreePop();
		}
#endif
	}

} // namespace TKM