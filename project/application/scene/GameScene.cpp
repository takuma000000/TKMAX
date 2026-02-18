#define NOMINMAX
#include "GameScene.h"
#include <limits>
#include <algorithm>
#include <psapi.h>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using namespace TKM;

void GameScene::Initialize() {
	/// ──────────────── NULLチェック ────────────────
	assert(this != nullptr && "this is nullptr in GameScene::Initialize");
	assert(dxCommon_ != nullptr && "dxCommon is nullptr in GameScene::Initialize");
	/// ──────────────── 各種初期化処理 ───────────────
	InitializeAudio();   // サウンドのロード＆再生
	LoadTextures();      // テクスチャのロード
	InitializeSprite();  // スプライトの作成＆初期化
	LoadModels();        // 3Dモデルのロード
	InitializeObjects(); // 3Dオブジェクトの作成＆初期化
	InitializeCamera();  // カメラの作成＆設定
	/// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize({ 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);
	/// ──────────────── ラインレンダラーの初期化 ───────────────
	LineRenderer::GetInstance()->Initialize(dxCommon_);
	/// ──────────────── パーティクルの初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());
	// パーティクルグループの登録は ParticleGroupsCatalogクラス へ
	ParticleGroupsCatalog::RegisterScene(ParticleManager::GetInstance());
	// パーティクルエミッターの初期化
	particleEmitter_ = std::make_unique<ParticleEmitter>();
	particleEmitter_->Initialize("uv", { 0.0f,2.5f,10.0f });
	/// ──────────────── スカイボックスの初期化 ───────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());
	/// ──────────────── 敵マネージャの初期化 ───────────────
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize(dxCommon_, camera_.get(), this, player_.get());
	/// ──────────────── ボスマネージャの初期化 ───────────────
	if (!bossManager_) {
		bossManager_ = std::make_unique<BossManager>();
	}
	bossManager_->Initialize(dxCommon_, camera_.get(), this, player_.get());
	/// ──────────────── タイムスケールコントローラーの初期化 ───────────────
	timeScale_.Initialize();
	bossManager_->SetTimeScaleController(&timeScale_);
	/// ──────────────── 花火コントローラーの初期化 ───────────────
	fireworkController_ = std::make_unique<TKM::FireworkController>();
	fireworkController_->Reset();
	/// ──────────────── ポストエフェクトの初期化 ───────────────
	postFx_ = std::make_unique<TKM::PostEffectController>();
	postFx_->Initialize(dxCommon_, player_.get(), bossManager_.get());
	/// ──────────────── ゲームフローの初期化 ───────────────
	clearSeq_ = std::make_unique<TKM::ClearSequenceController>();
	clearSeq_->Initialize(camera_.get(), player_.get(), bossManager_.get(), flow_.get(), dxCommon_, skybox_.get(), fireworkController_.get());
	if (flow_) {
		flow_->BindClearSequence(clearSeq_.get()); // ゲームフローにクリアシーケンスをバインド
	}
}

void GameScene::Finalize() {
	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();
	// 終了処理
	AudioManager::GetInstance()->Finalize();
	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();

	if (postFx_) { // ポストエフェクトの終了
		postFx_->Finalize();
	}
}

