#include "MyGame.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <InputHandler.h>

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

	// 入力処理の初期化
	input = std::make_unique<Input>();
	input->Initialize(windowsAPI.get());

	// InputHandlerのコマンドを設定
	inputHandler.AssignMoveCommands();


	ModelManager::GetInstance()->Initialize(dxCommon.get());
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon.get());

	///--------------------------------------------

	object3d->Initialize(object3dCommon.get(), dxCommon.get());
	object3d->SetModel("axis.obj");

	anotherObject3d->Initialize(object3dCommon.get(), dxCommon.get());
	anotherObject3d->SetModel("plane.obj");

	//---------------------------------------------

	//Object3d共通部の初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-30.0f });
	//object3dCommon->SetDefaultCamera(camera.get());
	object3d->SetCamera(camera.get());
	anotherObject3d->SetCamera(camera.get());
	//ImGui用のcamera設定
	Vector3 cameraPosition = camera->GetTranslate();
	Vector3 cameraRotation = camera->GetRotate();

	imguiManager = std::make_unique<ImGuiManager>();
	imguiManager->Initialize(windowsAPI.get(), dxCommon.get());

	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	ParticleManager::GetInstance()->Initialize(dxCommon.get(), srvManager.get(), camera.get());
	ParticleManager::GetInstance()->CreateParticleGroup("particle", "resources/circle.png");

	particleEmitter = std::make_unique<ParticleEmitter>();
	particleEmitter->Initialize("particle");

	particleEmitter->Emit();
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

	sprite->ImGuiDebug();

	// ** ImGui処理終了 **
	imguiManager->End();

	////開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
	////ImGui::ShowDemoWindow();

	////ImGuiの内部コマンドを生成する
	//ImGui::Render();

	viewport = dxCommon->GetViewport();
	scissorRect = dxCommon->GetRect();

	input->Update();

	ICommand* command = inputHandler.HandleInput(input.get());
	if (command) {
		command->Exec(*object3d);  // コマンドを実行
	}

	camera->Update();
	sprite->Update();
	object3d->Update();
	//anotherObject3d->Update();

	particleEmitter->Update();
	ParticleManager::GetInstance()->Update();


	//object3d*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	////translate
	//Vector3 translate = object3d->GetTranslate();
	//translate = { 3.0f,-4.0f,0.0f };
	//object3d->SetTranslate(translate);
	//rotate
	Vector3 rotate = object3d->GetRotate();
	rotate += { 0.0f, 0.0f, 0.1f };
	object3d->SetRotate(rotate);
	//scale
	Vector3 scale = object3d->GetScale();
	scale = { 1.0f, 1.0f, 1.0f };
	object3d->SetScale(scale);

	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-


	//anotherObject3d-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
	//translate
	//Vector3 anotherTranslate = anotherObject3d->GetTranslate();
	//anotherTranslate = { 3.0f,4.0f,0.0f };
	//anotherObject3d->SetTranslate(anotherTranslate);
	////ratate
	//Vector3 anotherRotate = anotherObject3d->GetRotate();
	//anotherRotate += { 0.1f, 0.0f, 0.0f };
	//anotherObject3d->SetRotate(anotherRotate);
	////scale
	//Vector3 anotherScale = anotherObject3d->GetScale();
	//anotherScale = { 1.0f, 1.0f, 1.0f };
	//anotherObject3d->SetScale(anotherScale);

	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-


	//---------------------------------------------------------
}

void MyGame::Draw()
{
	dxCommon->PreDraw();
	srvManager->PreDraw();

	//描画
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	//Draw
	spriteCommon->DrawSetCommon();
	object3dCommon->DrawSetCommon();

	sprite->Draw();  // textureSrvHandleGPU は必要に応じて設定
	object3d->Draw(dxCommon.get());
	//anotherObject3d->Draw(dxCommon.get());

	ParticleManager::GetInstance()->Draw();

	// ** ImGui描画 **
	imguiManager->Draw();

	dxCommon->PostDraw();
}
