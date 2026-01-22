#include "MyGame.h"

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "ImGuiManager.h"

namespace TKM {
	void MyGame::Initialize() {

		//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
		TKM::Framework::Initialize(); //基底クラスの初期化処理

		// Initialize sceneManager_
		sceneManager_ = std::make_unique<TKM::SceneManager>();

		//シーンファクトリーの生成、マネージャにセット
		sceneFactory_ = std::make_unique<TKM::SceneFactory>(dxCommon_.get(), srvManager_.get());
		sceneManager_->SetSceneFactory(sceneFactory_.get());

		//最初のシーンを設定
		sceneManager_->SetNextScene(sceneFactory_->CreateScene("TITLE"));

		assert(dxCommon_.get() != nullptr && "DirectXCommon is nullptr in MyGame::Initialize");
		assert(srvManager_.get() != nullptr && "SrvManager is nullptr in MyGame::Initialize");

		// ImGuiManagerの初期化
		imguiManager_ = std::make_unique<TKM::ImGuiManager>();
		imguiManager_->Initialize(windowsAPI_.get(), dxCommon_.get());
		//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

	}

	void MyGame::Finalize() {
		////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
		////				解放
		////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

		// 終了処理
		imguiManager_->Finalize();

		//基底クラスの終了処理
		TKM::Framework::Finalize();

	}

	void MyGame::Update() {
		//---------------------------------------------------------

		//基底クラスの更新処理
		TKM::Framework::Update();

		// ** ImGui処理開始 **
		imguiManager_->Begin();

		//シーンマネージャーの更新
		sceneManager_->Update();

		// ウィンドウメッセージの処理
		if (windowsAPI_->ProcessMessage()) {
			SetEndRequest(true); // Frameworkの終了フラグを設定
		}

		// ** ImGui処理終了 **
		imguiManager_->End();

		viewport = dxCommon_->GetViewport();
		scissorRect = dxCommon_->GetRect();

		//---------------------------------------------------------
	}

	void MyGame::Draw() {
		// ① シーンを RenderTexture に描く
		dxCommon_->PreDraw();
		srvManager_->PreDraw();

		sceneManager_->Draw();      // ← GameScene が RenderTexture に描く

		// ② Swapchain に切り替え
		dxCommon_->BeginDrawToSwapchain();

		// ③ RenderTexture → Swapchain へコピー（RadialBlur を含めた「正攻法」）
		srvManager_->PreDraw();
		dxCommon_->DrawPostEffectToSwapchain(); // RenderTexture を使った後処理

		// ④ ImGui描画
		imguiManager_->Draw();

		// ⑤ フレーム終了
		dxCommon_->PostDraw();
	}
}