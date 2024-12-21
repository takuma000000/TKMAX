#include "Framework.h"

#include "WindowsAPI.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Input.h"
#include <engine/audio/AudioManager.h>
#include <TextureManager.h>
#include <ModelManager.h>
#include <Object3dCommon.h>
#include <SpriteCommon.h>

void Framework::Initialize()
{
	//シーンマネージャの生成
	sceneManager_ = new SceneManager();


	windowsAPI = std::make_unique<WindowsAPI>();
	windowsAPI->Initialize();

	// DirectXCommon の初期化
	dxCommon = std::make_unique<DirectXCommon>();
	dxCommon->Initialize(windowsAPI.get());

	srvManager = std::make_unique<SrvManager>();
	srvManager->Initialize(dxCommon.get());
	assert(srvManager != nullptr && "SrvManager initialization failed");


	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon.get(), srvManager.get());

	ModelManager::GetInstance()->Initialize(dxCommon.get());

	AudioManager::GetInstance()->Initialize(); // AudioManagerを初期化

	Object3dCommon::GetInstance()->Initialize(dxCommon.get()); // Object3dCommonを初期化
	SpriteCommon::GetInstance()->Initialize(dxCommon.get()); // SpriteCommonを初期化

	Input::GetInstance()->Initialize(windowsAPI.get()); // Inputを初期化

}

void Framework::Finalize()
{
	//シーンマネージャーの解放
	delete sceneManager_;

	windowsAPI->Finalize();

	Input::GetInstance()->Finalize();
}

void Framework::Update()
{
	//シーンマネージャーの更新
	sceneManager_->Update();
}

void Framework::Draw()
{
	//シーンマネージャーの描画
	sceneManager_->Draw();
}

void Framework::Run()
{
	//ゲームの初期化
	Initialize();

	//ウィンドウの×ボタンが押されるまでループ
	while (true) {//ゲームループ
		//毎フレーム更新
		Update();
		//終了リクエストが来たら抜ける
		if (IsEndRequest()) {
			//ゲームループを抜ける
			break;
		}
		//描画
		Draw();
	}//ゲームループ終わり
	//ゲームの終了
	Finalize();
}
