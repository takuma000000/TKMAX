#include "TitleScene.h"

void TitleScene::Initialize()
{
	TextureManager::GetInstance()->LoadTexture("./resources/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.png");

	sprite = std::make_unique<Sprite>();
	sprite->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/circle.png");
}

void TitleScene::Finalize()
{
	TextureManager::GetInstance()->Finalize();
}

void TitleScene::Update()
{
	sprite->Update();
}

void TitleScene::Draw()
{
	SpriteCommon::GetInstance()->DrawSetCommon();
	sprite->Draw();
}
