#include <string>
#include <format>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <cstdint>
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <sstream>
#include <imgui/imgui.h>
#include "externals/DirectXTex/DirectXTex.h"
#include <xaudio2.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"xaudio2.lib")

#include "MyMath.h"
#include "Vector3.h"

// GE3クラス化(MyClass)
#include "WindowsAPI.h"
#include "Input.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "engine/2d/Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Camera.h"
#include "SrvManager.h"
#include "ImGuiManager.h"
#include "AudioManager.h"


//OutputDebugStringA関数
void Log(const std::string& message) {
	OutputDebugStringA(message.c_str());
}

//Transform変数を作る
Transform transform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
//Transform cameraTransform{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f} ,{0.0f,0.0f,-50.0f} };
//Transform transformSprite{ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
Transform uvTransformSprite{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

//SRV切り替え
bool useMonsterBall = true;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	//基盤システムの初期化*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
	
	//ポインタ...WindowsAPI
	std::unique_ptr<WindowsAPI> windowsAPI = nullptr;
	//WindowsAPIの初期化
	windowsAPI = std::make_unique<WindowsAPI>();
	windowsAPI->Initialize();

	//Audio
	std::unique_ptr<AudioManager> audio = nullptr;
	audio = std::make_unique<AudioManager>();
	audio->Initialize();

	//ポインタ...Input
	std::unique_ptr<Input> input = nullptr;
	//入力の初期化
	input = std::make_unique<Input>();
	input->Initialize(windowsAPI.get());

	//ポインタ...DirectXCommon
	std::unique_ptr<DirectXCommon> dxCommon = nullptr;
	//DirectXの初期化
	dxCommon = std::make_unique<DirectXCommon>();
	dxCommon->Initialize(windowsAPI.get());
	//XAudioのエンジンのインスタンスを生成
	audio->LoadSound("fanfare", "fanfare.wav");
	// 音声の再生
	audio->PlaySound("fanfare");

	//ポインタ...srvManager
	std::unique_ptr<SrvManager> srvManager = nullptr;
	srvManager = std::make_unique<SrvManager>();
	srvManager->Initialize(dxCommon.get());

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon.get(),srvManager.get());
	//ファイルパス
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./resources/pokemon.png");

	//ポインタ...SpriteCommon
	std::unique_ptr<SpriteCommon> spriteCommon = nullptr;
	//スプライト共通部の初期化
	spriteCommon = std::make_unique<SpriteCommon>();
	spriteCommon->Initialize(dxCommon.get());

	//ポインタ...Sprite
	std::unique_ptr<Sprite> sprite = nullptr;
	sprite = std::make_unique<Sprite>();
	sprite->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/uvChecker.png");

	//ポインタ...Object3dCommon
	std::unique_ptr<Object3dCommon> object3dCommon = nullptr;
	//Object3d共通部の初期化
	object3dCommon = std::make_unique<Object3dCommon>();
	object3dCommon->Initialize(dxCommon.get());


	ModelManager::GetInstance()->Initialize(dxCommon.get());
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon.get());

	///--------------------------------------------

	//Object3dの初期化
	Object3d* object3d = new Object3d();
	object3d->Initialize(object3dCommon.get(), dxCommon.get());
	object3d->SetModel("axis.obj");

	//AnotherObject3d ( もう一つのObject3d )
	Object3d* anotherObject3d = new Object3d();
	anotherObject3d->Initialize(object3dCommon.get(), dxCommon.get());
	anotherObject3d->SetModel("plane.obj");

	//---------------------------------------------

	//ポインタ...Camera
	std::unique_ptr<Camera> camera = nullptr;
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

	//ポインタ...ImGuiManager
	std::unique_ptr<ImGuiManager>  imguiManager = nullptr;
	imguiManager = std::make_unique<ImGuiManager>();
	imguiManager->Initialize(windowsAPI.get(),dxCommon.get());

	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	MSG msg{};
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {

		//Windowsのメッセージ処理
		if (windowsAPI->ProcessMessage()) {
			//ゲームループを抜ける
			break;
		} else {


			// ** ImGui処理開始 **
			imguiManager->Begin();

			sprite->ImGuiDebug();

			 // ** ImGui処理終了 **
			imguiManager->End();

			////開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
			////ImGui::ShowDemoWindow();

			////ImGuiの内部コマンドを生成する
			//ImGui::Render();

			D3D12_VIEWPORT viewport = dxCommon->GetViewport();
			D3D12_RECT scissorRect = dxCommon->GetRect();

			//Draw
			dxCommon->PreDraw();
			spriteCommon->DrawSetCommon();
			object3dCommon->DrawSetCommon();
			srvManager->PreDraw();

			//---------------------------------------------------------

			////現在の座標を変数で受け取る
			//Sprite::Vector2 position = sprite->GetPosition();
			////座標を変更する
			//position.x += 0.1f;
			//position.y += 0.1f;
			////変更を反映する
			//sprite->SetPosition(position);

			////角度を変化させる
			//float rotation = sprite->GetRotation();
			//rotation += 0.01f;
			//sprite->SetRotation(rotation);

			////色を変化させる
			//Sprite::Vector4 color = sprite->GetColor();
			//color.x += 0.01f;
			//if (color.x > 1.0f) {
			//	color.x -= 1.0f;
			//}
			//sprite->SetColor(color);

			//サイズを変化させる
			/*Sprite::Vector2 size = sprite->GetSize();
			size.x += 0.1f;
			size.y += 0.1f;
			sprite->SetSize(size);*/

			//Sprite::Vector2 position = sprite->GetPosition();
			////座標を変更する
			//position.x += 0.1f;
			//position.y += 0.1f;
			////変更を反映する
			//sprite->SetPosition(position);

			//object3d*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

			//translate
			Vector3 translate = object3d->GetTranslate();
			translate = { 3.0f,-4.0f,0.0f };
			object3d->SetTranslate(translate);
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
			Vector3 anotherTranslate = anotherObject3d->GetTranslate();
			anotherTranslate = { 3.0f,4.0f,0.0f };
			anotherObject3d->SetTranslate(anotherTranslate);
			//ratate
			Vector3 anotherRotate = anotherObject3d->GetRotate();
			anotherRotate += { 0.1f, 0.0f, 0.0f };
			anotherObject3d->SetRotate(anotherRotate);
			//scale
			Vector3 anotherScale = anotherObject3d->GetScale();
			anotherScale = { 1.0f, 1.0f, 1.0f };
			anotherObject3d->SetScale(anotherScale);

			//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-


			//---------------------------------------------------------

			//描画
			dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
			dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);

			camera->Update();
			sprite->Update();
			object3d->Update();
			anotherObject3d->Update();

			sprite->Draw();  // textureSrvHandleGPU は必要に応じて設定
			object3d->Draw(dxCommon.get());
			anotherObject3d->Draw(dxCommon.get());

			// ** ImGui描画 **
			imguiManager->Draw();

			dxCommon->PostDraw();

		}

	}//ゲームループ終わり




	//ImGuiの終了処理
	//ImGui_ImplDX12_Shutdown();
	//ImGui_ImplWin32_Shutdown();
	//ImGui::DestroyContext();

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


	return 0;

}