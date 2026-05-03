#include "GameClearScene.h"
#include "Input.h"
#include "SceneManager.h"

void GameClearScene::Initialize() {
}

void GameClearScene::Finalize() {
}

void GameClearScene::Update() {
	TKM::Input::GetInstance()->Update();

	if (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager_->ChangeScene("TITLE");
	}
}

void GameClearScene::Draw() {
}

void GameClearScene::Draw3D() {
}

void GameClearScene::DrawSprite() {
}

void GameClearScene::DrawBack() {
}