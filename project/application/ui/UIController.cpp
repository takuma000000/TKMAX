#include "UIController.h"
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

namespace TKM {

	void UIController::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon, BaseScene* parentScene, float screenW, float screenH) {
		spriteCommon_ = spriteCommon;
		dxCommon_ = dxCommon;
		parentScene_ = parentScene;
		screenW_ = screenW;
		screenH_ = screenH;

		// 初期値はHUD全体の透明度1.0f（完全不透明）とします。
		hudAlpha_ = 1.0f;

		// 各UIを初期化します。
		operationGuideUI_ = std::make_unique<OperationGuideUI>();
		operationGuideUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, screenW_, screenH_);
		// プレイヤーHUDを初期化します。
		playerHudUI_ = std::make_unique<PlayerHudUI>();
		playerHudUI_->Initialize(spriteCommon_, dxCommon_, parentScene_, screenW_, screenH_);
	}

	void UIController::SetHudAlpha(float a) {
		hudAlpha_ = MyMath::Clamp01(a); // 0.0f〜1.0fの範囲にクランプ
	}

	void UIController::SetRightUiMargin(float px) {
		operationGuideUI_->SetRightUiMargin(px); // 右側UI全体の余白を設定
	}

	void UIController::SetRightUiSpacing(float px) {
		operationGuideUI_->SetRightUiSpacing(px); // 右側UI全体の縦間隔を設定
	}

	void UIController::UpdateLayout(float screenW, float screenH) {
		screenW_ = screenW;
		screenH_ = screenH;

		// 画面サイズ変更時に各UIのレイアウトを更新します。
		operationGuideUI_->UpdateLayout(screenW_, screenH_); // 右側UIのレイアウトを更新
		playerHudUI_->UpdateLayout(screenW_, screenH_); // プレイヤーHUDのレイアウトを更新
	}

	void UIController::Update(float dt, Player* player) {
		// 各UIを更新します。
		operationGuideUI_->Update(dt); // 右側UIを更新
		playerHudUI_->Update(dt, player); // プレイヤーHUDを更新

		// ImGui表示
		DrawImGui();
	}

	void UIController::Draw() {
		// 各UIを描画します。HUD全体の透明度を引数で渡します。
		playerHudUI_->Draw(hudAlpha_); // プレイヤーHUDを描画
		operationGuideUI_->Draw(hudAlpha_); // 右側UIを描画
	}

	void UIController::DrawImGui() {
#ifdef USE_IMGUI
		if (!ImGui::Begin("UIController")) {
			ImGui::End();
			return;
		}

		// UI全体の調整
		ImGui::Text("UI全体設定");
		ImGui::DragFloat("HUD全体アルファ", &hudAlpha_, 0.01f, 0.0f, 1.0f); // HUD全体の透明度を調整するドラッグ可能なスライダー
		hudAlpha_ = MyMath::Clamp01(hudAlpha_); // 0.0f〜1.0fの範囲にクランプ

		ImGui::Separator(); // 区切り線

		// 右側UIのImGui表示
		operationGuideUI_->DrawImGui();

		ImGui::Separator(); // 区切り線

		// プレイヤーHUDのImGui表示
		playerHudUI_->DrawImGui();

		ImGui::End();
#endif
	}

} // namespace TKM