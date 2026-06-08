#include "PlayerHudUI.h"
#include "TextureManager.h"
#include "Input.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void PlayerHudUI::ApplyHudPositions_() {
		// LBゲージは左下の予約領域内に配置する。左端のX座標を計算する
		float leftEdgeX = hudLeftMargin_ + hudReserveLeftW_ + hudReserveGap_;

		// LBゲージの基準位置を計算して保存する
		if (lbGaugeUI_) {
			auto lbDesc = lbGaugeUI_->GetDesc(); // LBゲージの現在の描画情報を取得する
			// LBゲージの中心座標を、左端からゲージ幅の半分とオフセットを加えた位置にする
			Vector2 lbCenter{
				leftEdgeX + lbDesc.size_.x * 0.5f + lbGaugeOffset_.x,
				screenH_ - hudBottomMargin_ - ammoUiRaiseY_ + lbDesc.size_.y + lbGaugeSpacingY_ + lbGaugeOffset_.y
			};
			// LBゲージの描画情報を更新して反映する
			lbDesc.center_ = lbCenter;
			lbDesc.segments_ = 5;
			lbGaugeUI_->SetDesc(lbDesc); // LBゲージの描画情報を更新して反映する
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

	void PlayerHudUI::ApplyConfig_() {
		// HUD設定ファイルを読み込む
		config_.Load("./resources/data/playerHudConfig.json");

		// LBゲージ設定
		lbGaugeSpacingY_ = config_.GetLBGauge().spacingY_;
		lbGaugeOffset_ = config_.GetLBGauge().offset_;

		// HPゲージ設定
		hpShakePower_ = config_.GetHPGauge().shakePower_;
		colHPFrame_ = config_.GetHPGauge().frameColor_;
		colHPFill_ = config_.GetHPGauge().fillColor_;
		colHPIcon_ = config_.GetHPGauge().iconColor_;
		hpVertSize_ = config_.GetHPGauge().size_;
		hpVertOffset_ = config_.GetHPGauge().offset_;
		hpFramePad_ = config_.GetHPGauge().framePad_;
		hpIconOffset_ = config_.GetHPGauge().iconOffset_;
		hpIconScale_ = config_.GetHPGauge().iconScale_;
		hpSegmentCount_ = config_.GetHPGauge().segmentCount_;
		hpSegmentGap_ = config_.GetHPGauge().segmentGap_;
		hpSegmentMinW_ = config_.GetHPGauge().segmentMinW_;
		hpSegmentMaxW_ = config_.GetHPGauge().segmentMaxW_;
		hpSegmentSkewX_ = config_.GetHPGauge().segmentSkewX_;
		colHPBackSegment_ = config_.GetHPGauge().backSegmentColor_;
		colHPOuterFrame_ = config_.GetHPGauge().outerFrameColor_;
		hpOuterFramePad_ = config_.GetHPGauge().outerFramePad_;

		// 左下HUD配置設定
		ammoUiRaiseY_ = config_.GetLayout().ammoUiRaiseY_;
		hudLeftMargin_ = config_.GetLayout().hudLeftMargin_;
		hudReserveLeftW_ = config_.GetLayout().hudReserveLeftW_;
		hudReserveGap_ = config_.GetLayout().hudReserveGap_;
		hudBottomMargin_ = config_.GetLayout().hudBottomMargin_;

		// HP演出設定
		hpHitFlashSec_ = config_.GetHpEffect().hitFlashSec_;
		hpShakeSec_ = config_.GetHpEffect().shakeSec_;
		hpShakeAmpPx_ = config_.GetHpEffect().shakeAmpPx_;
		hpDrainEaseSec_ = config_.GetHpEffect().drainEaseSec_;
	}

	void PlayerHudUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH, Player* player) {
		// 外部から受け取った描画・シーン情報を保存する
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;
		player_ = player;

		// 外部設定を読み込んで反映する
		ApplyConfig_();

		// 現在のゲームパッド接続状態を取得する
		isGamepadConnected_ = Input::GetInstance()->IsGamepadConnected();

		// LBゲージUIを生成して初期化する
		lbGaugeUI_ = std::make_unique<LBGaugeUI>();
		LBGaugeUI::Desc lbDesc{};
		lbGaugeUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, lbDesc);

		// 回避クールタイムUIを生成して初期化する
		dodgeUI_ = std::make_unique<DodgeUI>();
		dodgeUI_->Initialize(spriteCommon_, dxCommon_);

		// HPゲージ用スプライトを生成する
		hpFrame_ = std::make_unique<Sprite>();
		hpFill_ = std::make_unique<Sprite>();
		hpIcon_ = std::make_unique<Sprite>();

		// HPゲージ用テクスチャパスを設定ファイルから取得する
		const std::string hpFrameTex = config_.GetTexture().hpFrameTex_;
		const std::string hpFillTex = config_.GetTexture().hpFillTex_;
		const std::string hpIconTex = config_.GetTexture().hpIconTex_;

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

		// HUDの配置を再計算して反映する
		ApplyHudPositions_();
	}

	void PlayerHudUI::OnHudStateChanged(const Player::HudState& state) {
		// Playerから通知されたHUD表示用の状態を保存する
		hudState_ = state;
	}

	void PlayerHudUI::Update(float dt) {
		Input* in = Input::GetInstance();

		// HP点滅演出用時間を進める
		hpPulseTime_ += dt;

		// 現在のゲームパッド接続状態を取得する
		isGamepadConnected_ = in->IsGamepadConnected();

		// RB/Kの押下状態を取得する
		const bool rbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)
			: in->PushKey(DIK_K);
		// Playerから通知されたRB弾数情報をもとにRBゲージを更新する
		const bool rbShooting = rbDown;
		// LB/Lの押下状態を取得する
		const bool lbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER)
			: in->PushKey(DIK_L);

		// Playerから通知されたLB弾数情報をもとにLBゲージを更新する
		lbGaugeUI_->Update(
			dt,
			hudState_.lbAmmo_,
			hudState_.lbAmmoMax_,
			lbDown
		);

		hpFrame_->Update();
		hpIcon_->Update();

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

				// 点滅の割合を0～1で求める
				float pulse =
					(std::sin(hpPulseTime_ * 6.0f) + 1.0f) * 0.5f;

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
			// シェイクの強さは時間経過とともに減らす
			ApplyHpSegmentPositions_({
				r1 * hpShakePower_,
				r2 * hpShakePower_
				});
		} else {
			// シェイクしていないときは基準位置に戻す
			ApplyHpSegmentPositions_();
		}

		// 回避クールタイムUIを更新する
		dodgeUI_->Update(player_, screenW_, screenH_);

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

		// HPアイコンを描画する
		hpIcon_->SetColor(mulAlpha(colHPIcon_));
		hpIcon_->Draw();

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

		// HP塗りを描画する
		lbGaugeUI_->Draw();
		// 回避クールタイムUIを描画する
		dodgeUI_->Draw(hudAlpha);
	}

	void PlayerHudUI::DrawImGui() {
#ifdef USE_IMGUI
		if (ImGui::TreeNode("プレイヤーHUD")) {
			bool changed = false;

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