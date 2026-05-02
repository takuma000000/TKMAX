#include "UIController.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void UIController::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH, Player* player) {
		// 外部から受け取った描画・シーン情報を保存する
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;

		// HUD全体の透明度を完全不透明で初期化する
		hudAlpha_ = 1.0f;

		// 右側の操作ガイドUIを生成して初期化する
		operationGuideUI_ = std::make_unique<OperationGuideUI>();
		operationGuideUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, screenW_, screenH_);

		// プレイヤーのHP・弾数などを表示するHUDを生成して初期化する
		playerHudUI_ = std::make_unique<PlayerHudUI>();
		playerHudUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, screenW_, screenH_);

		// イントロ中に表示するスキップガイドUIを生成して初期化する
		skipGuideUI_ = std::make_unique<SkipGuideUI>();
		skipGuideUI_->Initialize(spriteCommon_, dxCommon_, screenW_, screenH_);

		// PlayerのHUD状態変化をHUDに通知するコールバック関数として登録する
		if (player) {
			player->AddHudObserver([this](const Player::HudState& state) { // PlayerからHUD状態の通知を受け取ったときの処理
				hudState_ = state; // 受け取ったHUD状態を保存する
				playerHudUI_->OnHudStateChanged(state); // プレイヤーHUDに状態変化を通知する

				});
		}
	}

	void UIController::SetHudAlpha(float a) {
		// HUD全体の透明度を0.0f～1.0fに収めて設定する
		hudAlpha_ = MyMath::Clamp01(a);
	}

	void UIController::SetRightUiMargin(float px) {
		// 右側操作UIの右下余白を設定する
		operationGuideUI_->SetRightUiMargin(px);
	}

	void UIController::SetRightUiSpacing(float px) {
		// 右側操作UI同士の縦間隔を設定する
		operationGuideUI_->SetRightUiSpacing(px);
	}

	void UIController::SetIntroSkipUiActive(bool active) {
		// イントロ中のスキップUIを使うかどうかを設定する
		introSkipUiActive_ = active;
	}

	void UIController::SetGameplayHudVisible(bool visible) {
		// 通常ゲーム中HUDを表示するかどうかを設定する
		gameplayHudVisible_ = visible;
	}

	void UIController::UpdateLayout(float screenW, float screenH) {
		// 画面サイズを保存する
		screenW_ = screenW;
		screenH_ = screenH;

		// 右側操作UIの配置を画面サイズに合わせて更新する
		operationGuideUI_->UpdateLayout(screenW_, screenH_);

		// プレイヤーHUDの配置を画面サイズに合わせて更新する
		playerHudUI_->UpdateLayout(screenW_, screenH_);
	}

	void UIController::Update(float dt) {
		// イントロ中はスキップガイドだけを有効状態で更新する
		if (introSkipUiActive_) {
			skipGuideUI_->Update(dt, true);
		} else {
			// ゲーム開始後だけ通常HUDを更新する
			if (gameplayHudVisible_) {

				// RBとLBの残弾なし状態を取得する（playerがnullptrの場合はfalse扱い）
				const bool rbNoAmmo = hudState_.rbAmmo_ <= 0;
				const bool lbNoAmmo = hudState_.lbAmmo_ <= 0;
				// 右側の操作ガイドUIを更新する
				operationGuideUI_->Update(dt, rbNoAmmo, lbNoAmmo);

				// プレイヤーHUDを更新する
				playerHudUI_->Update(dt);
			}

			// イントロ中でない場合は、スキップUIを非アクティブ状態で更新して通常状態へ戻す
			skipGuideUI_->Update(dt, false);
		}

		// UI調整用のImGuiを更新する
		DrawImGui();
	}

	void UIController::Draw() {
		// イントロ中はスキップガイドだけを描画する
		if (introSkipUiActive_) {
			skipGuideUI_->Draw(hudAlpha_);
			return;
		}

		// ゲーム開始後だけ通常HUDを描画する
		if (gameplayHudVisible_) {
			playerHudUI_->Draw(hudAlpha_);
			operationGuideUI_->Draw(hudAlpha_);
		}
	}

	void UIController::DrawImGui() {
#ifdef USE_IMGUI
		// UIController用のImGuiウィンドウを開始する
		if (!ImGui::Begin("UIController")) {
			ImGui::End();
			return;
		}

		// UI全体の設定を表示する
		ImGui::Text("UI全体設定");

		// HUD全体の透明度を調整する
		ImGui::DragFloat("HUD全体アルファ", &hudAlpha_, 0.01f, 0.0f, 1.0f);

		// ImGui操作後も透明度を0.0f～1.0fに収める
		hudAlpha_ = MyMath::Clamp01(hudAlpha_);

		ImGui::Separator();

		// 右側操作ガイドUIの調整項目を表示する
		operationGuideUI_->DrawImGui();

		ImGui::Separator();

		// プレイヤーHUDの調整項目を表示する
		playerHudUI_->DrawImGui();

		ImGui::Separator();

		// イントロ用スキップガイドUIの調整項目を表示する
		skipGuideUI_->DrawImGui();

		ImGui::End();
#endif
	}

} // namespace TKM