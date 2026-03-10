#include "OperationGuideUI.h"
#include "TextureManager.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	std::unique_ptr<Sprite> OperationGuideUI::CreateSprite_(const std::string& texPath, const Vector2& anchor, Vector2* outTexSize) {
		auto sp = std::make_unique<Sprite>();
		sp->Initialize(spriteCommon_, dxCommon_, texPath);
		sp->SetAutoAdjustTextureSize(false);

		const auto& meta = TextureManager::GetInstance()->GetMetadata(texPath);
		Vector2 texSize{ (float)meta.width, (float)meta.height };

		sp->SetTextureLeftTop({ 0.0f, 0.0f });
		sp->SetTextureSize(texSize);
		sp->SetAnchorPoint(anchor);

		if (outTexSize) {
			*outTexSize = texSize;
		}
		return sp;
	}

	void OperationGuideUI::ApplySpriteTexture_(Sprite* sp, const std::string& texPath, Vector2* outTexSize) {
		if (!sp) {
			return;
		}

		sp->Initialize(spriteCommon_, dxCommon_, texPath);
		sp->SetAutoAdjustTextureSize(false);

		const auto& meta = TextureManager::GetInstance()->GetMetadata(texPath);
		Vector2 texSize{ (float)meta.width, (float)meta.height };

		sp->SetTextureLeftTop({ 0.0f, 0.0f });
		sp->SetTextureSize(texSize);

		if (outTexSize) {
			*outTexSize = texSize;
		}
	}

	void OperationGuideUI::RefreshGuideTextures_() {
		if (isGamepadConnected_) {
			lbTex_ = padLbTex_;
			rbTex_ = padRbTex_;
			xTex_ = padXTex_;
		} else {
			lbTex_ = keyLbTex_;
			rbTex_ = keyRbTex_;
			xTex_ = keyXTex_;
		}

		ApplySpriteTexture_(uiLB_.get(), lbTex_, &lbTexSize_);
		ApplySpriteTexture_(uiRB_.get(), rbTex_, &rbTexSize_);
		ApplySpriteTexture_(uiX_.get(), xTex_, &xTexSize_);

		ApplyGuideSizes_();
		ApplyGuidePositions_();
	}

	void OperationGuideUI::ApplyGuideSizes_() {
		const float lbScale = isGamepadConnected_ ? padLbScale_ : keyLbScale_;
		const float rbScale = isGamepadConnected_ ? padRbScale_ : keyRbScale_;
		const float xScale = isGamepadConnected_ ? padXScale_ : keyXScale_;

		lbDrawSize_ = { lbTexSize_.x * lbScale, lbTexSize_.y * lbScale };
		rbDrawSize_ = { rbTexSize_.x * rbScale, rbTexSize_.y * rbScale };
		xDrawSize_ = { xTexSize_.x * xScale, xTexSize_.y * xScale };
		lsDrawSize_ = { lsTexSize_.x * lsScale_, lsTexSize_.y * lsScale_ };

		if (uiLB_) { uiLB_->SetSize(lbDrawSize_); }
		if (uiRB_) { uiRB_->SetSize(rbDrawSize_); }
		if (uiX_) { uiX_->SetSize(xDrawSize_); }
		if (uiLS_) { uiLS_->SetSize(lsDrawSize_); }
	}

	void OperationGuideUI::ApplyGuidePositions_() {
		const float baseX = screenW_ - rightUiMargin_;
		const float baseY = screenH_ - rightUiMargin_;

		const Vector2& lbOffset = isGamepadConnected_ ? padLbOffset_ : keyLbOffset_;
		const Vector2& rbOffset = isGamepadConnected_ ? padRbOffset_ : keyRbOffset_;
		const Vector2& xOffset = isGamepadConnected_ ? padXOffset_ : keyXOffset_;

		Vector2 rbPos{ baseX, baseY };
		rbPos.x += rbOffset.x;
		rbPos.y += rbOffset.y;

		Vector2 lbPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) };
		lbPos.x += lbOffset.x;
		lbPos.y += lbOffset.y;

		Vector2 xPos{ baseX, baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) };
		xPos.x += xOffset.x;
		xPos.y += xOffset.y;

		Vector2 lsPos{
			baseX,
			baseY - (rbDrawSize_.y + rightUiSpacing_) - (lbDrawSize_.y + rightUiSpacing_) - (xDrawSize_.y + rightUiSpacing_)
		};
		lsPos.x += lsOffset_.x;
		lsPos.y += lsOffset_.y;

		basePosRB_ = rbPos;
		basePosLB_ = lbPos;
		basePosX_ = xPos;
		basePosLS_ = lsPos;

		if (uiRB_) { uiRB_->SetPosition(basePosRB_); }
		if (uiLB_) { uiLB_->SetPosition(basePosLB_); }
		if (uiX_) { uiX_->SetPosition({ basePosX_.x + xCurrentOfs_.x, basePosX_.y + xCurrentOfs_.y }); }
		if (uiLS_) { uiLS_->SetPosition({ basePosLS_.x + lsCurrentOfs_.x, basePosLS_.y + lsCurrentOfs_.y }); }
	}

	void OperationGuideUI::ApplyShake_(Sprite* sp, const Vector2& basePos, bool down, float& t) {
		if (!sp) {
			return;
		}

		if (!down) {
			t = 0.0f;
			sp->SetPosition(basePos);
			return;
		}

		float r1 = MyMath::Rand01() * 2.0f - 1.0f;
		float r2 = MyMath::Rand01() * 2.0f - 1.0f;

		float sx = r1 * shakeAmpPx_;
		float sy = r2 * shakeAmpPx_;

		sp->SetPosition({ basePos.x + sx, basePos.y + sy });
	}

	void OperationGuideUI::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;

		padLbTex_ = "./resources/texture/LB_ui.png";
		padRbTex_ = "./resources/texture/RB_ui.png";
		padXTex_ = "./resources/texture/X_ui.png";

		keyLbTex_ = "./resources/texture/L_ui.png";
		keyRbTex_ = "./resources/texture/K_ui.png";
		keyXTex_ = "./resources/texture/J_ui.png";

		lsTex_ = "./resources/texture/LS_ui.png";

		isGamepadConnected_ = Input::GetInstance()->IsGamepadConnected();
		prevGamepadConnected_ = isGamepadConnected_;

		lbTex_ = isGamepadConnected_ ? padLbTex_ : keyLbTex_;
		rbTex_ = isGamepadConnected_ ? padRbTex_ : keyRbTex_;
		xTex_ = isGamepadConnected_ ? padXTex_ : keyXTex_;

		uiLB_ = CreateSprite_(lbTex_, { 1.0f, 1.0f }, &lbTexSize_);
		uiRB_ = CreateSprite_(rbTex_, { 1.0f, 1.0f }, &rbTexSize_);
		uiX_ = CreateSprite_(xTex_, { 1.0f, 1.0f }, &xTexSize_);
		uiLS_ = CreateSprite_(lsTex_, { 1.0f, 1.0f }, &lsTexSize_);

		ApplyGuideSizes_();
		ApplyGuidePositions_();
	}

	void OperationGuideUI::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;
		ApplyGuidePositions_();
	}

	void OperationGuideUI::SetRightUiMargin(float px) {
		rightUiMargin_ = px;
		ApplyGuidePositions_();
	}

	void OperationGuideUI::SetRightUiSpacing(float px) {
		rightUiSpacing_ = px;
		ApplyGuidePositions_();
	}

	void OperationGuideUI::Update(float dt) {
		Input* in = Input::GetInstance();

		isGamepadConnected_ = in->IsGamepadConnected();

		if (isGamepadConnected_ != prevGamepadConnected_) {
			RefreshGuideTextures_();
			prevGamepadConnected_ = isGamepadConnected_;
		}

		const bool rbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)
			: in->PushKey(DIK_K);

		const bool lbDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER)
			: in->PushKey(DIK_L);

		const bool xDown = isGamepadConnected_
			? in->PushButton(XINPUT_GAMEPAD_X)
			: in->PushKey(DIK_J);

		SHORT rawX = 0;
		SHORT rawY = 0;

		if (isGamepadConnected_) {
			rawX = in->GetLeftStickX();
			rawY = in->GetLeftStickY();
		} else {
			if (in->PushKey(DIK_A)) { rawX -= 32768; }
			if (in->PushKey(DIK_D)) { rawX += 32767; }
			if (in->PushKey(DIK_W)) { rawY += 32767; }
			if (in->PushKey(DIK_S)) { rawY -= 32768; }
		}

		auto normAxis = [&](SHORT v)->float {
			float f = (v >= 0) ? (float)v / 32767.0f : (float)v / 32768.0f;
			if (f < -1.0f) { f = -1.0f; }
			if (f > 1.0f) { f = 1.0f; }
			return f;
			};

		auto applyDeadzone = [&](float a)->float {
			float absA = (a < 0.0f) ? -a : a;
			if (absA <= lsDeadzone_) {
				return 0.0f;
			}
			float t = (absA - lsDeadzone_) / (1.0f - lsDeadzone_);
			return (a < 0.0f) ? -t : t;
			};

		float lsX = applyDeadzone(normAxis(rawX));
		float lsY = applyDeadzone(normAxis(rawY));
		bool lsMoving = (lsX != 0.0f) || (lsY != 0.0f);

		colRB_ = rbDown ? onCol_ : idleCol_;
		colLB_ = lbDown ? onCol_ : idleCol_;
		colX_ = xDown ? onCol_ : idleCol_;
		colLS_ = lsMoving ? onCol_ : idleCol_;

		colRB_.w = rbDown ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colLB_.w = lbDown ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colX_.w = xDown ? rightUiActiveAlpha_ : rightUiIdleAlpha_;
		colLS_.w = lsMoving ? rightUiActiveAlpha_ : rightUiIdleAlpha_;

		if (uiLB_) { uiLB_->Update(); }
		if (uiRB_) { uiRB_->Update(); }
		if (uiX_) { uiX_->Update(); }
		if (uiLS_) { uiLS_->Update(); }

		if (uiRB_) { ApplyShake_(uiRB_.get(), basePosRB_, rbDown, shakeT_RB_); }
		if (uiLB_) { ApplyShake_(uiLB_.get(), basePosLB_, lbDown, shakeT_LB_); }

		if (uiX_) {
			bool xTrig = (xDown && !prevXDown_);

			if (xTrig) {
				Vector2 dir{ lsX, -lsY };
				float lenSq = dir.x * dir.x + dir.y * dir.y;

				if (lenSq > 0.0001f) {
					float len = std::sqrt(lenSq);
					dir.x /= len;
					dir.y /= len;
				} else {
					dir = { 0.0f, -1.0f };
				}

				xTargetOfs_ = {
					dir.x * xMoveRangePx_,
					dir.y * xMoveRangePx_
				};
			}

			float rt = 1.0f - std::exp(-xReturnSpeed_ * dt);
			rt = std::clamp(rt, 0.0f, 1.0f);

			xTargetOfs_.x += (0.0f - xTargetOfs_.x) * rt;
			xTargetOfs_.y += (0.0f - xTargetOfs_.y) * rt;

			float ft = 1.0f - std::exp(-xFollowSpeed_ * dt);
			ft = std::clamp(ft, 0.0f, 1.0f);

			xCurrentOfs_.x += (xTargetOfs_.x - xCurrentOfs_.x) * ft;
			xCurrentOfs_.y += (xTargetOfs_.y - xCurrentOfs_.y) * ft;

			uiX_->SetPosition({
				basePosX_.x + xCurrentOfs_.x,
				basePosX_.y + xCurrentOfs_.y
				});
		}
		prevXDown_ = xDown;

		if (uiLS_) {
			lsTargetOfs_ = {
				lsX * lsMoveRangePx_,
				-lsY * lsMoveRangePx_
			};

			float t = 1.0f - std::exp(-lsFollowSpeed_ * dt);
			t = std::clamp(t, 0.0f, 1.0f);

			lsCurrentOfs_.x += (lsTargetOfs_.x - lsCurrentOfs_.x) * t;
			lsCurrentOfs_.y += (lsTargetOfs_.y - lsCurrentOfs_.y) * t;

			uiLS_->SetPosition({
				basePosLS_.x + lsCurrentOfs_.x,
				basePosLS_.y + lsCurrentOfs_.y
				});
		}

		DrawImGui();
	}

	void OperationGuideUI::Draw(float hudAlpha) {
		auto mulAlpha = [&](const Vector4& c) {
			Vector4 out = c;
			out.w *= hudAlpha;
			return out;
			};

		if (uiLB_) { uiLB_->SetColor(mulAlpha(colLB_)); uiLB_->Draw(); }
		if (uiRB_) { uiRB_->SetColor(mulAlpha(colRB_)); uiRB_->Draw(); }
		if (uiX_) { uiX_->SetColor(mulAlpha(colX_)); uiX_->Draw(); }

		if (isGamepadConnected_) {
			if (uiLS_) {
				uiLS_->SetColor(mulAlpha(colLS_));
				uiLS_->Draw();
			}
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

			if (changed) {
				ApplyGuideSizes_();
				ApplyGuidePositions_();
			}

			ImGui::TreePop();
		}
#endif
	}

} // namespace TKM