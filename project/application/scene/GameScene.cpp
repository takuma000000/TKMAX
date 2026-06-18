#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include <algorithm>

void GameScene::Initialize() {
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/circle2.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/goal.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/magic_attack.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/typeC_Bullet.png");
}

void GameScene::Finalize() {

}

void GameScene::Update() {
	TKM::Input::GetInstance()->Update();
}

void GameScene::Draw() {
	DrawSprite();
	Draw3D();
}

void GameScene::Draw3D() {}

void GameScene::DrawSprite() {
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
}

void GameScene::DrawBack() {
}