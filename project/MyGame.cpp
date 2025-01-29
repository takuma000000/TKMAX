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
	TextureManager::GetInstance()->LoadTexture("./resources/crushring.png");
	TextureManager::GetInstance()->LoadTexture("./resources/clear_ring.png");
	TextureManager::GetInstance()->LoadTexture("./resources/over_ring.png");
	TextureManager::GetInstance()->LoadTexture("./resources/setumei.png");
	TextureManager::GetInstance()->LoadTexture("./resources/bullet.png");
	TextureManager::GetInstance()->LoadTexture("./resources/enemyBullet.png");
	TextureManager::GetInstance()->LoadTexture("./resources/heart_CR.png");

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
	ModelManager::GetInstance()->LoadModel("enemyBullet.obj", dxCommon.get());

	///--------------------------------------------

	//Object3d共通部の初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-30.0f });

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

	// Title の初期化
	title = new Title();
	title->Initialize(spriteCommon.get(), dxCommon.get());

	// CLEARスプライトの初期化
	clearSprite_ = std::make_unique<Sprite>();
	clearSprite_->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/clear_ring.png");
	clearSprite_->SetPosition({ 0.0f, 0.0f });

	// OVERスプライトの初期化
	overSprite_ = std::make_unique<Sprite>();
	overSprite_->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/over_ring.png");
	overSprite_->SetPosition({ 0.0f, 0.0f });

	// 説明画面のスプライトの初期化
	explanationSprite_ = std::make_unique<Sprite>();
	explanationSprite_->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/setumei.png");
	explanationSprite_->SetPosition({ 0.0f, 0.0f });

	// 15 発分のスプライトを作成
	for (int i = 0; i < 15; i++) {
		auto sprite = std::make_unique<Sprite>();
		sprite->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/bullet.png");
		sprite->SetPosition({ 20.0f + i * 30.0f, 20.0f }); // 左上から横に並べる
		sprite->SetSize({ 0.5f, 0.5f }); // サイズ調整
		bulletSprites_.push_back(std::move(sprite));
	}


	// ウィンドウサイズを取得
	int screenWidth = windowsAPI->GetWindowWidth();
	int screenHeight = windowsAPI->GetWindowHeight();

	// HP表示用スプライトの作成（画面右下に配置）
	for (int i = 0; i < 3; i++) {
		auto heartSprite = std::make_unique<Sprite>();
		heartSprite->Initialize(spriteCommon.get(), dxCommon.get(), "./resources/heart_CR.png");

		// 画面右下に並べる
		float xPos = screenWidth - 50.0f - i * 50.0f;
		float yPos = screenHeight - 50.0f;
		heartSprite->SetPosition({ xPos, yPos });

		heartSprites_.push_back(std::move(heartSprite));
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

	// Title の解放
	if (title) {
		delete title;
		title = nullptr;
	}

	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();

	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();

	// 終了処理
	audio->Finalize();

	// 終了処理
	//imguiManager->Finalize();
	windowsAPI->Finalize();

	//基底クラスの終了処理
	Framework::Finalize();
}

