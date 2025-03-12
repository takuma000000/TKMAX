#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "GameScene.h"

#include "Input.h"

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

	Input::GetInstance()->Update();

	//ENTERキーが押されたら
	 // ENTERキーが押されたら GameScene に遷移
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) // DirectInput のキーコードを使用
	{
		// 次のシーンを生成
		BaseScene* nextScene = new GameScene(dxCommon,srvManager);

		// シーン切り替えを依頼
		sceneManager_->SetNextScene(nextScene);
	}

}

void TitleScene::Draw()
{
	SpriteCommon::GetInstance()->DrawSetCommon();
	sprite->Draw();
}
