#include "MyGame.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "ImGuiManager.h"

void MyGame::Initialize()
{

	Framework::Initialize();

	// Initialize sceneManager_
	sceneManager_ = new SceneManager();

	//最初のシーンを設定
	BaseScene* scene = new TitleScene(dxCommon.get(), srvManager.get());
	sceneManager_->SetNextScene(scene);

	assert(dxCommon.get() != nullptr && "DirectXCommon is nullptr in MyGame::Initialize");
	assert(srvManager.get() != nullptr && "SrvManager is nullptr in MyGame::Initialize");

	scene_ = new TitleScene(dxCommon.get(), srvManager.get());
	scene_->Initialize();

	imguiManager = std::make_unique<ImGuiManager>();
	imguiManager->Initialize(windowsAPI.get(), dxCommon.get());



	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

}

void MyGame::Finalize()
{
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	////				解放
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

	scene_->Finalize();
	delete scene_;
	delete sceneManager_;

	// 終了処理
	imguiManager->Finalize();

	//基底クラスの終了処理
	Framework::Finalize();

}

void MyGame::Update()
{

	//基底クラスの更新処理
	Framework::Update();

	scene_->Update();

	if (windowsAPI->ProcessMessage()) {
		endRequest_ = true;
	}

	// ** ImGui処理開始 **
	imguiManager->Begin();

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

	scene_->Draw();

	//描画
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	// ** ImGui描画 **
	imguiManager->Draw();

	dxCommon->PostDraw();
}
