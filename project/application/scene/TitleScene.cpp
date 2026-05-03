#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"

void TitleScene::Initialize() {
}

void TitleScene::Finalize() {
}

void TitleScene::Update() {
	TKM::Input::GetInstance()->Update();

	if (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager_->ChangeScene("GAME");
	}
}

void TitleScene::Draw() {
}

void TitleScene::Draw3D() {
}

void TitleScene::DrawSprite() {
}

void TitleScene::DrawBack() {
}