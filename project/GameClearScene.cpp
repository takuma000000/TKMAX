#include "GameClearScene.h"
#include "Input.h"
#include "TitleScene.h"
#include "ImGuiManager.h"

void GameClearScene::Initialize() {
	// 「ゲームクリア」演出の初期化（スプライトやBGMなど）
}

void GameClearScene::Finalize() {
	// 解放処理（必要に応じて）
}

void GameClearScene::Update() {
	Input::GetInstance()->Update();

	// Aボタンでタイトルへ
	if (Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A)) {
		sceneManager_->SetNextScene(new TitleScene(dxCommon, srvManager));
	}

	UpdatePerformanceInfo();
}

void GameClearScene::Draw() {
	// UI描画など（背景画像 or テキスト等）
}
