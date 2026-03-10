#include "OperationGuideUI.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {
	std::unique_ptr<Sprite> OperationGuideUI::CreateSprite_(const std::string& texPath, const Vector2& anchor, Vector2* outTexSize) {
		// スプライトを生成して、画像全体を使う設定にします。
		auto sp = std::make_unique<Sprite>();
		sp->Initialize(spriteCommon_, dxCommon_, texPath);
		sp->SetAutoAdjustTextureSize(false);
		// テクスチャサイズを取得して、スプライトに設定します。
		const auto& meta = TextureManager::GetInstance()->GetMetadata(texPath);
		Vector2 texSize{ (float)meta.width, (float)meta.height };

		sp->SetTextureLeftTop({ 0.0f, 0.0f }); // 画像全体を使うので左上は(0,0)
		sp->SetTextureSize(texSize); // 画像全体を使うのでサイズはテクスチャサイズ
		sp->SetAnchorPoint(anchor); // アンカーポイントを設定

		if (outTexSize) {
			*outTexSize = texSize; // テクスチャサイズを呼び出し元に返す
		}
		return sp;
	}

	void OperationGuideUI::ApplySpriteTexture_(Sprite* sp, const std::string& texPath, Vector2* outTexSize) {
		if (!sp) {
			return;
		}
		// 指定スプライトへテクスチャを再適用します。
		sp->Initialize(spriteCommon_, dxCommon_, texPath);
		sp->SetAutoAdjustTextureSize(false);
		// テクスチャサイズを取得して、スプライトに設定します。
		const auto& meta = TextureManager::GetInstance()->GetMetadata(texPath);
		Vector2 texSize{ (float)meta.width, (float)meta.height };

		sp->SetTextureLeftTop({ 0.0f, 0.0f }); // 画像全体を使うので左上は(0,0)
		sp->SetTextureSize(texSize); // 画像全体を使うのでサイズはテクスチャサイズ

		if (outTexSize) {
			*outTexSize = texSize; // テクスチャサイズを呼び出し元に返す
		}
	}

	void OperationGuideUI::RefreshGuideTextures_() {
		// 現在の入力デバイスに応じてUI画像を切り替えます。
		if (isGamepadConnected_) {
			lbTex_ = padLbTex_;
			rbTex_ = padRbTex_;
			xTex_ = padXTex_;
		} else { // キーボード
			lbTex_ = keyLbTex_;
			rbTex_ = keyRbTex_;
			xTex_ = keyXTex_;
		}

		// スプライトへテクスチャを再適用します。
		ApplySpriteTexture_(uiLB_.get(), lbTex_, &lbTexSize_);
		ApplySpriteTexture_(uiRB_.get(), rbTex_, &rbTexSize_);
		ApplySpriteTexture_(uiX_.get(), xTex_, &xTexSize_);

		// サイズと位置を再計算して適用します。
		ApplyGuideSizes_();
		ApplyGuidePositions_();
	}

	void OperationGuideUI::ApplyGuideSizes_() {
		// 各UIの描画サイズを再計算して適用します。
		const float lbScale = isGamepadConnected_ ? padLbScale_ : keyLbScale_;
		const float rbScale = isGamepadConnected_ ? padRbScale_ : keyRbScale_;
		const float xScale = isGamepadConnected_ ? padXScale_ : keyXScale_;
		// LSは共通サイズ
		lbDrawSize_ = { lbTexSize_.x * lbScale, lbTexSize_.y * lbScale };
		rbDrawSize_ = { rbTexSize_.x * rbScale, rbTexSize_.y * rbScale };
		xDrawSize_ = { xTexSize_.x * xScale, xTexSize_.y * xScale };
		lsDrawSize_ = { lsTexSize_.x * lsScale_, lsTexSize_.y * lsScale_ };
		// スプライトにサイズを適用します。
		uiLB_->SetSize(lbDrawSize_);
		uiRB_->SetSize(rbDrawSize_);
		uiX_->SetSize(xDrawSize_);
		uiLS_->SetSize(lsDrawSize_);
	}

	void OperationGuideUI::ApplyGuidePositions_() {
		// 各UIの基準位置を再計算して適用します。
		const float baseX = screenW_ - rightUiMargin_;
		const float baseY = screenH_ - rightUiMargin_;
		// オフセットも入力デバイスによって切り替えます。
		const Vector2& lbOffset = isGamepadConnected_ ? padLbOffset_ : keyLbOffset_;
		const Vector2& rbOffset = isGamepadConnected_ ? padRbOffset_ : keyRbOffset_;
		const Vector2& xOffset = isGamepadConnected_ ? padXOffset_ : keyXOffset_;
		// LSは共通オフセット
		Vector2 rbPos{ baseX, baseY };
		rbPos.x += rbOffset.x;
		rbPos.y += rbOffset.y;
		// RBを基準にして、下にLB、その下にX、その下にLSが来るように配置します。
		Vector2 lbPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) };
		lbPos.x += lbOffset.x;
		lbPos.y += lbOffset.y;
		// XはLBの下に配置します。
		Vector2 xPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) };
		xPos.x += xOffset.x;
		xPos.y += xOffset.y;
		// LSはXの下に配置します。
		Vector2 lsPos{
			baseX,
			baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) - (xDrawSize_.y + rightUiSpacing_)
		};
		lsPos.x += lsOffset_.x;
		lsPos.y += lsOffset_.y;

		// 基準位置を保存して、スプライトに適用します。
		basePosRB_ = rbPos;
		basePosLB_ = lbPos;
		basePosX_ = xPos;
		basePosLS_ = lsPos;

		// スプライトに位置を適用します。
		uiRB_->SetPosition(basePosRB_);
		uiLB_->SetPosition(basePosLB_);
		uiX_->SetPosition({ basePosX_.x + xCurrentOfs_.x, basePosX_.y + xCurrentOfs_.y });
		uiLS_->SetPosition({ basePosLS_.x + lsCurrentOfs_.x, basePosLS_.y + lsCurrentOfs_.y });
	}

	void OperationGuideUI::ApplyShake_(Sprite* sp, const Vector2& basePos, bool down, float& t) {
		if (!sp) {
			return;
		}

		// 押下中に小刻みに揺らす処理です。
		if (!down) {
			t = 0.0f;
			sp->SetPosition(basePos); // 基準位置に戻す
			return;
		}

		// 揺れの周期を設定します。
		float r1 = MyMath::Rand01() * 2.0f - 1.0f;
		float r2 = MyMath::Rand01() * 2.0f - 1.0f;
		// 揺れの振幅を設定します。
		float sx = r1 * shakeAmpPx_;
		float sy = r2 * shakeAmpPx_;

		sp->SetPosition({ basePos.x + sx, basePos.y + sy }); // 基準位置に揺れオフセットを加算して設定します。
	}

	void OperationGuideUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;

		// テクスチャパスを設定します。
		padLbTex_ = "./resources/texture/LB_ui.png"; // LBは共通テクスチャ
		padRbTex_ = "./resources/texture/RB_ui.png"; // RBは共通テクスチャ
		padXTex_ = "./resources/texture/X_ui.png"; // Xは共通テクスチャ
		// LSは共通テクスチャ
		keyLbTex_ = "./resources/texture/L_ui.png"; // LBは共通テクスチャ
		keyRbTex_ = "./resources/texture/K_ui.png"; // RBは共通テクスチャ
		keyXTex_ = "./resources/texture/J_ui.png"; // Xは共通テクスチャ
		// LSは共通テクスチャ
		lsTex_ = "./resources/texture/LS_ui.png"; // LSは共通テクスチャ

		// 初期状態の入力デバイスに応じてUI画像を切り替えます。
		isGamepadConnected_ = Input::GetInstance()->IsGamepadConnected();
		prevGamepadConnected_ = isGamepadConnected_;
		// テクスチャパスを選択して、スプライトを生成します。
		lbTex_ = isGamepadConnected_ ? padLbTex_ : keyLbTex_;
		rbTex_ = isGamepadConnected_ ? padRbTex_ : keyRbTex_;
		xTex_ = isGamepadConnected_ ? padXTex_ : keyXTex_;
		// LSは共通テクスチャ
		uiLB_ = CreateSprite_(lbTex_, { 1.0f, 1.0f }, &lbTexSize_);
		uiRB_ = CreateSprite_(rbTex_, { 1.0f, 1.0f }, &rbTexSize_);
		uiX_ = CreateSprite_(xTex_, { 1.0f, 1.0f }, &xTexSize_);
		uiLS_ = CreateSprite_(lsTex_, { 1.0f, 1.0f }, &lsTexSize_);

		// サイズと位置を計算して適用します。
		ApplyGuideSizes_();
		ApplyGuidePositions_();
	}

	void OperationGuideUI::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;
		ApplyGuidePositions_(); // 画面サイズの変更に伴い、UIの基準位置を再計算して適用します。
	}

	void OperationGuideUI::SetRightUiMargin(float px) {
		rightUiMargin_ = px;
		ApplyGuidePositions_(); // 余白の変更に伴い、UIの基準位置を再計算して適用します。
	}

	void OperationGuideUI::SetRightUiSpacing(float px) {
		rightUiSpacing_ = px;
		ApplyGuidePositions_(); // 縦間隔の変更に伴い、UIの基準位置を再計算して適用します。
	}

	void OperationGuideUI::Update(float dt) {
		Input* in = Input::GetInstance();
		isGamepadConnected_ = in->IsGamepadConnected();

		// 入力デバイスの接続状態が変化したら、UI画像を切り替えます。
		if (isGamepadConnected_ != prevGamepadConnected_) {
			RefreshGuideTextures_(); // 画像を切り替えて、サイズと位置も再計算して適用します。
			prevGamepadConnected_ = isGamepadConnected_; // 接続状態の変化を保存します。
		}

		// 各入力の押下状態を取得します。ゲームパッドが接続されている場合はゲームパッドのボタン、そうでない場合はキーボードのキーをチェックします。
		const bool rbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)
			: in->PushKey(DIK_K);
		// RBはゲームパッドの右肩ボタン、キーボードのKキー
		const bool lbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER)
			: in->PushKey(DIK_L);
		// LBはゲームパッドの左肩ボタン、キーボードのLキー
		const bool xDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_X)
			: in->PushKey(DIK_J);

		// XはゲームパッドのXボタン、キーボードのJキー
		SHORT rawX = 0;
		SHORT rawY = 0;

		// LSは左スティックの入力を使用します。ゲームパッドが接続されている場合はスティックの値を取得し、そうでない場合はWASDキーで擬似的にスティック入力を作ります。
		if (isGamepadConnected_) {
			rawX = in->GetLeftStickX();
			rawY = in->GetLeftStickY();
		} else { // キーボードの場合はWASDキーで擬似的にスティック入力を作ります。AとSが負方向、DとWが正方向になります。
			if (in->PushKey(DIK_A)) { rawX -= 32768; }
			if (in->PushKey(DIK_D)) { rawX += 32767; }
			if (in->PushKey(DIK_W)) { rawY += 32767; }
			if (in->PushKey(DIK_S)) { rawY -= 32768; }
		}

		// スティックの値は-32768～32767の範囲なので、これを-1.0f～1.0fの範囲に正規化します。負の値は32768で割り、正の値は32767で割ります。
		auto normAxis = [&](SHORT v)->float {
			float f = (v >= 0) ? (float)v / 32767.0f : (float)v / 32768.0f;
			if (f < -1.0f) { f = -1.0f; }
			if (f > 1.0f) { f = 1.0f; }
			return f;
			};
		// スティックの入力にデッドゾーンを適用します。デッドゾーン内の入力は0にし、デッドゾーン外の入力はデッドゾーンを除いた範囲で正規化します。
		auto applyDeadzone = [&](float a)->float {
			float absA = (a < 0.0f) ? -a : a;
			// デッドゾーン内の入力は0にします。
			if (absA <= lsDeadzone_) {
				return 0.0f;
			}
			float t = (absA - lsDeadzone_) / (1.0f - lsDeadzone_);
			return (a < 0.0f) ? -t : t;
			};

		// スティックの入力を正規化してデッドゾーンを適用します。
		float lsX = applyDeadzone(normAxis(rawX));
		float lsY = applyDeadzone(normAxis(rawY));
		bool lsMoving = (lsX != 0.0f) || (lsY != 0.0f);
		// スティックが入力されているかどうかを判定します。
		colRB_ = rbDown ? onCol_ : idleCol_;
		colLB_ = lbDown ? onCol_ : idleCol_;
		colX_ = xDown ? onCol_ : idleCol_;
		colLS_ = lsMoving ? onCol_ : idleCol_;
		// 各UIの色を更新します。押下中はアクティブアルファ、そうでないときはアイドルアルファを使用します。
		colRB_.w = rbDown ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colLB_.w = lbDown ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colX_.w = xDown ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colLS_.w = lsMoving ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		// スプライトの色を更新します。
		uiLB_->Update();
		uiRB_->Update();
		uiX_->Update();
		uiLS_->Update();
		// RBとLBの押下状態に応じて、基準位置から小刻みに揺らす処理を適用します。
		ApplyShake_(uiRB_.get(), basePosRB_, rbDown, shakeT_RB_);
		ApplyShake_(uiLB_.get(), basePosLB_, lbDown, shakeT_LB_);

		// Xの押下状態に応じて、基準位置からのオフセットを計算して、スムーズに移動する処理を適用します。
		bool xTrig = (xDown && !prevXDown_);
		// Xが押された瞬間に、スティックの入力方向に応じたオフセットの目標値を設定します。スティックがニュートラルなら上方向になります。
		if (xTrig) {
			Vector2 dir{ lsX, -lsY };
			float lenSq = dir.x * dir.x + dir.y * dir.y;
			// 入力方向の長さがある程度以上あれば正規化します。そうでなければ上方向を向かせます。
			if (lenSq > 0.0001f) {
				float len = std::sqrt(lenSq);
				dir.x /= len;
				dir.y /= len;
			} else { // 入力がほとんどない場合は上方向を向かせます。
				dir = { 0.0f, -1.0f };
			}
			// 入力方向に応じたオフセットの目標値を設定します。スティックの入力が大きいほど遠くに移動させます。
			xTargetOfs_ = {
				dir.x * xMoveRangePx_,
				dir.y * xMoveRangePx_
			};
		}
		// Xが押されていないときは、オフセットの目標値を基準位置に戻します。
		float rt = 1.0f - std::exp(-xReturnSpeed_ * dt);
		rt = std::clamp(rt, 0.0f, 1.0f);
		// Xが押されていないときは、オフセットの目標値を基準位置に戻します。
		xTargetOfs_.x += (0.0f - xTargetOfs_.x) * rt;
		xTargetOfs_.y += (0.0f - xTargetOfs_.y) * rt;
		// Xのオフセットをスムーズに移動させます。
		float ft = 1.0f - std::exp(-xFollowSpeed_ * dt);
		ft = std::clamp(ft, 0.0f, 1.0f);
		// Xのオフセットをスムーズに移動させます。
		xCurrentOfs_.x += (xTargetOfs_.x - xCurrentOfs_.x) * ft;
		xCurrentOfs_.y += (xTargetOfs_.y - xCurrentOfs_.y) * ft;
		// Xのスプライト位置を、基準位置にオフセットを加算した値に設定します。
		uiX_->SetPosition({
			basePosX_.x + xCurrentOfs_.x,
			basePosX_.y + xCurrentOfs_.y
			});
		prevXDown_ = xDown; // Xの押下状態を保存します。

		// LSの入力に応じて、オフセットの目標値をスティックの入力方向に設定します。スティックがニュートラルならオフセットも0になります。
		lsTargetOfs_ = {
			lsX * lsMoveRangePx_,
			-lsY * lsMoveRangePx_
		};
		// LSのオフセットをスムーズに移動させます。
		float t = 1.0f - std::exp(-lsFollowSpeed_ * dt);
		t = std::clamp(t, 0.0f, 1.0f);
		// LSのオフセットをスムーズに移動させます。
		lsCurrentOfs_.x += (lsTargetOfs_.x - lsCurrentOfs_.x) * t;
		lsCurrentOfs_.y += (lsTargetOfs_.y - lsCurrentOfs_.y) * t;
		// LSのスプライト位置を、基準位置にオフセットを加算した値に設定します。
		uiLS_->SetPosition({
			basePosLS_.x + lsCurrentOfs_.x,
			basePosLS_.y + lsCurrentOfs_.y
			});

		// ImGui調整を表示します。
		DrawImGui();
	}

	void OperationGuideUI::Draw(float hudAlpha) {
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 out = c;
			out.w *= hudAlpha;
			return out;
			};

		// 各UIの色を更新します。押下中はアクティブアルファ、そうでないときはアイドルアルファを使用します。
		uiLB_->SetColor(mulAlpha(colLB_)); uiLB_->Draw();
		uiRB_->SetColor(mulAlpha(colRB_)); uiRB_->Draw();
		uiX_->SetColor(mulAlpha(colX_)); uiX_->Draw();
		// LSはゲームパッド接続時のみ表示します。
		if (isGamepadConnected_) {
				uiLS_->SetColor(mulAlpha(colLS_));
				uiLS_->Draw();
		}
	}

	void OperationGuideUI::DrawImGui() {
#ifdef USE_IMGUI
		if (ImGui::TreeNode("操作UI")) {
			bool changed = false;

			ImGui::Text("入力デバイス : %s", isGamepadConnected_ ? "ゲームパッド" : "キーボード");
			if (ImGui::Button("操作UI画像を再読込")) {
				RefreshGuideTextures_();
			}

			ImGui::Separator();
			changed |= ImGui::DragFloat("右下余白(px)", &rightUiMargin_, 0.5f, 0.0f, 300.0f);
			changed |= ImGui::DragFloat("縦間隔(px)", &rightUiSpacing_, 0.5f, 0.0f, 200.0f);

			if (ImGui::TreeNode(isGamepadConnected_ ? "RB（右バンパー）" : "Kキー")) {
				float& rbScale = isGamepadConnected_ ? padRbScale_ : keyRbScale_;
				Vector2& rbOffset = isGamepadConnected_ ? padRbOffset_ : keyRbOffset_;
				changed |= ImGui::DragFloat("サイズ##rb", &rbScale, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##rb", &rbOffset.x, 0.5f, -500.0f, 500.0f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode(isGamepadConnected_ ? "LB（左バンパー）" : "Lキー")) {
				float& lbScale = isGamepadConnected_ ? padLbScale_ : keyLbScale_;
				Vector2& lbOffset = isGamepadConnected_ ? padLbOffset_ : keyLbOffset_;
				changed |= ImGui::DragFloat("サイズ##lb", &lbScale, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##lb", &lbOffset.x, 0.5f, -500.0f, 500.0f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode(isGamepadConnected_ ? "Xボタン" : "Jキー")) {
				float& xScale = isGamepadConnected_ ? padXScale_ : keyXScale_;
				Vector2& xOffset = isGamepadConnected_ ? padXOffset_ : keyXOffset_;
				changed |= ImGui::DragFloat("サイズ##x", &xScale, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##x", &xOffset.x, 0.5f, -500.0f, 500.0f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("左スティック（LS）")) {
				changed |= ImGui::DragFloat("サイズ##ls", &lsScale_, 0.001f, 0.01f, 2.0f);
				changed |= ImGui::DragFloat2("位置オフセット##ls", &lsOffset_.x, 0.5f, -500.0f, 500.0f);
				changed |= ImGui::DragFloat("移動量(px)##ls", &lsMoveRangePx_, 0.1f, 0.0f, 50.0f);
				changed |= ImGui::DragFloat("デッドゾーン##ls", &lsDeadzone_, 0.01f, 0.0f, 0.95f);
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("色設定##OperationGuide")) {
				changed |= ImGui::ColorEdit4("通常色##guideIdle", &idleCol_.x);
				changed |= ImGui::ColorEdit4("押下色##guideOn", &onCol_.x);
				changed |= ImGui::DragFloat("通常時アルファ##guideIdleAlpha", &rightUiIdleAlpha_, 0.01f, 0.0f, 1.0f);
				changed |= ImGui::DragFloat("入力時アルファ##guideActiveAlpha", &rightUiActiveAlpha_, 0.01f, 0.0f, 1.0f);
				ImGui::TreePop();
			}

			if (ImGui::Button("操作UIリセット")) {
				rightUiMargin_ = 20.0f;
				rightUiSpacing_ = 10.0f;

				padLbScale_ = 0.065f;
				padRbScale_ = 0.114f;
				padXScale_ = 0.066f;

				keyLbScale_ = 0.076f;
				keyRbScale_ = 0.074f;
				keyXScale_ = 0.084f;

				padLbOffset_ = { 0.0f, 0.0f };
				padRbOffset_ = { 0.0f, 0.0f };
				padXOffset_ = { 0.0f, 0.0f };

				keyLbOffset_ = { 4.0f, 0.0f };
				keyRbOffset_ = { 1.5f, 0.0f };
				keyXOffset_ = { 5.0f, 0.0f };

				lsOffset_ = { 1.0f, -37.5f };
				lsScale_ = 0.064f;

				idleCol_ = { 1.0f, 1.0f, 1.0f, 0.75f };
				onCol_ = { 1.0f, 0.25f, 0.25f, 1.0f };
				rightUiIdleAlpha_ = 0.45f;
				rightUiActiveAlpha_ = 1.0f;

				xCurrentOfs_ = { 0.0f, 0.0f };
				xTargetOfs_ = { 0.0f, 0.0f };
				lsCurrentOfs_ = { 0.0f, 0.0f };
				lsTargetOfs_ = { 0.0f, 0.0f };

				changed = true;
			}

			// 変更があった場合は、サイズと位置を再計算して適用します。
			if (changed) {
				ApplyGuideSizes_();
				ApplyGuidePositions_();
			}

			ImGui::TreePop();
		}
#endif
	}
} // namespace TKM