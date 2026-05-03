#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"

void GameScene::Initialize() {
}

void GameScene::Finalize() {
}

void GameScene::Update() {
	TKM::Input::GetInstance()->Update();

	if (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager_->ChangeScene("CLEAR");
	}
}

void GameScene::Draw() {
}

void GameScene::Draw3D() {
}

void GameScene::DrawSprite() {
}

void GameScene::DrawBack() {
}