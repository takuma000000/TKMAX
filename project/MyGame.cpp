#include "MyGame.h"
#include "MyMath.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <imgui/imgui.h>

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

	////開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
	////ImGui::ShowDemoWindow();

	////ImGuiの内部コマンドを生成する
	//ImGui::Render();

	//camera->Update();
	//sprite->Update();
	//object3d->Update();
	//anotherObject3d->Update();

	//viewport = dxCommon->GetViewport();
	//scissorRect = dxCommon->GetRect();


	//object3d*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	//translate
	//Vector3 translate = object3d->GetTranslate();
	//translate = { 3.0f,-4.0f,0.0f };
	//object3d->SetTranslate(translate);
	////rotate
	//Vector3 rotate = object3d->GetRotate();
	//rotate += { 0.0f, 0.0f, 0.1f };
	//object3d->SetRotate(rotate);
	////scale
	//Vector3 scale = object3d->GetScale();
	//scale = { 1.0f, 1.0f, 1.0f };
	//object3d->SetScale(scale);

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

	Vector3 from0 = MyMath::Normalize(Vector3{ 1.0f,0.7f,0.5f });
	Vector3 to0 = from0 * -1.0f;
	Vector3 from1 = MyMath::Normalize(Vector3{ -0.6f,0.9f,0.2f });
	Vector3 to1 = MyMath::Normalize(Vector3{ 0.4f,0.7f,-0.5f });
	Matrix4x4 rotateMatrix0 = MyMath::DirectionToDirection(MyMath::Normalize(Vector3{ 1.0f,0.0f,0.0f }), MyMath::Normalize(Vector3{ -1.0f,0.0f,0.0f }));
	Matrix4x4 rotateMatrix1 = MyMath::DirectionToDirection(from0, to0);
	Matrix4x4 rotateMatrix2 = MyMath::DirectionToDirection(from1, to1);

	imguiManager->Begin();

	// ImGuiウィンドウで行列を表示
	if (ImGui::Begin("Matrix Display")) {
		// rotateMatrix0の表示（転置）
		ImGui::Text("rotateMatrix0 (transposed):");
		for (int y = 0; y < 4; ++y) {
			ImGui::Text("%f %f %f %f", rotateMatrix0.m[0][y], rotateMatrix0.m[1][y], rotateMatrix0.m[2][y], rotateMatrix0.m[3][y]);
		}

		// rotateMatrix1の表示（転置）
		ImGui::Text("rotateMatrix1 (transposed):");
		for (int y = 0; y < 4; ++y) {
			ImGui::Text("%f %f %f %f", rotateMatrix1.m[0][y], rotateMatrix1.m[1][y], rotateMatrix1.m[2][y], rotateMatrix1.m[3][y]);
		}

		// rotateMatrix2の表示（転置）
		ImGui::Text("rotateMatrix2 (transposed):");
		for (int y = 0; y < 4; ++y) {
			ImGui::Text("%f %f %f %f", rotateMatrix2.m[0][y], rotateMatrix2.m[1][y], rotateMatrix2.m[2][y], rotateMatrix2.m[3][y]);
		}

		ImGui::End();
	}

	imguiManager->End();


	//---------------------------------------------------------
}

void MyGame::Draw()
{
	//Draw
	dxCommon->PreDraw();

	//描画
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	//sprite->Draw();  // textureSrvHandleGPU は必要に応じて設定
	//object3d->Draw(dxCommon.get());
	//anotherObject3d->Draw(dxCommon.get());

	spriteCommon->DrawSetCommon();
	object3dCommon->DrawSetCommon();

	// ** ImGui描画 **
	imguiManager->Draw();

	srvManager->PreDraw();

	dxCommon->PostDraw();
}
