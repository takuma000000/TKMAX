#include "GameOverScene.h"
#include "Input.h"
#include "SceneManager.h"

void GameOverScene::Initialize() {
}

void GameOverScene::Finalize() {
}

void GameOverScene::Update() {
	TKM::Input::GetInstance()->Update();

	if (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager_->ChangeScene("TITLE");
	}
}

void GameOverScene::Draw() {
}

void GameOverScene::Draw3D() {
}

void GameOverScene::DrawSprite() {
}

void GameOverScene::DrawBack() {
}