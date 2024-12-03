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