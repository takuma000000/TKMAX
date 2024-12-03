#include "MyMath.h"
#include "Vector3.h"

#include "MyGame.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	MyGame game;
	//ゲームの初期化
	game.Initialize();

	MSG msg{};
	//ウィンドウの×ボタンが押されるまでループ
	while (true) {

		//Windowsのメッセージ処理
		if (game.IsEndRequest()) {
			//ゲームループを抜ける
			break;
		} else {

			//毎フレーム更新
			game.Update();

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