void GameScene::Update() {
	float rawDt = 0.0f; // 生のデルタタイム（ポーズ中も進む）
	float scaledDt = 0.0f; // スケール済みデルタタイム（ポーズ中は0）

	BeginFrameUpdate(rawDt, scaledDt); // フレーム開始処理

	if (flow_ && flow_->UpdateClear(rawDt, scaledDt, postFx_.get(), ui_.get(), bossManager_.get(), camera_.get(), player_.get())) { // クリアシーケンス更新
		return; // クリアシーケンス中は他の更新をスキップ
	}

	UpdateFlow(); // ゲーム進行フロー更新

	// Intro等でロック中はポーズを開けない（誤動作防止）
	const bool isClear = (flow_ && flow_->IsInClear());
	const bool locked = (flow_ && flow_->IsGameplayLocked()) || isClear;
	const bool allowPauseOpen = !locked;

	// ──────────────── ポーズ更新（rawDtでUIだけ動かす） ───────────────
	if (pause_) {
		const auto cmd = pause_->Update(rawDt, allowPauseOpen); // ポーズメニュー更新
		if (ui_) { // HUD透明度調整
			const float hudAlpha = pause_->IsPaused() ? 0.25f : 1.0f; // ポーズ中は半透明に
			ui_->SetHudAlpha(hudAlpha); // HUD透明度セット
		}

		if (cmd == TKM::PauseMenuController::Command::ReturnToTitle) {
			if (flow_) { // タイトル戻りリクエスト
				flow_->RequestToTitleByIris(); // いつものアイリスで戻す
			}
		} else if (cmd == TKM::PauseMenuController::Command::Restart) { // リスタート
			sceneManager_->SetNextScene(new GameScene(dxCommon_, srvManager_)); // 新しいゲームシーンをセット
			return;
		}

		// ポーズ中はゲーム本体を止める。ただし「遷移（タイトル戻り等）」は回す
		if (pause_->IsPaused()) { // ポーズ中
			// デバッグ表示更新
			ImGuiDebug();
			// アクティブカメラの更新
			UpdateActiveCamera();
			// ポーズ中はゲーム更新をスキップ
			UpdateTransitionsAndSceneChange(rawDt);
			// デバッグキー＆リクエスト処理
			HandleDebugKeysAndRequests();
			// フレーム終了処理
			EndFrameUpdate();
			return;
		}
	}

	// ──────────────── ゲーム本体更新（scaledDtで動かす） ───────────────
	// タイムスケールコントローラー更新
	UpdateEnemyAndWaveLogic(scaledDt);
	// デバッグ表示更新
	ImGuiDebug();
	// アクティブカメラの更新
	UpdateActiveCamera();
	// ゲームプレイシステムの更新
	UpdateGameplaySystems(rawDt, scaledDt);
	// シーン遷移＆タイトル戻り等の更新
	UpdateTransitionsAndSceneChange(rawDt);
	// デバッグキー＆リクエスト処理
	HandleDebugKeysAndRequests();
	// フレーム終了処理
	EndFrameUpdate();
}

void GameScene::Draw() {
	if (skybox_) skybox_->Draw(); // スカイボックスの描画

	// 3Dまとめ
	Object3dCommon::GetInstance()->DrawSetCommon();
	//for (auto& g : groundTiles_) g->Draw(dxCommon);
	player_->Draw(dxCommon_); // プレイヤーの描画

	if (enemyManager_) {
		enemyManager_->Draw(dxCommon_); // 敵群の描画を EnemyManager に委譲
	}

	const bool isClear = (flow_ && flow_->IsInClear()); // クリアシーケンス中か？
	if (!isClear) {
		if (bossManager_) {
			bossManager_->Draw(dxCommon_); // ボスマネージャの描画
		}
	}

	TKM::Camera* activeCamera = (useDebugCamera_ && debugCamera_) ? (TKM::Camera*)debugCamera_.get() : camera_.get();
	if (postFx_) {
		postFx_->DrawVolumes(activeCamera); // ポストエフェクトのボリューム系エフェクト描画
	}

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

#ifdef USE_IMGUI
	// ライン描画
	Matrix4x4 vp;
	if (useDebugCamera_ && debugCamera_) {
		vp = debugCamera_->GetViewProjectionMatrix();
	} else {
		vp = camera_->GetViewProjectionMatrix();
	}
	LineRenderer::GetInstance()->Draw(vp);
#endif
	// スプライトまとめ
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	if (flow_) { flow_->Draw(); } // ゲームフローの描画
	if (ui_) { ui_->Draw(); } // UIの描画
	if (pause_) { pause_->Draw(); } // ポーズメニューの描画
	if (bossManager_) { bossManager_->DrawUI(); } // ボスのUI描画
}

void GameScene::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	if (bossManager_) {
		bossManager_->SpawnEnemyBullet(pos, dir, speed, damage, lifeFrame); // ボスマネージャに委譲
	}
}

