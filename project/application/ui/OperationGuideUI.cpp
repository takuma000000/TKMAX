#include "OperationGuideUI.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	std::unique_ptr<Sprite> OperationGuideUI::CreateSprite_(const std::string& texPath, const Vector2& anchor, Vector2* outTexSize) {
		// スプライトを生成する
		auto sp = std::make_unique<Sprite>();

		// 指定されたテクスチャでスプライトを初期化する
		sp->Initialize(spriteCommon_, dxCommon_, texPath);

		// 自動サイズ調整を使わず、こちらで明示的にサイズを設定する
		sp->SetAutoAdjustTextureSize(false);

		// テクスチャ情報から元画像サイズを取得する
		const auto& meta = TextureManager::GetInstance()->GetMetadata(texPath);
		Vector2 texSize{ (float)meta.width, (float)meta.height };

		// 画像全体を使うため、左上座標は(0,0)にする
		sp->SetTextureLeftTop({ 0.0f, 0.0f });

		// 画像全体を使うため、テクスチャサイズをそのまま設定する
		sp->SetTextureSize(texSize);

		// 右下基準など、呼び出し側が指定したアンカーを設定する
		sp->SetAnchorPoint(anchor);

		// 必要なら呼び出し元へテクスチャサイズを返す
		if (outTexSize) {
			*outTexSize = texSize;
		}

		return sp;
	}

	void OperationGuideUI::ApplySpriteTexture_(Sprite* sp, const std::string& texPath, Vector2* outTexSize) {
		// 対象スプライトがない場合は何もしない
		if (!sp) {
			return;
		}

		// 指定テクスチャでスプライトを再初期化する
		sp->Initialize(spriteCommon_, dxCommon_, texPath);

		// 自動サイズ調整を使わず、こちらで明示的にサイズを設定する
		sp->SetAutoAdjustTextureSize(false);

		// テクスチャ情報から元画像サイズを取得する
		const auto& meta = TextureManager::GetInstance()->GetMetadata(texPath);
		Vector2 texSize{ (float)meta.width, (float)meta.height };

		// 画像全体を使うため、左上座標は(0,0)にする
		sp->SetTextureLeftTop({ 0.0f, 0.0f });

		// 画像全体を使うため、テクスチャサイズをそのまま設定する
		sp->SetTextureSize(texSize);

		// 必要なら呼び出し元へテクスチャサイズを返す
		if (outTexSize) {
			*outTexSize = texSize;
		}
	}

	void OperationGuideUI::RefreshGuideTextures_() {
		// 現在の入力デバイスに応じて表示するUI画像を切り替える
		if (isGamepadConnected_) {
			lbTex_ = padLbTex_;
			rbTex_ = padRbTex_;
			xTex_ = padXTex_;
		} else {
			lbTex_ = keyLbTex_;
			rbTex_ = keyRbTex_;
			xTex_ = keyXTex_;
		}

		// 入力デバイスに合わせたテクスチャを各スプライトへ再適用する
		ApplySpriteTexture_(uiLB_.get(), lbTex_, &lbTexSize_);
		ApplySpriteTexture_(uiRB_.get(), rbTex_, &rbTexSize_);
		ApplySpriteTexture_(uiX_.get(), xTex_, &xTexSize_);

		// テクスチャサイズが変わる可能性があるため、描画サイズを再計算する
		ApplyGuideSizes_();

		// サイズ変更後の並びを再計算する
		ApplyGuidePositions_();
	}

	void OperationGuideUI::ApplyGuideSizes_() {
		// 入力デバイスごとに使う拡大率を選択する
		const float lbScale = isGamepadConnected_ ? padLbScale_ : keyLbScale_;
		const float rbScale = isGamepadConnected_ ? padRbScale_ : keyRbScale_;
		const float xScale = isGamepadConnected_ ? padXScale_ : keyXScale_;

		// テクスチャサイズと拡大率から実際の描画サイズを求める
		lbDrawSize_ = { lbTexSize_.x * lbScale, lbTexSize_.y * lbScale };
		rbDrawSize_ = { rbTexSize_.x * rbScale, rbTexSize_.y * rbScale };
		xDrawSize_ = { xTexSize_.x * xScale, xTexSize_.y * xScale };

		// LSはゲームパッド・キーボードで共通のサイズを使う
		lsDrawSize_ = { lsTexSize_.x * lsScale_, lsTexSize_.y * lsScale_ };

		// 計算した描画サイズをスプライトへ反映する
		uiLB_->SetSize(lbDrawSize_);
		uiRB_->SetSize(rbDrawSize_);
		uiX_->SetSize(xDrawSize_);
		uiLS_->SetSize(lsDrawSize_);
	}

	void OperationGuideUI::ApplyGuidePositions_() {
		// 右下基準の配置位置を作る
		const float baseX = screenW_ - rightUiMargin_;
		const float baseY = screenH_ - rightUiMargin_;

		// 入力デバイスごとに位置補正を切り替える
		const Vector2& lbOffset = isGamepadConnected_ ? padLbOffset_ : keyLbOffset_;
		const Vector2& rbOffset = isGamepadConnected_ ? padRbOffset_ : keyRbOffset_;
		const Vector2& xOffset = isGamepadConnected_ ? padXOffset_ : keyXOffset_;

		// RBは右下の基準位置に置く
		Vector2 rbPos{ baseX, baseY };
		rbPos.x += rbOffset.x;
		rbPos.y += rbOffset.y;

		// LBはRBの一段上に置く
		Vector2 lbPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) };
		lbPos.x += lbOffset.x;
		lbPos.y += lbOffset.y;

		// XはLBのさらに上に置く
		Vector2 xPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) };
		xPos.x += xOffset.x;
		xPos.y += xOffset.y;

		// LSはXのさらに上に置く
		Vector2 lsPos{
			baseX,
			baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) - (xDrawSize_.y + rightUiSpacing_)
		};
		lsPos.x += lsOffset_.x;
		lsPos.y += lsOffset_.y;

		// 計算した基準位置を保存する
		basePosRB_ = rbPos;
		basePosLB_ = lbPos;
		basePosX_ = xPos;
		basePosLS_ = lsPos;

		// RBとLBは基準位置をそのまま使う
		uiRB_->SetPosition(basePosRB_);
		uiLB_->SetPosition(basePosLB_);

		// XとLSは入力によるオフセット分も加えて配置する
		uiX_->SetPosition({ basePosX_.x + xCurrentOfs_.x, basePosX_.y + xCurrentOfs_.y });
		uiLS_->SetPosition({ basePosLS_.x + lsCurrentOfs_.x, basePosLS_.y + lsCurrentOfs_.y });
	}

	void OperationGuideUI::ApplyShake_(Sprite* sp, const Vector2& basePos, bool down, float& t) {
		// 対象スプライトがない場合は何もしない
		if (!sp) {
			return;
		}

		// 押されていない場合は揺れ時間を戻し、基準位置へ戻す
		if (!down) {
			t = 0.0f;
			sp->SetPosition(basePos);
			return;
		}

		// -1.0f～1.0fのランダム値を2軸分作る
		float r1 = MyMath::Rand01() * 2.0f - 1.0f;
		float r2 = MyMath::Rand01() * 2.0f - 1.0f;

		// ランダム値に揺れ幅を掛けて、画面上の揺れ量にする
		float sx = r1 * shakeAmpPx_;
		float sy = r2 * shakeAmpPx_;

		// 基準位置に揺れオフセットを足して配置する
		sp->SetPosition({ basePos.x + sx, basePos.y + sy });
	}

	void OperationGuideUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		// 外部から受け取った描画・シーン情報を保存する
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;

		// ゲームパッド用のUI画像パスを設定する
		padLbTex_ = "./resources/texture/LB_ui.png";
		padRbTex_ = "./resources/texture/RB_ui.png";
		padXTex_ = "./resources/texture/X_ui.png";

		// キーボード用のUI画像パスを設定する
		keyLbTex_ = "./resources/texture/L_ui.png";
		keyRbTex_ = "./resources/texture/K_ui.png";
		keyXTex_ = "./resources/texture/J_ui.png";

		// LSはゲームパッド専用だが、画像パス自体は共通で持つ
		lsTex_ = "./resources/texture/LS_ui.png";

		// 初期状態の入力デバイスを取得する
		isGamepadConnected_ = Input::GetInstance()->IsGamepadConnected();
		prevGamepadConnected_ = isGamepadConnected_;

		// 入力デバイスに応じた初期テクスチャを選択する
		lbTex_ = isGamepadConnected_ ? padLbTex_ : keyLbTex_;
		rbTex_ = isGamepadConnected_ ? padRbTex_ : keyRbTex_;
		xTex_ = isGamepadConnected_ ? padXTex_ : keyXTex_;

		// 各操作UI用のスプライトを生成する
		uiLB_ = CreateSprite_(lbTex_, { 1.0f, 1.0f }, &lbTexSize_);
		uiRB_ = CreateSprite_(rbTex_, { 1.0f, 1.0f }, &rbTexSize_);
		uiX_ = CreateSprite_(xTex_, { 1.0f, 1.0f }, &xTexSize_);
		uiLS_ = CreateSprite_(lsTex_, { 1.0f, 1.0f }, &lsTexSize_);

		// 残弾なし表示用の赤バツを生成する
		rbNoAmmoCross_ = CreateSprite_("./resources/texture/cross.png", { 0.5f, 0.5f }, &noAmmoCrossTexSize_);
		lbNoAmmoCross_ = CreateSprite_("./resources/texture/cross.png", { 0.5f, 0.5f }, nullptr);
		xCooldownCross_ = CreateSprite_("./resources/texture/cross.png", { 0.5f, 0.5f }, nullptr);

		// 赤バツの描画サイズを設定する
		rbNoAmmoCross_->SetSize(noAmmoCrossDrawSize_);
		lbNoAmmoCross_->SetSize(noAmmoCrossDrawSize_);
		xCooldownCross_->SetSize(noAmmoCrossDrawSize_);
		xCooldownCross_->SetSize(noAmmoCrossDrawSize_);

		// テクスチャサイズから描画サイズを計算する
		ApplyGuideSizes_();

		// 右下基準で各UIの位置を計算する
		ApplyGuidePositions_();
	}

	void OperationGuideUI::UpdateLayout(float screenW, float screenH) {
		// 画面サイズを更新する
		screenW_ = screenW;
		screenH_ = screenH;

		// 画面サイズに合わせて配置を再計算する
		ApplyGuidePositions_();
	}

	void OperationGuideUI::SetRightUiMargin(float px) {
		// 右下からの余白を更新する
		rightUiMargin_ = px;

		// 余白変更後の位置を再計算する
		ApplyGuidePositions_();
	}

	void OperationGuideUI::SetRightUiSpacing(float px) {
		// 操作UI同士の縦間隔を更新する
		rightUiSpacing_ = px;

		// 間隔変更後の位置を再計算する
		ApplyGuidePositions_();
	}

	void OperationGuideUI::Update(
		float dt,
		bool rbNoAmmo,
		bool lbNoAmmo,
		bool xCooldown
	) {
		Input* in = Input::GetInstance();

		// 残弾なしになった瞬間だけ、赤バツ出現演出を最初から再生する
		if (rbNoAmmo && !prevRbNoAmmo_) {
			rbCrossPopT_ = 0.0f;
		}

		if (lbNoAmmo && !prevLbNoAmmo_) {
			lbCrossPopT_ = 0.0f;
		}

		// 残弾なし状態を保存する
		rbNoAmmo_ = rbNoAmmo;
		lbNoAmmo_ = lbNoAmmo;
		xCooldown_ = xCooldown;

		// クールタイムになった瞬間だけ演出開始
		if (xCooldown_ && !prevXCooldown_) {
			xCrossPopT_ = 0.0f;
		}

		// 現在のゲームパッド接続状態を取得する
		isGamepadConnected_ = in->IsGamepadConnected();

		// 入力デバイスの接続状態が変わった場合は、UI画像を切り替える
		if (isGamepadConnected_ != prevGamepadConnected_) {
			RefreshGuideTextures_();
			prevGamepadConnected_ = isGamepadConnected_;
		}

		// RB/Kの押下状態を取得する
		const bool rbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)
			: in->PushKey(DIK_K);

		// LB/Lの押下状態を取得する
		const bool lbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER)
			: in->PushKey(DIK_L);

		// X/Jの押下状態を取得する
		const bool xDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_X)
			: in->PushKey(DIK_J);

		// 左スティック、またはWASD入力用の生値
		SHORT rawX = 0;
		SHORT rawY = 0;

		// ゲームパッド接続中は左スティックの値を使う
		if (isGamepadConnected_) {
			rawX = in->GetLeftStickX();
			rawY = in->GetLeftStickY();
		}
		// キーボード時はWASDで疑似スティック入力を作る
		else {
			if (in->PushKey(DIK_A)) { rawX -= 32768; }
			if (in->PushKey(DIK_D)) { rawX += 32767; }
			if (in->PushKey(DIK_W)) { rawY += 32767; }
			if (in->PushKey(DIK_S)) { rawY -= 32768; }
		}

		// スティックの生値を-1.0f～1.0fに正規化する
		auto normAxis = [&](SHORT v)->float {
			float f = (v >= 0) ? (float)v / 32767.0f : (float)v / 32768.0f;

			// 念のため範囲を-1.0f～1.0fに収める
			if (f < -1.0f) { f = -1.0f; }
			if (f > 1.0f) { f = 1.0f; }

			return f;
			};

		// 正規化済みの軸入力にデッドゾーンを適用する
		auto applyDeadzone = [&](float a)->float {
			float absA = (a < 0.0f) ? -a : a;

			// デッドゾーン内は入力なしとして扱う
			if (absA <= lsDeadzone_) {
				return 0.0f;
			}

			// デッドゾーン外だけを0.0f～1.0fへ再マッピングする
			float t = (absA - lsDeadzone_) / (1.0f - lsDeadzone_);

			return (a < 0.0f) ? -t : t;
			};

		// 左スティック入力を正規化し、デッドゾーンを反映する
		float lsX = applyDeadzone(normAxis(rawX));
		float lsY = applyDeadzone(normAxis(rawY));

		// LSが動いているか判定する
		bool lsMoving = (lsX != 0.0f) || (lsY != 0.0f);

		// 入力状態に応じて各UIの色を切り替える
		colRB_ = (rbDown && !rbNoAmmo_) ? onCol_ : idleCol_;
		colLB_ = (lbDown && !lbNoAmmo_) ? onCol_ : idleCol_;
		colX_ = (xDown && !xCooldown_) ? onCol_ : idleCol_;
		colLS_ = lsMoving ? onCol_ : idleCol_;

		// 入力状態に応じてアルファを切り替える
		colRB_.w = (rbDown && !rbNoAmmo_) ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colLB_.w = (lbDown && !lbNoAmmo_) ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colX_.w = (xDown && !xCooldown_) ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colLS_.w = lsMoving ? rightUiActiveAlpha_ : rightUiIdleAlpha_;

		// 各スプライトの内部更新を行う
		uiLB_->Update();
		uiRB_->Update();
		uiX_->Update();
		uiLS_->Update();

		// RB/LBは押下中だけ小刻みに揺らす
		ApplyShake_(uiRB_.get(), basePosRB_, rbDown && !rbNoAmmo_, shakeT_RB_);
		ApplyShake_(uiLB_.get(), basePosLB_, lbDown && !lbNoAmmo_, shakeT_LB_);

		// Xが押された瞬間だけ、現在の入力方向へ跳ねる
		bool xTrig = (xDown && !prevXDown_);

		if (xTrig) {
			// スティック方向を画面座標系の移動方向に変換する
			Vector2 dir{ lsX, -lsY };
			float lenSq = dir.x * dir.x + dir.y * dir.y;

			// 入力方向がある場合は正規化する
			if (lenSq > 0.0001f) {
				float len = std::sqrt(lenSq);
				dir.x /= len;
				dir.y /= len;
			}
			// 入力がほぼない場合は上方向へ跳ねさせる
			else {
				dir = { 0.0f, -1.0f };
			}

			// Xの移動目標オフセットを設定する
			xTargetOfs_ = {
				dir.x * xMoveRangePx_,
				dir.y * xMoveRangePx_
			};
		}

		// Xの目標オフセットを徐々に0へ戻す
		float rt = 1.0f - std::exp(-xReturnSpeed_ * dt);
		rt = std::clamp(rt, 0.0f, 1.0f);

		xTargetOfs_.x += (0.0f - xTargetOfs_.x) * rt;
		xTargetOfs_.y += (0.0f - xTargetOfs_.y) * rt;

		// Xの現在オフセットを目標オフセットへ滑らかに追従させる
		float ft = 1.0f - std::exp(-xFollowSpeed_ * dt);
		ft = std::clamp(ft, 0.0f, 1.0f);

		xCurrentOfs_.x += (xTargetOfs_.x - xCurrentOfs_.x) * ft;
		xCurrentOfs_.y += (xTargetOfs_.y - xCurrentOfs_.y) * ft;

		// Xスプライトを基準位置＋現在オフセットへ配置する
		uiX_->SetPosition({
			basePosX_.x + xCurrentOfs_.x,
			basePosX_.y + xCurrentOfs_.y
			});

		// 次フレームのトリガー判定用にXの押下状態を保存する
		prevXDown_ = xDown;

		// LSの目標オフセットを入力方向から作る
		lsTargetOfs_ = {
			lsX * lsMoveRangePx_,
			-lsY * lsMoveRangePx_
		};

		// LSの現在オフセットを目標オフセットへ滑らかに追従させる
		float t = 1.0f - std::exp(-lsFollowSpeed_ * dt);
		t = std::clamp(t, 0.0f, 1.0f);

		lsCurrentOfs_.x += (lsTargetOfs_.x - lsCurrentOfs_.x) * t;
		lsCurrentOfs_.y += (lsTargetOfs_.y - lsCurrentOfs_.y) * t;

		// LSスプライトを基準位置＋現在オフセットへ配置する
		uiLS_->SetPosition({
			basePosLS_.x + lsCurrentOfs_.x,
			basePosLS_.y + lsCurrentOfs_.y
			});


		// 赤バツ出現演出タイマーを進める
		rbCrossPopT_ = std::min(1.0f, rbCrossPopT_ + dt / kCrossPopSec_);
		lbCrossPopT_ = std::min(1.0f, lbCrossPopT_ + dt / kCrossPopSec_);
		// イージングで「ポンッ」と出る倍率を作る
		const float rbEase = Ease::Eval(Ease::Type::OutBack, rbCrossPopT_);
		const float lbEase = Ease::Eval(Ease::Type::OutBack, lbCrossPopT_);
		const float rbScale = MyMath::Lerp(kCrossStartScale_, kCrossEndScale_, rbEase);
		const float lbScale = MyMath::Lerp(kCrossStartScale_, kCrossEndScale_, lbEase);
		const float xEase = Ease::Eval(Ease::Type::OutBack, xCrossPopT_);
		const float xScale = MyMath::Lerp(kCrossStartScale_, kCrossEndScale_, xEase);
		// 赤バツのサイズを反映する
		rbNoAmmoCross_->SetSize({ kCrossBaseSize_ * rbScale, kCrossBaseSize_ * rbScale });
		lbNoAmmoCross_->SetSize({ kCrossBaseSize_ * lbScale, kCrossBaseSize_ * lbScale });
		// 赤バツをRB/LBアイコンの中心に重ねる
		rbNoAmmoCross_->SetPosition({
			basePosRB_.x - rbDrawSize_.x * 0.5f,
			basePosRB_.y - rbDrawSize_.y * 0.5f
			});
		lbNoAmmoCross_->SetPosition({
			basePosLB_.x - lbDrawSize_.x * 0.5f,
			basePosLB_.y - lbDrawSize_.y * 0.5f
			});
		xCooldownCross_->SetPosition({
			basePosX_.x - xDrawSize_.x * 0.5f,
			basePosX_.y - xDrawSize_.y * 0.5f
			});
		// 赤バツスプライトを更新する
		rbNoAmmoCross_->Update();
		lbNoAmmoCross_->Update();
		xCooldownCross_->Update();
		// 次フレーム用に残弾なし状態を保存する
		prevRbNoAmmo_ = rbNoAmmo_;
		prevLbNoAmmo_ = lbNoAmmo_;
		prevXCooldown_ = xCooldown_;

		// ImGui調整項目を表示する
		DrawImGui();
	}

	void OperationGuideUI::Draw(float hudAlpha) {
		// HUD全体のアルファを各UI色へ掛ける
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 out = c;
			out.w *= hudAlpha;
			return out;
			};

		// LB/RB/Xを描画する
		uiLB_->SetColor(mulAlpha(colLB_));
		uiLB_->Draw();

		uiRB_->SetColor(mulAlpha(colRB_));
		uiRB_->Draw();

		uiX_->SetColor(mulAlpha(colX_));
		uiX_->Draw();

		// RB/LBの残弾なし状態を赤バツで表示する
		if (rbNoAmmo_ && rbNoAmmoCross_) {
			rbNoAmmoCross_->Draw();
		}
		if (lbNoAmmo_ && lbNoAmmoCross_) {
			lbNoAmmoCross_->Draw();
		}
		if (xCooldown_ && xCooldownCross_) {
			xCooldownCross_->Draw();
		}

		// LSはゲームパッド接続時のみ表示する
		if (isGamepadConnected_) {
			uiLS_->SetColor(mulAlpha(colLS_));
			uiLS_->Draw();
		}
	}

	void OperationGuideUI::DrawImGui() {
#ifdef USE_IMGUI
		if (ImGui::TreeNode("操作UI")) {
			bool changed = false;

			// 現在の入力デバイスを表示する
			ImGui::Text("入力デバイス : %s", isGamepadConnected_ ? "ゲームパッド" : "キーボード");

			// 操作UI画像を手動で再読み込みする
			if (ImGui::Button("操作UI画像を再読込")) {
				RefreshGuideTextures_();
			}

			ImGui::Separator();

			// 操作UI全体の右下配置を調整する
			changed |= ImGui::DragFloat("右下余白(px)", &rightUiMargin_, 0.5f, 0.0f, 300.0f);
			changed |= ImGui::DragFloat("縦間隔(px)", &rightUiSpacing_, 0.5f, 0.0f, 200.0f);

			if (ImGui::TreeNode(isGamepadConnected_ ? "RB（右バンパー）" : "Kキー")) {
				// 現在の入力デバイスに対応したRB/Kのサイズ・位置補正を調整する
				float& rbScale = isGamepadConnected_ ? padRbScale_ : keyRbScale_;
				Vector2& rbOffset = isGamepadConnected_ ? padRbOffset_ : keyRbOffset_;

				changed |= ImGui::DragFloat("サイズ##rb", &rbScale, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##rb", &rbOffset.x, 0.5f, -500.0f, 500.0f);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode(isGamepadConnected_ ? "LB（左バンパー）" : "Lキー")) {
				// 現在の入力デバイスに対応したLB/Lのサイズ・位置補正を調整する
				float& lbScale = isGamepadConnected_ ? padLbScale_ : keyLbScale_;
				Vector2& lbOffset = isGamepadConnected_ ? padLbOffset_ : keyLbOffset_;

				changed |= ImGui::DragFloat("サイズ##lb", &lbScale, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##lb", &lbOffset.x, 0.5f, -500.0f, 500.0f);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode(isGamepadConnected_ ? "Xボタン" : "Jキー")) {
				// 現在の入力デバイスに対応したX/Jのサイズ・位置補正を調整する
				float& xScale = isGamepadConnected_ ? padXScale_ : keyXScale_;
				Vector2& xOffset = isGamepadConnected_ ? padXOffset_ : keyXOffset_;

				changed |= ImGui::DragFloat("サイズ##x", &xScale, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##x", &xOffset.x, 0.5f, -500.0f, 500.0f);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("左スティック（LS）")) {
				// LSの見た目と入力反応を調整する
				changed |= ImGui::DragFloat("サイズ##ls", &lsScale_, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##ls", &lsOffset_.x, 0.5f, -500.0f, 500.0f);
				changed |= ImGui::DragFloat("移動量(px)##ls", &lsMoveRangePx_, 0.1f, 0.0f, 50.0f);
				changed |= ImGui::DragFloat("デッドゾーン##ls", &lsDeadzone_, 0.01f, 0.0f, 0.95f);

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("色設定##OperationGuide")) {
				// 通常時・入力時の色とアルファを調整する
				changed |= ImGui::ColorEdit4("通常色##guideIdle", &idleCol_.x);
				changed |= ImGui::ColorEdit4("押下色##guideOn", &onCol_.x);
				changed |= ImGui::DragFloat("通常時アルファ##guideIdleAlpha", &rightUiIdleAlpha_, 0.01f, 0.0f, 1.0f);
				changed |= ImGui::DragFloat("入力時アルファ##guideActiveAlpha", &rightUiActiveAlpha_, 0.01f, 0.0f, 1.0f);

				ImGui::TreePop();
			}

			if (ImGui::Button("操作UIリセット")) {
				// 右下配置設定を初期値へ戻す
				rightUiMargin_ = 20.0f;
				rightUiSpacing_ = 10.0f;

				// ゲームパッド用サイズを初期値へ戻す
				padLbScale_ = 0.065f;
				padRbScale_ = 0.114f;
				padXScale_ = 0.066f;

				// キーボード用サイズを初期値へ戻す
				keyLbScale_ = 0.076f;
				keyRbScale_ = 0.074f;
				keyXScale_ = 0.084f;

				// ゲームパッド用位置補正を初期値へ戻す
				padLbOffset_ = { 0.0f, 0.0f };
				padRbOffset_ = { 0.0f, 0.0f };
				padXOffset_ = { 0.0f, 0.0f };

				// キーボード用位置補正を初期値へ戻す
				keyLbOffset_ = { 4.0f, 0.0f };
				keyRbOffset_ = { 1.5f, 0.0f };
				keyXOffset_ = { 5.0f, 0.0f };

				// LS用設定を初期値へ戻す
				lsOffset_ = { 1.0f, -37.5f };
				lsScale_ = 0.064f;

				// 色とアルファを初期値へ戻す
				idleCol_ = { 1.0f, 1.0f, 1.0f, 0.75f };
				onCol_ = { 1.0f, 0.25f, 0.25f, 1.0f };
				rightUiIdleAlpha_ = 0.45f;
				rightUiActiveAlpha_ = 1.0f;

				// 入力オフセットを初期化する
				xCurrentOfs_ = { 0.0f, 0.0f };
				xTargetOfs_ = { 0.0f, 0.0f };
				lsCurrentOfs_ = { 0.0f, 0.0f };
				lsTargetOfs_ = { 0.0f, 0.0f };

				// サイズと位置を再計算するため変更扱いにする
				changed = true;
			}

			// 変更があった場合だけ、サイズと位置を再計算する
			if (changed) {
				ApplyGuideSizes_();
				ApplyGuidePositions_();
			}

			ImGui::TreePop();
		}
#endif
	}

} // namespace TKM