void MyGame::Update() {
	// 基底クラスの更新処理
	Framework::Update();

	if (windowsAPI->ProcessMessage()) {
		endRequest_ = true;
	}

	input->Update();

	switch (currentPhase_) {
	case GamePhase::Title:
		if (input->TriggerKey(DIK_SPACE)) {
			currentPhase_ = GamePhase::Explanation;
			input->Update(); // キー入力をリセット
		}
		title->Update(); // タイトル画面の更新処理
		break;

	case GamePhase::Explanation:
		if (input->TriggerKey(DIK_SPACE)) {
			currentPhase_ = GamePhase::GameScene;
			input->Update(); // キー入力をリセット
		}
		explanationSprite_->Update(); // 説明画面の更新
		break;

	case GamePhase::GameScene:
		camera->Update();
		skydome->Update();

		if (!player->IsDead()) {
			player->Update(enemies); // HPが0なら更新しない
		}

		playerHP = player->GetHP();

		for (int i = 0; i < heartSprites_.size(); i++) {
			heartSprites_[i]->SetVisible(i < playerHP); // HPの数だけ表示
			heartSprites_[i]->Update();
		}

		// 敵の更新
		for (auto it = enemies.begin(); it != enemies.end();) {
			Enemy* enemy = *it;
			if (enemy->IsDead()) {
				delete enemy;
				it = enemies.erase(it);
				continue;
			}

			enemy->Update(player.get()); // 敵がプレイヤーをターゲットにして弾を発射
			++it;
		}

		// 敵の弾とプレイヤーの衝突判定
		for (const auto& enemy : enemies) {
			for (const auto& bullet : enemy->GetBullets()) {
				float distance = MyMath::Distance(player->GetTranslate(), bullet->GetPosition());
				float collisionThreshold = 1.0f; // 衝突判定の距離

				// **プレイヤーが無敵状態なら衝突を無視**
				if (distance < collisionThreshold && !player->IsInvincible()) {
					player->OnCollision(); // プレイヤーの被弾処理
					bullet->Deactivate();  // 弾を無効化
					break;
				}
			}
		}

		// プレイヤーの弾と敵の衝突判定
		for (auto it = enemies.begin(); it != enemies.end();) {
			Enemy* enemy = *it;
			for (const auto& bullet : player->GetBullets()) {
				float enemySize = (enemy->GetScale().x + enemy->GetScale().y + enemy->GetScale().z) / 3.0f;
				float collisionThreshold = enemySize * 0.5f;

				float distance = MyMath::Distance(enemy->GetPosition(), bullet->GetPosition());
				if (distance < collisionThreshold) {
					enemy->OnCollision(); // 敵の被弾処理
					bullet->Deactivate();
					break;
				}
			}

			if (enemy->IsDead()) {
				delete enemy;
				it = enemies.erase(it);
			} else {
				++it;
			}
		}

		// 弾のスプライト更新
		if (currentPhase_ == GamePhase::GameScene) {
			int bulletCount = player->GetBulletCount();
			for (int i = 0; i < bulletCount; i++) {
				bulletSprites_[i]->Update();
			}
		}

		// 敵が全滅したらクリアフェーズへ
		if (enemies.empty()) {
			currentPhase_ = GamePhase::Clear;
		}

		// **HPが0のときにカウントダウンが進むようにする**
		if (player->GetHP() <= 0 && player->IsHPOverTimerRunning()) {
			player->Update(enemies); // HPが0でも `hpOverTimer_` を減少させる
		}

		if ((player->GetHP() <= 0 && player->IsHPOverTimerExpired()) ||
			(player->GetBulletCount() <= 0 && player->IsOverTimerExpired())) {
			currentPhase_ = GamePhase::Over;
		}

		break;

	case GamePhase::Clear:
		clearSprite_->Update(); // クリア画面の更新

		if (input->TriggerKey(DIK_SPACE)) { // スペースキーでタイトルフェーズに戻る
			ResetGame();
			currentPhase_ = GamePhase::Title;
			input->Update(); // キー入力をリセット
		}
		break;

	case GamePhase::Over:
		overSprite_->Update(); // ゲームオーバー画面の更新

		if (input->TriggerKey(DIK_SPACE)) { // スペースキーでタイトルフェーズに戻る
			ResetGame();
			currentPhase_ = GamePhase::Title;
			input->Update(); // キー入力をリセット
		}
		break;
	}

	viewport = dxCommon->GetViewport();
	scissorRect = dxCommon->GetRect();
}


void MyGame::Draw()
{
	// 描画開始
	dxCommon->PreDraw();
	srvManager->PreDraw();

	spriteCommon->DrawSetCommon();
	object3dCommon->DrawSetCommon();

	// 🎯 `bulletCount` を switch の外で宣言
	int bulletCount = player->GetBulletCount();

	switch (currentPhase_) {
	case GamePhase::Title:
		title->Draw(); // Title の描画処理
		break;

	case GamePhase::Explanation:
		explanationSprite_->Draw(); // 説明画面のスプライトを描画
		break;

	case GamePhase::GameScene:
		skydome->Draw();
		if (!player->IsDead()) {
			player->Draw(); // HPが0なら描画しない
		}
		for (const auto& enemy : enemies) {
			enemy->Draw();
		}

		// HPスプライトの描画
		if (currentPhase_ == GamePhase::GameScene) {
			for (const auto& heart : heartSprites_) {
				heart->Draw();
			}
		}

		// 🔥 **残弾表示**
		for (int i = 0; i < bulletCount; i++) {
			bulletSprites_[i]->Draw();
		}
		break;

	case GamePhase::Clear:
		clearSprite_->Draw(); // CLEAR画面のスプライトを描画
		break;

	case GamePhase::Over:
		overSprite_->Draw(); // OVER画面のスプライトを描画
		break;
	}

	// 描画終了
	dxCommon->GetCommandList()->RSSetViewports(1, &viewport);
	dxCommon->GetCommandList()->RSSetScissorRects(1, &scissorRect);
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

	// **弾のリセット**
	player->ResetBulletCount();

	// **HPのリセット**
	player->ResetHP();
}
