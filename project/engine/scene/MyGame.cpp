#include "MyGame.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "ImGuiManager.h"

void MyGame::Initialize()
{

	Framework::Initialize();

	// Initialize sceneManager_
	sceneManager_ = std::make_unique<SceneManager>();

	//最初のシーンを設定
	//BaseScene* scene = new TitleScene(dxCommon.get(),srvManager.get());
	//sceneManager_->SetNextScene(scene);

	//シーンファクトリーの生成、マネージャにセット
	sceneFactory_ = std::make_unique<SceneFactory>(dxCommon.get(), srvManager.get());
	sceneManager_->SetSceneFactory(sceneFactory_.get());

	//最初のシーンを設定
	sceneManager_->SetNextScene(sceneFactory_->CreateScene("TITLE"));

	assert(dxCommon.get() != nullptr && "DirectXCommon is nullptr in MyGame::Initialize");
	assert(srvManager.get() != nullptr && "SrvManager is nullptr in MyGame::Initialize");

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

	//基底クラスの更新処理
	Framework::Update();

	// ** ImGui処理開始 **
	imguiManager->Begin();

	sceneManager_->Update();

	// ウィンドウメッセージの処理
	if (windowsAPI->ProcessMessage()) {
		SetEndRequest(true); // Frameworkの終了フラグを設定
	}

	// ** ImGui処理終了 **
	imguiManager->End();

	////開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
	////ImGui::ShowDemoWindow();

	////ImGuiの内部コマンドを生成する
	//ImGui::Render();

	viewport = dxCommon->GetViewport();
	scissorRect = dxCommon->GetRect();



	//---------------------------------------------------------
}

void MyGame::Draw()
{
	dxCommon->PreDraw();
	srvManager->PreDraw();

	sceneManager_->Draw();

	//描画
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	// ** ImGui描画 **
	imguiManager->Draw();

	dxCommon->PostDraw();
}
