#include "Framework.h"

#include "WindowsAPI.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Input.h"
#include <AudioManager.h>
#include <TextureManager.h>
#include <ModelManager.h>
#include <Object3dCommon.h>
#include <SpriteCommon.h>

using TKM::TextureManager;
using TKM::ModelManager;
using TKM::Object3dCommon;
using TKM::AudioManager;

void Framework::Initialize() {
	//シーンマネージャの生成
	sceneManager_ = std::make_unique<SceneManager>();

	// WindowsAPI の初期化
	windowsAPI = std::make_unique<WindowsAPI>();
	windowsAPI->Initialize();

	// DirectXCommon の初期化
	dxCommon = std::make_unique<TKM::DirectXCommon>();
	dxCommon->Initialize(windowsAPI.get());

	// SRVマネージャの初期化
	srvManager = std::make_unique<SrvManager>();
	srvManager->Initialize(dxCommon.get());
	assert(srvManager != nullptr && "SrvManager initialization failed");

	// DirectXCommon に SrvManager を教える
	dxCommon->SetSrvManager(srvManager.get());

	// RenderTexture 用の RTV/SRV を作成（ここで rtvHandles[2] が有効になる）
	dxCommon->CreateRenderTextureRTV();

	// RenderTexture 用の RTV/SRV を作成
	dxCommon->CreateRenderTextureRTV();

	// CopyImage 用パイプラインを初期化
	dxCommon->InitializeCopyImagePipeline();

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon.get(), srvManager.get());
	// モデルマネージャの初期化
	ModelManager::GetInstance()->Initialize(dxCommon.get());
	// オーディオマネージャの初期化
	AudioManager::GetInstance()->Initialize(); // AudioManagerを初期化

	Object3dCommon::GetInstance()->Initialize(dxCommon.get()); // Object3dCommonを初期化
	TKM::SpriteCommon::GetInstance()->Initialize(dxCommon.get());   // SpriteCommonを初期化
	Input::GetInstance()->Initialize(windowsAPI.get());        // Inputを初期化
}

void Framework::Finalize() {
	windowsAPI->Finalize(); // WindowsAPI の終了

	Input::GetInstance()->Finalize(); // Inputの終了
}

void Framework::Update() {
	//シーンマネージャーの更新
	sceneManager_->Update();
}

void Framework::Draw() {
	//シーンマネージャーの描画
	sceneManager_->Draw();
}

void Framework::Run() {
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
