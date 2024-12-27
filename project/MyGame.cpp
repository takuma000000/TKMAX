#include "MyGame.h"

#include "Enemy.h" // 敵クラスのインクルード

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
	//スプライトの初期化
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
	ModelManager::GetInstance()->LoadModel("player.obj", dxCommon.get());
	ModelManager::GetInstance()->LoadModel("enemy.obj", dxCommon.get());
	ModelManager::GetInstance()->LoadModel("bullet.obj", dxCommon.get());

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

	//Skydomeの初期化
	skydome = std::make_unique<Skydome>();
	skydome->Initialize(object3dCommon.get(), dxCommon.get());
	skydome->SetCamera(camera.get());

	//Playerの初期化
	player = std::make_unique<Player>();
	player->Initialize(object3dCommon.get(), dxCommon.get(), camera.get(), input.get(),spriteCommon.get());
	player->SetCamera(camera.get());
	// PlayerにSprite共通部を設定
	player->SetSpriteCommon(spriteCommon.get());

	// 敵の生成
	std::vector<Vector3> enemyPositions = {
		{20.0f, 0.0f, 50.0f},
		{0.0f, 8.0f, 50.0f},
		{-20.0f, 0.0f, 50.0f},
		{0.0f, -8.0f, 50.0f},
		{20.0f, 8.0f, 50.0f},
		{-20.0f, 8.0f, 50.0f},
		{20.0f, -8.0f, 50.0f},
		{-20.0f, -8.0f, 50.0f},
	};

	for (const auto& position : enemyPositions) {
		Enemy* enemy = new Enemy();
		enemy->Initialize(object3dCommon.get(), dxCommon.get(), camera.get(), position);
		enemy->SetCamera(camera.get()); // カメラを設定
		enemies.push_back(enemy);
	}

	//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
}

void MyGame::Finalize()
{
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	////				解放
	////*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

	for (Enemy* enemy : enemies) {
		delete enemy;
	}
	enemies.clear();

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

	// ** ImGui描画 **
	imguiManager->Begin(); // ImGuiの描画開始

	player->DrawImGui(); // PlayerクラスのImGui描画

	imguiManager->End(); // ImGuiの描画終了

	//入力の更新
	input->Update();


	switch (currentPhase_) {
	case GamePhase::Title:
		if (input->PushKey(DIK_SPACE)) { // スペースキーで次のフェーズへ
			currentPhase_ = GamePhase::Explanation;
		}
		break;

	case GamePhase::Explanation:
		if (input->PushKey(DIK_RETURN)) { // Enterキーで次のフェーズへ
			currentPhase_ = GamePhase::GameScene;
		}
		break;

	case GamePhase::GameScene:
		camera->Update();
		skydome->Update();
		player->Update();

		// 敵の更新と当たり判定
		for (auto it = enemies.begin(); it != enemies.end(); ) {
			Enemy* enemy = *it;

			if (enemy->IsDead()) {
				delete enemy;
				it = enemies.erase(it);
				continue;
			}

			// 弾との衝突判定
			for (const auto& bullet : player->GetBullets()) {
				float distance = MyMath::Distance(enemy->GetPosition(), bullet->GetPosition());
				if (distance < 1.0f) { // 衝突範囲の閾値
					enemy->OnCollision(); // 敵を無効化
					bullet->Deactivate(); // 弾を無効化
					break;
				}
			}

			enemy->Update();
			++it;
		}

		// 敵が全滅したらクリアフェーズへ
		if (enemies.empty()) {
			currentPhase_ = GamePhase::Clear;
		}
		break;

	case GamePhase::Clear:
		if (input->TriggerKey(DIK_RETURN)) { // Enterキーでタイトルフェーズに戻る
			ResetGame(); // 状態をリセット
			currentPhase_ = GamePhase::Title;
		}
		break;
	}

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
	switch (currentPhase_) {
	case GamePhase::Title:
		
		break;

	case GamePhase::Explanation:
		
		break;

	case GamePhase::GameScene:
		skydome->Draw();
		player->Draw();
		for (const auto& enemy : enemies) {
			enemy->Draw();
		}
		break;

	case GamePhase::Clear:
		
		break;
	}

	//描画
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);

	//sprite->Draw();  // textureSrvHandleGPU は必要に応じて設定
	//object3d->Draw(dxCommon.get());
	//anotherObject3d->Draw(dxCommon.get());
	// Skydomeの描画

	imguiManager->Draw();


	dxCommon->PostDraw();
}

void MyGame::ResetGame()
{
	// 敵を全削除
	for (Enemy* enemy : enemies) {
		delete enemy;
	}
	enemies.clear();

	// 敵を再生成
	std::vector<Vector3> enemyPositions = {
		{20.0f, 0.0f, 50.0f},
		{0.0f, 8.0f, 50.0f},
		{-20.0f, 0.0f, 50.0f},
		{0.0f, -8.0f, 50.0f},
		{20.0f, 8.0f, 50.0f},
		{-20.0f, 8.0f, 50.0f},
		{20.0f, -8.0f, 50.0f},
		{-20.0f, -8.0f, 50.0f},
	};

	for (const auto& position : enemyPositions) {
		Enemy* enemy = new Enemy();
		enemy->Initialize(object3dCommon.get(), dxCommon.get(), camera.get(), position);
		enemy->SetCamera(camera.get());
		enemies.push_back(enemy);
	}

	// プレイヤーの初期位置リセット
	player->SetTranslate({ 0.0f, 0.0f, 0.0f });
}
