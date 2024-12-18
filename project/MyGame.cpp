#include "MyGame.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

void MyGame::Initialize()
{

	//基底クラスの初期化処理
	Framework::Initialize();

	//基盤システムの初期化*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	//WindowsAPIの初期化
	windowsAPI = std::make_unique<WindowsAPI>();
	windowsAPI->Initialize();

	audio = std::make_unique<AudioManager>();
	audio->Initialize();

	//入力の初期化
	input = std::make_unique<Input>();
	input->Initialize(windowsAPI.get());

	//DirectXの初期化
	dxCommon = std::make_unique<DirectXCommon>();
	dxCommon->Initialize(windowsAPI.get());
	//XAudioのエンジンのインスタンスを生成
	//audio->LoadSound("fanfare", "fanfare.wav");
	// 音声の再生
	//audio->PlaySound("fanfare");

	srvManager = std::make_unique<SrvManager>();
	srvManager->Initialize(dxCommon.get());

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon.get(), srvManager.get());
	//ファイルパス
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./resources/pokemon.png");

	//スプライト共通部の初期化
	spriteCommon = std::make_unique<SpriteCommon>();
	spriteCommon->Initialize(dxCommon.get());

	sprite = std::make_unique<Sprite>();
	sprite->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/uvChecker.png");

	//Object3d共通部の初期化
	object3dCommon = std::make_unique<Object3dCommon>();
	object3dCommon->Initialize(dxCommon.get());


	///--------------------------------------------

	/*object3d->Initialize(object3dCommon.get(), dxCommon.get());
	object3d->SetModel("axis.obj");

	anotherObject3d->Initialize(object3dCommon.get(), dxCommon.get());
	anotherObject3d->SetModel("plane.obj");*/

	ModelManager::GetInstance()->Initialize(dxCommon.get());
	//ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon.get());
	ModelManager::GetInstance()->LoadModel("SkyDome.obj", dxCommon.get());


	///--------------------------------------------

	//Object3d共通部の初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-30.0f });

	////object3dCommon->SetDefaultCamera(camera.get());
	//object3d->SetCamera(camera.get());
	//anotherObject3d->SetCamera(camera.get());
	////ImGui用のcamera設定
	//Vector3 cameraPosition = camera->GetTranslate();
	//Vector3 cameraRotation = camera->GetRotate();

	imguiManager = std::make_unique<ImGuiManager>();
	imguiManager->Initialize(windowsAPI.get(), dxCommon.get());

	skydome = std::make_unique<Skydome>();
	skydome->Initialize(object3dCommon.get(), dxCommon.get());
	//skydome->SetCamera(camera.get());

	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
}

void MyGame::Finalize()
{
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	////				解放
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

	// Object3dの解放
	delete object3d;

	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();

	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();

	// 終了処理
	audio->Finalize();

	// 終了処理
	imguiManager->Finalize();
	windowsAPI->Finalize();

	//基底クラスの終了処理
	Framework::Finalize();
}

void MyGame::Update()
{

	//基底クラスの更新処理
	Framework::Update();

	if (windowsAPI->ProcessMessage()) {
		endRequest_ = true;
	}

	// ** ImGui処理開始 **
	imguiManager->Begin();

	//sprite->ImGuiDebug();

	// ** ImGui処理終了 **
	imguiManager->End();

	camera->Update();
	// Skydomeの更新
	skydome->Update();
	//sprite->Update();
	//object3d->Update();
	//anotherObject3d->Update();

	viewport = dxCommon->GetViewport();
	scissorRect = dxCommon->GetRect();


	//---------------------------------------------------------
}

void MyGame::Draw()
{

	//Draw
	dxCommon->PreDraw();
	srvManager->PreDraw();

	spriteCommon->DrawSetCommon();
	object3dCommon->DrawSetCommon();

	skydome->Draw();

	//描画
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	//sprite->Draw();  // textureSrvHandleGPU は必要に応じて設定
	//object3d->Draw(dxCommon.get());
	//anotherObject3d->Draw(dxCommon.get());
	// Skydomeの描画

	// ** ImGui描画 **
	imguiManager->Draw();


	dxCommon->PostDraw();
}
