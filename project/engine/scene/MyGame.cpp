#include "MyGame.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "ImGuiManager.h"

void MyGame::Initialize()
{

	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
	Framework::Initialize(); //基底クラスの初期化処理

	// Initialize sceneManager_
	sceneManager_ = std::make_unique<SceneManager>();

	//シーンファクトリーの生成、マネージャにセット
	sceneFactory_ = std::make_unique<SceneFactory>(dxCommon.get(), srvManager.get());
	sceneManager_->SetSceneFactory(sceneFactory_.get());

	//最初のシーンを設定
	sceneManager_->SetNextScene(sceneFactory_->CreateScene("TITLE"));

	assert(dxCommon.get() != nullptr && "DirectXCommon is nullptr in MyGame::Initialize");
	assert(srvManager.get() != nullptr && "SrvManager is nullptr in MyGame::Initialize");

	// ImGuiManagerの初期化
	imguiManager = std::make_unique<ImGuiManager>();
	imguiManager->Initialize(windowsAPI.get(), dxCommon.get());
	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

}

void MyGame::Finalize()
{
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	////				解放
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

	// 終了処理
	imguiManager->Finalize();

	//基底クラスの終了処理
	Framework::Finalize();

}

void MyGame::Update()
{
	//---------------------------------------------------------

	//基底クラスの更新処理
	Framework::Update();

	// ** ImGui処理開始 **
	imguiManager->Begin();

	//シーンマネージャーの更新
	sceneManager_->Update();

	// ウィンドウメッセージの処理
	if (windowsAPI->ProcessMessage()) {
		SetEndRequest(true); // Frameworkの終了フラグを設定
	}

	// ** ImGui処理終了 **
	imguiManager->End();

	viewport = dxCommon->GetViewport();
	scissorRect = dxCommon->GetRect();

	//---------------------------------------------------------
}

void MyGame::Draw()
{
	dxCommon->PreDraw(); //描画前処理
	srvManager->PreDraw(); //SRVデスクリプタヒープセット

	sceneManager_->Draw(); //シーンマネージャーの描画

	//描画
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport); // ビューポートの設定
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);	// シザー矩形の設定

	// ** ImGui描画 **
	imguiManager->Draw();

	dxCommon->PostDraw(); //描画後処理
}