TKM::Camera* GameScene::UpdateActiveCamera() {
	// ──────────────── アクティブカメラの決定＆更新 ───────────────
	TKM::Camera* activeCamera = camera_.get();
	if (useDebugCamera_ && debugCamera_) {
		debugCamera_->Update();	// デバッグカメラを更新
		activeCamera = debugCamera_.get();
	} else {
		// 通常カメラを更新
		camera_->Update();
	}

	// ここで「今フレームのカメラ」を全部に渡す
	if (player_) {
		player_->SetCamera(activeCamera);
	}
	if (skybox_) {
		skybox_->SetCamera(activeCamera); // スカイボックス適用
	}
	if (enemyManager_) {
		enemyManager_->SetCamera(activeCamera); // 敵マネージャ適用
	}
	if (bossManager_) {
		bossManager_->SetCamera(activeCamera); // ボスマネージャ適用
	}

	// パーティクルマネージャー適用
	ParticleManager::GetInstance()->SetCamera(activeCamera);

	if (postFx_) { // ポストエフェクト適用
		postFx_->OnCameraUpdated(activeCamera); // カメラ更新通知
	}

	return activeCamera; // 呼び出し元にも返す
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// ゲーム内のサウンドをロード＆再生する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeAudio() {
	auto* audio = AudioManager::GetInstance();
	audio->Initialize();
	audio->LoadSound("bossP2", "FLASHness.wav"); // ボス戦フェーズ2用BGM ( FLASHness / NEURAY )
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要なテクスチャをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadTextures() {
	//ファイルパス
	TextureManager::GetInstance()->LoadTexture("./resources/texture/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/rostock_laage_airport_4k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/test.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/Ground.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/start.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/damageSpark.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/firework_star.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/RB_ui.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/LB_ui.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/X_ui.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/LS_ui.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/resume_pause.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/restart_pause.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/title_pause.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/gauge_fill_grad.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/gauge_frame_glass.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/RB_gauge_ui.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/gauge_shard.jpeg");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/blue.jpg");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/gray.jpg");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/player_hp.jpg");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/player_hp_frame.jpg");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/uvChecker.dds");
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite() {

	// ──────────────── ゲームフローの初期化 ───────────────
	flow_ = std::make_unique<TKM::GameFlowController>();
	flow_->Initialize(dxCommon_);
	// ──────────────── UIコントローラーの初期化 ───────────────
	ui_ = std::make_unique<TKM::UIController>();
	const float w = (float)WindowsAPI::kClientWidth_;
	const float h = (float)WindowsAPI::kClientHeight_;
	ui_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, this, w, h);
	// ──────────────── ポーズメニュー（形だけ） ───────────────
	pause_ = std::make_unique<TKM::PauseMenuController>();
	pause_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, this, w, h);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要な3Dモデルをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadModels() {
	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("turtle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("turtle_flipper.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("jerryfish.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("jerryfish_boss.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("tentacle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("tentacle_boss.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("reticle_big.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("reticle_normal.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("reticle_small.obj", dxCommon_);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeObjects() {
	// ──────────────── プレイヤーの初期化 ───────────────
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	player_->SetPosition({ 0.0f, 0.0f, 0.0f });
	player_->SetParentScene(this);
	player_->SetEnemy(nullptr); // 最初はボスはいないのでnullptr
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを作成し、各オブジェクトに適用する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeCamera() {
	camera_ = std::make_unique<TKM::Camera>();

	// IntroSequence に移したなら、ここは固定値でOK
	const float camPitchStart = 0.12f;
	const float camYawStart = -1.2f;

	camera_->SetRotate({ camPitchStart, camYawStart, 0.0f });
	camera_->SetTranslate({ 0.0f,0.0f,-30.0f });

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(
		camera_->GetTranslate(),
		Vector3{ 0.0f, 0.0f, 0.0f }
	);

	player_->SetCamera(camera_.get());
}

void GameScene::ImGuiDebug() {
#ifdef USE_IMGUI
	/////////////////////////////////////////////////////
	player_->ImGuiDebug(); // プレイヤーのデバッグ表示
	/////////////////////////////////////////////////////
	if (bossManager_ && bossManager_->GetBoss()) { // ボスマネージャ＆ボスが存在するなら
		bossManager_->GetBoss()->ImGuiDebug(); // ボスのデバッグ表示
	}
	/////////////////////////////////////////////////////
	enemyManager_->ImGuiDebug(); // 敵マネージャのデバッグ表示
	/////////////////////////////////////////////////////
	//camera->ImGuiDebug(); // カメラのデバッグ表示

	ImGui::Begin("デバッグカメラ");
	ImGui::Checkbox("オン/オフ", &useDebugCamera_);
	ImGui::Text("DebugCam: RMB rotate, LMB/Z, MMB/Y");
	ImGui::End();
	/////////////////////////////////////////////////////
	//skybox_->ImGuiUpdate(); // スカイボックスのデバッグ表示
	/////////////////////////////////////////////////////
	//if (postFx_) {
	//	postFx_->ImGuiDebug();
	//}
	///////////////////////////////////////////////////////
	//ImGuiDebugGamepad(); // ゲームパッド入力デバッグ
	//ImGuiDebugInfo(); // パフォーマンス情報デバッグ
	/////////////////////////////////////////////////////
#endif
}

void GameScene::UpdateAirStreak(float dt) {
	if (!player_) { return; } // プレイヤーがいないなら何もしない
	// プレイヤーの速度を取得
	airStreakTimer_ += dt;
	// どれくらいの密度で出すか（小さいほど密度↑）
	const float emitInterval = 0.02f; // 0.02秒ごと ≒ 1秒あたり50個

	while (airStreakTimer_ >= emitInterval) { // 一定時間経過したら出す
		airStreakTimer_ -= emitInterval; // タイマーリセット

		// カメラ基準ベクトル
		const Matrix4x4 camW = camera_->GetWorldMatrix();
		Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
		Vector3 camFwd = MyMath::Normalize(Vector3{ camW.m[2][0], camW.m[2][1], camW.m[2][2] });
		Vector3 camRight = MyMath::Normalize(Vector3{ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
		Vector3 camUp = MyMath::Normalize(Vector3{ camW.m[1][0], camW.m[1][1], camW.m[1][2] });
		// 乱数生成ラムダ
		auto rand01 = []() { return MyMath::Rand01(); };
		// ─────────────────────────────
		// カメラ前方の「巨大な箱」の中に出す
		// ─────────────────────────────
		const float boxHalfWidth = 40.0f;  // X方向（左右）±40
		const float boxHalfHeight = 25.0f;  // Y方向（上下）±25
		const float depthNear = 10.0f;  // カメラから10手前
		const float depthFar = 120.0f; // カメラから120まで

		// 画面中心はちょっと避けたいので、中心半径を決める
		const float centerHoleRadius = 3.0f; // この半径内は出にくくする

		// 平面オフセット（x,y）を決める
		float offsetX = 0.0f;
		float offsetY = 0.0f;

		for (int tries = 0; tries < 4; ++tries) { // 最大4回までリトライ
			// -1～+1 の乱数を生成
			float u = rand01() * 2.0f - 1.0f;
			float v = rand01() * 2.0f - 1.0f;
			// スケールして箱内の座標に変換
			float x = u * boxHalfWidth;
			float y = v * boxHalfHeight;
			// 中心付近を少しだけ避ける
			if (x * x + y * y < centerHoleRadius * centerHoleRadius) {
				// たまになら良いので、25%くらいの確率で許可
				if (rand01() > 0.25f) {
					continue; // 取り直し
				}
			}
			// 成功したらループ脱出
			offsetX = x;
			offsetY = y;
			break;
		}

		// 奥行き（カメラからの距離）
		float tDepth = rand01();
		float depth = MyMath::Lerp(depthNear, depthFar, tDepth);

		// ワールド座標に変換
		Vector3 emitPos =
			camPos
			+ camFwd * depth
			+ camRight * offsetX
			+ camUp * offsetY;
		ParticleManager::GetInstance()->Emit("airStreak", emitPos, 1); // airStreakパーティクルを1個出す
	}
}

void GameScene::BeginFrameUpdate(float& rawDt, float& scaledDt) {
	// 入力処理
	Input::GetInstance()->Update();
	// 毎フレームの最初に、前フレームのラインをクリア
	LineRenderer::GetInstance()->BeginFrame();
	// フレームタイム計測
	rawDt = dt_; /// デフォルトデルタタイム（補間なし）
	timeScale_.Update(rawDt); // タイムスケールコントローラーの更新
	scaledDt = rawDt * timeScale_.GetScale(); /// スローデルタタイム

	// 描画コール・メモリの初期化
	ResetDrawCallCount();
	UpdateMemory();
}

void GameScene::UpdateFlow() {
	if (flow_) { // ゲームフローの更新
		flow_->Update(dt_, camera_.get(), enemiesInitialized_, requestInitEnemies_);
	}
}

void GameScene::UpdateEnemyAndWaveLogic(float scaledDt) {
	const bool isClear = (flow_ && flow_->IsInClear()); // クリア演出中かどうか
	const bool locked = (flow_ && flow_->IsGameplayLocked()) || isClear; // ゲームプレイがロックされているかどうか

	// --- 敵とWaveは「ゲーム開始後」だけ動かす ---
	if (!locked && enemiesInitialized_) {

		// 敵の更新（敵ロジックは EnemyManager に完全委譲）
		if (enemyManager_) {
			enemyManager_->Update(scaledDt);
		}

		// 全てのWaveが終了していて、敵がいない → ボスへ進行 or クリア処理
		if (enemyManager_ && enemyManager_->IsAllWavesCleared()) {
			if (bossManager_) {
				// まだボス戦始まっていなければ開始
				if (!bossManager_->IsBattleActive() && !bossManager_->IsBossDead()) {
					bossManager_->StartBattle();
				} else {
					// ボス撃破 → クリア演出へ
					if (bossManager_->IsBossDead()) {
						if (!isClear) { // まだクリア演出始まっていなければ開始
							if (flow_) { // ゲームフローコントローラー経由でクリアシーケンス開始
								flow_->RequestStartClear(); // クリアシーケンス開始リクエスト
							}
							return;
						}
					}
				}
			}
		}

		// ロックオン対象の更新
		if (bossManager_ && bossManager_->IsBossAlive()) {
			player_->SetEnemy(bossManager_->GetBoss());
		} else {
			enemyManager_->UpdateClosestEnemy();
		}
	}
}

void GameScene::UpdateGameplaySystems(float dt, float scaledDt) {
	const bool isClear = (flow_ && flow_->IsInClear()); // クリア演出中かどうか
	const bool locked = (flow_ && flow_->IsGameplayLocked()) || isClear; // ゲームプレイがロックされているかどうか

	if (!enemyManager_) {
		return;
	}

	// スカイボックスの回転更新
	skybox_->UpdateRotation();
	// プレイヤーの更新
	player_->Update(scaledDt);

	if (ui_) {
		ui_->Update(scaledDt, player_.get());
	}

	// ボスマネージャの更新
	if (bossManager_) {
		bossManager_->Update(scaledDt);
	}
	// ポストエフェクトの更新
	if (postFx_) {
		postFx_->Update(scaledDt, bossManager_.get());
	}

	// ライトの更新
	directionalLight_->Update();

	if (!isClear && !locked) {
		UpdateAirStreak(dt);
	}

	// その他のオブジェクト・パーティクルの更新
	ParticleManager::GetInstance()->Update(scaledDt);
}

void GameScene::UpdateTransitionsAndSceneChange(float dt) {
	// 遷移は enemyManager_ の有無に依存させない（ここが原因になりやすい）
	if (flow_) {
		const auto req = flow_->UpdateTransitions(dt, player_.get());

		if (req == TKM::GameFlowController::TransitionRequest::ToTitle) { // タイトル戻りリクエスト
			sceneManager_->SetNextScene(new TitleScene(dxCommon_, srvManager_)); // タイトルシーンをセット
			return;
		}

		if (req == TKM::GameFlowController::TransitionRequest::ToGameOver) { // ゲームオーバーリクエスト
			sceneManager_->SetNextScene(new GameOverScene(dxCommon_, srvManager_)); // ゲームオーバーシーンをセット
			return;
		}

		if (req == TKM::GameFlowController::TransitionRequest::ToGameClear) { // ゲームクリアリクエスト
			sceneManager_->SetNextScene(new GameClearScene(dxCommon_, srvManager_)); // ゲームクリアシーンをセット
			return;
		}
	}
}

void GameScene::HandleDebugKeysAndRequests() {
	if (!enemyManager_) {
		return; // 敵マネージャがないなら何もしない
	}

	// ─── キーボードのYキーでプレイヤーのHPを0にする（デバッグ用）───
	if (Input::GetInstance()->TriggerKey(DIK_Y)) {
		if (player_) player_->SetHP(0);
	}

	// ── 敵初期化要求が来ていたら実行 ──
	if (requestInitEnemies_) {
		enemyManager_->InitializeWaves();
		enemiesInitialized_ = true;
		requestInitEnemies_ = false;
	}
}

void GameScene::EndFrameUpdate() {
	if (!enemyManager_) {
		return; // 敵マネージャがないなら何もしない
	}

	// パフォーマンス情報・デバッグUI
	UpdatePerformanceInfo();
}