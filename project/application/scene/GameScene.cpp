#define NOMINMAX
#include "GameScene.h"
#include <limits>
#include <algorithm>
#include <psapi.h>
#include <Input.h>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using namespace TKM;

void GameScene::Initialize() {
	// ──────────────── NULLチェック ────────────────
	assert(this != nullptr && "this is nullptr in GameScene::Initialize");
	assert(dxCommon_ != nullptr && "dxCommon is nullptr in GameScene::Initialize");

	// ──────────────── 各種初期化処理 ───────────────
	InitializeAudio();   // サウンドのロード＆再生
	LoadTextures();      // テクスチャのロード
	InitializeSprite();  // スプライトの作成＆初期化
	LoadModels();        // 3Dモデルのロード
	InitializeObjects(); // 3Dオブジェクトの作成＆初期化
	InitializeCamera();  // カメラの作成＆設定

	// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize({ 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);
	// ──────────────── ラインレンダラーの初期化 ───────────────
	LineRenderer::GetInstance()->Initialize(dxCommon_);
	// ──────────────── パーティクルの初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());
	ParticleManager::GetInstance()->CreateParticleGroup("uv", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	particleEmitter_ = std::make_unique<ParticleEmitter>();
	particleEmitter_->Initialize("uv", { 0.0f,2.5f,10.0f });

	/// ===== パーティクルグループの作成 =====
	// 開幕用：うっすら光が吸い込まれるリング
	ParticleManager::GetInstance()->CreateParticleGroup("irisOpen", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// 花火用：放射状に飛ぶ粒（通常クアッド）
	ParticleManager::GetInstance()->CreateParticleGroup("irisFire", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// 花火用：打ち上げ＆閃光＆爆発
	ParticleManager::GetInstance()->CreateParticleGroup("fw_launch", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	ParticleManager::GetInstance()->CreateParticleGroup("fw_flash", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	ParticleManager::GetInstance()->CreateParticleGroup("fw_burst", "./resources/firework_star.png", ParticleManager::ParticleType::NORMAL);
	// 空気の流れ(風)エフェクト
	ParticleManager::GetInstance()->CreateParticleGroup("airStreak", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// 敵スポーン
	ParticleManager::GetInstance()->CreateParticleGroup("enemySpawn", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	/// === ここから被弾エフェクト用 ===
	// 中央の強いフラッシュ
	ParticleManager::GetInstance()->CreateParticleGroup("enemyHit_flash", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 外側に広がるリング
	ParticleManager::GetInstance()->CreateParticleGroup("enemyHit_ring", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// 放射状のレイ（細い光の筋）
	ParticleManager::GetInstance()->CreateParticleGroup("enemyHit_rays", "./resources/gradationLine.png", ParticleManager::ParticleType::NORMAL);
	// 小さいスパーク
	ParticleManager::GetInstance()->CreateParticleGroup("enemyHit_spark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	/// === ここから LT弾ヒット用・さらにド派手版 ===
	// 爆心コア（まぶしい光の玉）
	ParticleManager::GetInstance()->CreateParticleGroup("lt_nova_core", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 球状ショックウェーブ（外側のエネルギー殻）
	ParticleManager::GetInstance()->CreateParticleGroup("lt_nova_wave", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// デブリ＆煙（暗い破片、煙っぽい粒）
	ParticleManager::GetInstance()->CreateParticleGroup("lt_nova_debris", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// 亀裂エフェクト（空間が裂けるような光の筋）
	ParticleManager::GetInstance()->CreateParticleGroup("lt_nova_crack", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 爆発バースト（明るい爆発の粒）
	ParticleManager::GetInstance()->CreateParticleGroup("lt_nova_burst", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	/// === 敵吹っ飛び死亡専用エフェクト ===
	// 核となる小さな光の塊（中央でフッと光って消える）
	ParticleManager::GetInstance()->CreateParticleGroup("enemyDeath_core", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 周りに飛び散る光の破片
	ParticleManager::GetInstance()->CreateParticleGroup("enemyDeath_shard", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 残り香みたいにふわっと残る煙
	ParticleManager::GetInstance()->CreateParticleGroup("enemyDeath_smoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	/// === ボス撃破専用エフェクト ===
	// 揺れている最中にボンボン出る中サイズ爆発
	ParticleManager::GetInstance()->CreateParticleGroup("bossDeath_bomb", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 最後にドカンと出るリング衝撃波
	ParticleManager::GetInstance()->CreateParticleGroup("bossDeath_ring", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// 倒れたあとしばらく残る大きめの煙
	ParticleManager::GetInstance()->CreateParticleGroup("bossDeath_smoke", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// 爆心コア（画面中央でドーンと光る玉）
	ParticleManager::GetInstance()->CreateParticleGroup("bossClear_core", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 超デカいショックウェーブ（リング）
	ParticleManager::GetInstance()->CreateParticleGroup("bossClear_ring", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// 四方八方に飛ぶ光の破片
	ParticleManager::GetInstance()->CreateParticleGroup("bossClear_spark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 重めの破片・残り香みたいな煙
	ParticleManager::GetInstance()->CreateParticleGroup("bossClear_debris", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	/// === 敵飛び掛かり用エフェクト群 ===
	// 敵の飛び掛かり軌道レール
	ParticleManager::GetInstance()->CreateParticleGroup("enemyPounceTrail", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// 軌道上のスパーク
	ParticleManager::GetInstance()->CreateParticleGroup("enemyPounceSpark", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	/// === 蘇生核チャージ演出 ===
	// 外側を覆うエネルギー殻
	ParticleManager::GetInstance()->CreateParticleGroup("core_charge_shell", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// 内向きに吸い込まれる粒子
	ParticleManager::GetInstance()->CreateParticleGroup("core_charge_inward", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// リボン状のエネルギー帯
	ParticleManager::GetInstance()->CreateParticleGroup("core_charge_ribbon", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// 中心の強いフラッシュ
	ParticleManager::GetInstance()->CreateParticleGroup("core_charge_flash", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	/// === LT弾のチャージエフェクト ===
	// 外側を覆うエネルギー殻
	ParticleManager::GetInstance()->CreateParticleGroup("trail_lt_path", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	/// --- Boss Windup FX（予備動作）---
	// 外側を覆うリング状エネルギー
	ParticleManager::GetInstance()->CreateParticleGroup("boss_windup_shell", "./resources/gradationLine.png", ParticleManager::ParticleType::RING);
	// 火花がパチパチ飛ぶエフェクト
	ParticleManager::GetInstance()->CreateParticleGroup("boss_windup_crackle", "./resources/circle2.png", ParticleManager::ParticleType::NORMAL);
	// 内向きに吸い込まれる粒子
	ParticleManager::GetInstance()->CreateParticleGroup("boss_windup_inward", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	// ──────────────── スカイボックスの初期化 ───────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());
	// ──────────────── 敵マネージャの初期化 ───────────────
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize(dxCommon_, camera_.get(), this, player_.get());
	enemyManager_->BindEnemies(&enemies_, &defeatedEnemyCount_, &maxEnemyCount_);
	// ──────────────── ボスマネージャの初期化 ───────────────
	if (!bossManager_) {
		bossManager_ = std::make_unique<BossManager>();
	}
	bossManager_->Initialize(dxCommon_, camera_.get(), this, player_.get());
	// ──────────────── タイムスケールコントローラーの初期化 ───────────────
	timeScale_.Initialize();
	bossManager_->SetTimeScaleController(&timeScale_);
	// ──────────────── 花火コントローラーの初期化 ───────────────
	fireworkController_ = std::make_unique<TKM::FireworkController>();
	fireworkController_->Reset();
	// ──────────────── ポストエフェクトの初期化 ───────────────
	postFx_ = std::make_unique<TKM::PostEffectController>();
	postFx_->Initialize(dxCommon_, player_.get(), bossManager_.get());
	// ──────────────── ゲームフローの初期化 ───────────────
	clearSeq_ = std::make_unique<TKM::ClearSequenceController>();
	clearSeq_->Initialize(camera_.get(), player_.get(), bossManager_.get(), flow_.get(), dxCommon_, skybox_.get(), fireworkController_.get());
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
	// 入力処理
	Input::GetInstance()->Update();
	// 毎フレームの最初に、前フレームのラインをクリア
	LineRenderer::GetInstance()->BeginFrame();
	// フレームタイム計測
	const float rawDt = dt_; /// デフォルトデルタタイム（補間なし）
	timeScale_.Update(rawDt); // タイムスケールコントローラーの更新
	const float scaledDt = rawDt * timeScale_.GetScale(); /// スローデルタタイム

	// 描画コール・メモリの初期化
	ResetDrawCallCount();
	UpdateMemory();

	// クリア演出中なら専用処理だけ回して終わり
	if (clearSeq_ && clearSeq_->IsActive()) {

		// ゲームフローの更新
		if (flow_) {
			flow_->Update(dt_, camera_.get(), enemiesInitialized_, requestInitEnemies_);
		}

		// クリア演出本体（スロー非依存）
		bool finished = clearSeq_->Update(dt_);

		// クリア中でも動かしたいもの（止めない）
		ParticleManager::GetInstance()->Update(scaledDt);

		if (postFx_) {
			postFx_->Update(scaledDt, bossManager_.get());
			postFx_->OnCameraUpdated(camera_.get());
		}

		if (ui_) {
			ui_->Update(scaledDt, player_.get());
		}

		if (finished) {
			sceneManager_->SetNextScene(new GameClearScene(dxCommon_, srvManager_));
			return;
		}

		UpdatePerformanceInfo();
		return;
	}

	if (flow_) { // ゲームフローの更新
		flow_->Update(dt_, camera_.get(), enemiesInitialized_, requestInitEnemies_);
	}
	const bool isClear = (clearSeq_ && clearSeq_->IsActive()); // クリア演出中かどうか
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
						if (!isClear) {
							StartClearSequence();
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

	// デバッグ用ImGui表示
	ImGuiDebug();
	// パフォーマンス情報更新
	UpdateActiveCamera();

	if (enemyManager_) {
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
			UpdateAirStreak(dt_);
		}

		// その他のオブジェクト・パーティクルの更新
		ParticleManager::GetInstance()->Update(scaledDt);

		if (flow_) {
			const auto req = flow_->UpdateTransitions(dt_, player_.get());
			if (req == TKM::GameFlowController::TransitionRequest::ToTitle) {
				sceneManager_->SetNextScene(new TitleScene(dxCommon_, srvManager_));
				return;
			}
			if (req == TKM::GameFlowController::TransitionRequest::ToGameOver) {
				sceneManager_->SetNextScene(new GameOverScene(dxCommon_, srvManager_));
				return;
			}
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

		// パフォーマンス情報・デバッグUI
		UpdatePerformanceInfo();
	}
}

void GameScene::Draw() {
	//if (skybox_) skybox_->Draw(); // スカイボックスの描画

	// 3Dまとめ
	Object3dCommon::GetInstance()->DrawSetCommon();
	//for (auto& g : groundTiles_) g->Draw(dxCommon);
	player_->Draw(dxCommon_); // プレイヤーの描画

	if (enemyManager_) {
		enemyManager_->Draw(dxCommon_); // 敵群の描画を EnemyManager に委譲
	}

	const bool isClear = (clearSeq_ && clearSeq_->IsActive()); // クリア演出中かどうか
	if (!isClear) {
		if (bossManager_) {
			bossManager_->Draw(dxCommon_);
		}
	}

	TKM::Camera* activeCamera = (useDebugCamera_ && debugCamera_) ? (TKM::Camera*)debugCamera_.get() : camera_.get();
	if (postFx_) {
		postFx_->DrawVolumes(activeCamera);
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
	// IntroSequence 側で Iris / start 表示を描画する
	if (flow_) {
		flow_->Draw();
	}
	if (ui_) {
		ui_->Draw();
	}
	if (bossManager_) { bossManager_->DrawUI(); }
}

void GameScene::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	if (bossManager_) {
		bossManager_->SpawnEnemyBullet(pos, dir, speed, damage, lifeFrame);
	}
}

TKM::Camera* GameScene::UpdateActiveCamera() {
	// ──────────────── アクティブカメラの決定＆更新 ───────────────
	TKM::Camera* activeCamera = camera_.get();
	if (useDebugCamera_ && debugCamera_) {
		// デバッグカメラを更新
		debugCamera_->Update();
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

	if (postFx_) {
		postFx_->OnCameraUpdated(activeCamera);
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
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./resources/pokemon.png");
	TextureManager::GetInstance()->LoadTexture("./resources/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./resources/gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("./resources/rostock_laage_airport_4k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/test.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/Ground.png");
	TextureManager::GetInstance()->LoadTexture("./resources/start.png");
	TextureManager::GetInstance()->LoadTexture("./resources/reticle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/damageSpark.png");
	TextureManager::GetInstance()->LoadTexture("./resources/firework_star.png");
	TextureManager::GetInstance()->LoadTexture("./resources/LB.png");
	TextureManager::GetInstance()->LoadTexture("./resources/LT.png");
	TextureManager::GetInstance()->LoadTexture("./resources/RB.png");
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.dds");
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite() {

	// ──────────────── ゲームフローの初期化 ───────────────
	flow_ = std::make_unique<TKM::GameFlowController>();
	flow_->Initialize(dxCommon_);

	ui_ = std::make_unique<TKM::UIController>();
	const float w = (float)WindowsAPI::kClientWidth_;
	const float h = (float)WindowsAPI::kClientHeight_;
	ui_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, this, w, h);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要な3Dモデルをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadModels() {
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("terrain.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("ground.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("jett.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("enemy.obj", dxCommon_);
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
	if (postFx_) {
		postFx_->ImGuiDebug();
	}
	/////////////////////////////////////////////////////
	//ImGuiDebugGamepad(); // ゲームパッド入力デバッグ
	//ImGuiDebugInfo(); // パフォーマンス情報デバッグ
	/////////////////////////////////////////////////////
#endif
}

void GameScene::StartClearSequence() {
	if (clearSeq_) {
		clearSeq_->Start();
	}
}

void GameScene::UpdateAirStreak(float dt) {
	if (!player_) { return; }

	airStreakTimer_ += dt;

	// どれくらいの密度で出すか（小さいほど密度↑）
	const float emitInterval = 0.02f; // 0.02秒ごと ≒ 1秒あたり50個

	while (airStreakTimer_ >= emitInterval) {
		airStreakTimer_ -= emitInterval;

		// カメラ基準ベクトル
		const Matrix4x4 camW = camera_->GetWorldMatrix();
		Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
		Vector3 camFwd = MyMath::Normalize(Vector3{ camW.m[2][0], camW.m[2][1], camW.m[2][2] });
		Vector3 camRight = MyMath::Normalize(Vector3{ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
		Vector3 camUp = MyMath::Normalize(Vector3{ camW.m[1][0], camW.m[1][1], camW.m[1][2] });

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

		for (int tries = 0; tries < 4; ++tries) {
			float u = rand01() * 2.0f - 1.0f; // -1～+1
			float v = rand01() * 2.0f - 1.0f;

			float x = u * boxHalfWidth;
			float y = v * boxHalfHeight;

			// 中心付近を少しだけ避ける
			if (x * x + y * y < centerHoleRadius * centerHoleRadius) {
				// たまになら良いので、25%くらいの確率で許可
				if (rand01() > 0.25f) {
					continue; // 取り直し
				}
			}

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

		ParticleManager::GetInstance()->Emit("airStreak", emitPos, 1);
	}
}