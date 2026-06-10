#define NOMINMAX
#include "GameScene.h"
#include "AudioCatalog.h"
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
	AudioCatalog::LoadGameAudios();
	TextureCatalog::LoadTextureCatalogs();
	ModelCatalog::LoadModelCatalogs(dxCommon_);
	InitializeObjects();
	InitializeSprite();
	InitializeCamera();

	/// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize({ 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);

	/// ──────────────── ラインレンダラーの初期化 ───────────────
	LineRenderer::GetInstance()->Initialize(dxCommon_);

	/// ──────────────── パーティクルの初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, TKM::CameraManager::GetInstance()->GetMainCamera());
	ParticleGroupsCatalog::RegisterScene(ParticleManager::GetInstance());

	particleEmitter_ = std::make_unique<ParticleEmitter>();
	particleEmitter_->Initialize("uv", { 0.0f,2.5f,10.0f });

	/// ──────────────── スカイボックスの初期化 ───────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(TKM::CameraManager::GetInstance()->GetMainCamera());

	/// ──────────────── 敵マネージャの初期化 ───────────────
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize(dxCommon_, TKM::CameraManager::GetInstance()->GetMainCamera(), this, player_.get());

	/// ──────────────── ボスマネージャの初期化 ───────────────
	bossManager_ = std::make_unique<BossManager>();
	bossManager_->Initialize(dxCommon_, TKM::CameraManager::GetInstance()->GetMainCamera(), this, player_.get());

	bossEntranceSeq_ = std::make_unique<BossEntranceSequence>();

	/// ──────────────── タイムスケールコントローラーの初期化 ───────────────
	timeScale_.Initialize();
	bossManager_->SetTimeScaleController(&timeScale_);

	/// ──────────────── 花火コントローラーの初期化 ───────────────
	fireworkController_ = std::make_unique<TKM::FireworkController>();
	fireworkController_->Reset();

	/// ──────────────── ポストエフェクトの初期化 ───────────────
	postFx_ = std::make_unique<TKM::PostEffectController>();
	postFx_->Initialize(dxCommon_, player_.get(), bossManager_.get());
	flow_->GetIntro()->SetPostEffectController(postFx_.get()); // イントロの演出にもポストエフェクトを渡す

	/// ──────────────── ゲームフローの初期化 ───────────────
	clearSeq_ = std::make_unique<TKM::ClearSequenceController>();
	clearSeq_->Initialize(player_.get(), bossManager_.get(), flow_.get(), dxCommon_, skybox_.get(), fireworkController_.get(), postFx_->GetSmokeVolume());
	// クリア演出側からシーン遷移要求を返せるようにFlowへ紐づける
	flow_->BindClearSequence(clearSeq_.get());

	/// ──────────────── ゲーム開始時の状態設定 ───────────────
	if (startFromBoss_) {
		// ボス戦から再開する場合、開幕イントロを完全に終了扱いにする
		if (flow_ && flow_->GetIntro()) {
			flow_->GetIntro()->ForceComplete();
		}

		// 敵はもう初期化済み扱いにする
		enemiesInitialized_ = true;
		requestInitEnemies_ = false;

		// START表示後の前進演出も発生しないようにする
		wasStartVisibleLastFrame_ = false;
		playerIntroMoveStarted_ = true;

		// ゲーム開始BGMを鳴らさないようにする
		gameStartedBGMPlayed_ = true;

		// 雑魚戦フェーズを終わらせて、ボス戦準備状態へ進める
		if (enemyManager_) {
			enemyManager_->FinishSmallEnemyPhase();
		}

		// ボス戦から再開する場合は、即本戦開始ではなくボス登場演出から始める
		if (bossEntranceSeq_ && bossManager_) {
			player_->SetShootingEnabled(false);
			bossEntranceSeq_->Start(bossManager_->GetSpawnPos());
		}
	}
}

void GameScene::Finalize() {
	/// ──────────────── 各種終了処理 ───────────────
	TextureManager::GetInstance()->Finalize();
	AudioManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();

	/// ──────────────── ポストエフェクトの終了 ───────────────
	postFx_->Finalize();

	/// ──────────────── パーティクルの終了 ───────────────
	ParticleManager::GetInstance()->ClearAllGroups();
}

void GameScene::Update() {
	/// ──────────────── フレーム時間の準備 ───────────────
	float rawDeltaTime = 0.0f;
	float scaledDeltaTime = 0.0f;

	BeginFrameUpdate(rawDeltaTime, scaledDeltaTime);

	/// ──────────────── クリアシーケンス更新 ───────────────
	if (flow_->UpdateClear(rawDeltaTime, scaledDeltaTime, postFx_.get(), ui_.get(), bossManager_.get(), TKM::CameraManager::GetInstance()->GetMainCamera(), player_.get())) {
		// クリア演出中は通常ゲーム更新を進めない
		EndFrameUpdate();
		return;
	}

	/// ──────────────── ゲーム進行フロー更新 ───────────────
	UpdateFlow();

	/// ──────────────── START表示が消えた瞬間にプレイヤー前進演出を開始 ────────────────
	// 前フレームのSTART表示状態と現在の状態を比較して、START表示が消えた瞬間を検出する
	const bool startVisibleNow =
		flow_ &&
		flow_->GetIntro() &&
		flow_->GetIntro()->IsStartVisible();
	// START表示が消えた瞬間にプレイヤーの前進演出を開始する
	if (!playerIntroMoveStarted_ &&
		wasStartVisibleLastFrame_ &&
		!startVisibleNow) {

		// カメラがZ=-30なので、画面側はマイナスZ
		player_->StartIntroForwardMove(-40.0f, 1.8f);
		playerIntroMoveStarted_ = true; // このフラグが立ったら以降はこの処理を行わない
	}
	// フレーム更新の最後に現在のSTART表示状態を保存しておく
	wasStartVisibleLastFrame_ = startVisibleNow;

	/// ──────────────── 空の色変更 ───────────────
	if (skybox_) {
		const bool introBossRed =
			(flow_ && flow_->GetIntro() && flow_->GetIntro()->IsBossSkyRedPhase());

		const bool entranceActive =
			(bossEntranceSeq_ && bossEntranceSeq_->IsActive());

		const bool bossBattleRed =
			(bossManager_ && bossManager_->IsBattleActive() && !bossManager_->IsBossDead());

		if (entranceActive) {
			// ボス登場演出中は演出側が管理している空色を使う
			skybox_->SetColor(bossEntranceSeq_->GetSkyColor());
		} else if (bossBattleRed) {
			// ボス戦中は赤くして緊張感を出す
			skybox_->SetColor({ 10.0f, 0.0f, 0.0f, 1.0f });
		} else if (introBossRed) {
			// イントロのボス演出中も赤空にする
			skybox_->SetColor({ 10.0f, 0.0f, 0.0f, 1.0f });
		} else {
			// 通常時は元の色に戻す
			skybox_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}

	/// ──────────────── ゲームプレイのロック状態を判定 ───────────────
	const bool isClear = (flow_->IsInClear());
	const bool bossEntranceLocked = (bossEntranceSeq_ && bossEntranceSeq_->IsActive());
	const bool locked = (flow_->IsGameplayLocked()) || isClear || bossEntranceLocked;
	const bool allowPauseOpen = !locked;

	/// ──────────────── ポーズUI更新 ───────────────
	if (TryUpdatePauseAndMaybeEarlyReturn_(rawDeltaTime, allowPauseOpen)) {
		// ポーズ中は通常ゲーム更新を行わない
		EndFrameUpdate();
		return;
	}

	/// ──────────────── 通常ゲーム更新 ───────────────
	UpdateNormalGameplay_(rawDeltaTime, scaledDeltaTime);

	/// ──────────────── フレーム終了処理 ───────────────
	EndFrameUpdate();

#ifdef USE_IMGUI
	/// ──────────────── デバッグ表示更新 ───────────────
	ImGuiDebug();
#endif
}

void GameScene::Draw() {
	Draw3D();
	DrawSprite();
}

void GameScene::Draw3D() {
	/// ──────────────── 背景描画 ───────────────
	skybox_->Draw();

	/// ──────────────── 3Dオブジェクト描画 ───────────────
	Object3dCommon::GetInstance()->DrawSetCommon();

	player_->Draw(dxCommon_);
	enemyManager_->Draw(dxCommon_);
	flow_->DrawIntroBoss3D(dxCommon_);

	const bool isClear = (flow_->IsInClear()) || clearSequenceTriggered_;

	if (!isClear) {
		// クリア演出中はボス本体の通常描画を止める
		bossManager_->Draw(dxCommon_);
	}

	/// ──────────────── ボリューム描画 ───────────────
	TKM::Camera* activeCamera = TKM::CameraManager::GetInstance()->GetActiveCamera();
	postFx_->DrawVolumes(activeCamera);

	/// ──────────────── パーティクル描画 ───────────────
	ParticleManager::GetInstance()->Draw();

	/// ──────────────── プレイヤー軌跡描画 ───────────────
	player_->DrawTrails(dxCommon_);

#ifdef USE_IMGUI
	/// ──────────────── デバッグライン描画 ───────────────
	TKM::Camera* cam = TKM::CameraManager::GetInstance()->GetActiveCamera();
	if (cam) {
		LineRenderer::GetInstance()->Draw(cam->GetViewProjectionMatrix());
	}
#endif
}

void GameScene::DrawSprite() {
	/// ──────────────── 2D描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── UI描画 ───────────────
	flow_->Draw();
	ui_->Draw();
	pause_->Draw();
	bossManager_->DrawUI();
}

void GameScene::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	/// ──────────────── 敵弾生成をボスマネージャへ委譲 ───────────────
	bossManager_->SpawnEnemyBullet(pos, dir, speed, damage, lifeFrame);
}

TKM::Camera* GameScene::UpdateActiveCamera() {
	/// ──────────────── アクティブカメラの決定 ───────────────
	TKM::Camera* activeCamera = TKM::CameraManager::GetInstance()->Update();
	if (!activeCamera) { return nullptr; }

	/// ──────────────── 各システムへカメラ反映 ───────────────
	player_->SetCamera(activeCamera);
	skybox_->SetCamera(activeCamera);
	enemyManager_->SetCamera(activeCamera);
	bossManager_->SetCamera(activeCamera);

	/// ──────────────── パーティクル・ポストエフェクトへカメラ反映 ───────────────
	ParticleManager::GetInstance()->SetCamera(activeCamera);
	postFx_->OnCameraUpdated(activeCamera);

	return activeCamera;
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite() {
	/// ──────────────── ゲームフローの初期化 ───────────────
	flow_ = std::make_unique<TKM::GameFlowController>();
	flow_->Initialize(dxCommon_, TKM::Object3dCommon::GetInstance());

	/// ──────────────── UIコントローラーの初期化 ───────────────
	ui_ = std::make_unique<TKM::UIController>();

	const float w = static_cast<float>(WindowsAPI::GetClientWidth());
	const float h = static_cast<float>(WindowsAPI::GetClientHeight());

	ui_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, this, w, h, player_.get());

	/// ──────────────── ポーズメニューの初期化 ───────────────
	pause_ = std::make_unique<TKM::PauseMenuController>();
	pause_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, this, w, h);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeObjects() {
	/// ──────────────── プレイヤーの初期化 ───────────────
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	player_->SetPosition({ 0.0f, 0.0f, 0.0f });
	player_->SetParentScene(this);
	player_->SetEnemy(nullptr);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを作成し、各オブジェクトに適用する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeCamera() {
	/// ──────────────── カメラ初期値 ───────────────
	Vector3 mainRot = { 0.12f, -1.2f, 0.0f };
	Vector3 mainPos = { 0.0f, 0.0f, -30.0f };
	Vector3 debugTarget = { 0.0f, 0.0f, 0.0f };

	// ボス戦から再開する場合は、通常開幕カメラではなくボス戦用のカメラ状態にする
	if (startFromBoss_) {
		mainRot = { 0.05f, 0.0f, 0.0f };
		mainPos = { 0.0f, 0.0f, -30.0f };
		debugTarget = { 0.0f, 0.0f, 42.0f };
	}

	/// ──────────────── カメラマネージャー初期化 ───────────────
	TKM::CameraManager::GetInstance()->Initialize(mainRot, mainPos, debugTarget);

	// 行列を即更新して、1フレームだけ古いカメラになるのを防ぐ
	if (TKM::CameraManager::GetInstance()->GetMainCamera()) {
		TKM::CameraManager::GetInstance()->GetMainCamera()->Update();
	}

	/// ──────────────── プレイヤーへ通常カメラを適用 ───────────────
	player_->SetCamera(TKM::CameraManager::GetInstance()->GetMainCamera());
}

void GameScene::ImGuiDebug() {
#ifdef USE_IMGUI
	/// ──────────────── プレイヤーデバッグ ───────────────
	player_->ImGuiDebug();

	/// ──────────────── ボスデバッグ ───────────────
	if (bossManager_->GetBoss()) {
		bossManager_->GetBoss()->ImGuiDebug();
	}

	/// ──────────────── 敵マネージャデバッグ ───────────────
	enemyManager_->ImGuiDebug();

	/// ──────────────── デバッグカメラ切り替え ───────────────
	bool useDbg = TKM::CameraManager::GetInstance()->IsUsingDebugCamera();
	if (ImGui::Checkbox("オン/オフ", &useDbg)) {
		TKM::CameraManager::GetInstance()->SetUseDebugCamera(useDbg);
	}

	/// ──────────────── ポストエフェクトデバッグ ───────────────
	postFx_->ImGuiDebug();
#endif
}

void GameScene::UpdateAirStreak(float rawDeltaTime) {
	/// ──────────────── 発生タイマー更新 ───────────────
	airStreakTimer_ += rawDeltaTime;

	const float emitInterval = 0.035f;

	/// ──────────────── アクティブカメラ取得 ───────────────
	auto* cam = TKM::CameraManager::GetInstance()->GetActiveCamera();
	if (!cam) { return; }

	/// ──────────────── 一定間隔で空気の流れを発生 ───────────────
	while (airStreakTimer_ >= emitInterval) {
		airStreakTimer_ -= emitInterval;

		/// ──────────────── カメラ基準ベクトル取得 ───────────────
		const Matrix4x4 camW = cam->GetWorldMatrix();
		Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
		Vector3 camFwd = MyMath::Normalize(Vector3{ camW.m[2][0], camW.m[2][1], camW.m[2][2] });
		Vector3 camRight = MyMath::Normalize(Vector3{ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
		Vector3 camUp = MyMath::Normalize(Vector3{ camW.m[1][0], camW.m[1][1], camW.m[1][2] });

		/// ──────────────── 乱数生成 ───────────────
		auto rand01 = []() { return MyMath::Rand01(); };

		/// ──────────────── 発生範囲設定 ───────────────
		const float boxHalfWidth = 40.0f;
		const float boxHalfHeight = 25.0f;
		const float depthNear = 10.0f;
		const float depthFar = 120.0f;
		const float centerHoleRadius = 3.0f;

		float offsetX = 0.0f;
		float offsetY = 0.0f;

		/// ──────────────── 画面中心を避けた発生位置の平面オフセット計算 ───────────────
		for (int tries = 0; tries < 4; ++tries) {
			float u = rand01() * 2.0f - 1.0f;
			float v = rand01() * 2.0f - 1.0f;

			float x = u * boxHalfWidth;
			float y = v * boxHalfHeight;

			if (x * x + y * y < centerHoleRadius * centerHoleRadius) {
				// 画面中央に出すぎると視界の邪魔になるため、基本は避ける
				if (rand01() > 0.25f) {
					continue;
				}
			}

			offsetX = x;
			offsetY = y;
			break;
		}

		/// ──────────────── 奥行き計算 ───────────────
		float tDepth = rand01();
		float depth = MyMath::Lerp(depthNear, depthFar, tDepth);

		/// ──────────────── ワールド座標へ変換して発生 ───────────────
		Vector3 emitPos =
			camPos
			+ camFwd * depth
			+ camRight * offsetX
			+ camUp * offsetY;

		ParticleManager::GetInstance()->Emit("airStreak", emitPos, 1);
	}
}

void GameScene::BeginFrameUpdate(float& outRawDeltaTime, float& outScaledDeltaTime) {
	/// ──────────────── 入力更新 ───────────────
	Input::GetInstance()->Update();

	/// ──────────────── デバッグライン初期化 ───────────────
	LineRenderer::GetInstance()->BeginFrame();

	/// ──────────────── フレームタイム計測 ───────────────
	outRawDeltaTime = kFixedDeltaTime_;

	/// ──────────────── タイムスケール更新 ───────────────
	timeScale_.Update(outRawDeltaTime);
	outScaledDeltaTime = outRawDeltaTime * timeScale_.GetScale();

	/// ──────────────── メモリ情報更新 ───────────────
	UpdateMemory();
}

void GameScene::UpdateFlow() {
	/// ──────────────── ゲームフロー更新 ───────────────
	flow_->Update(kFixedDeltaTime_, TKM::CameraManager::GetInstance()->GetActiveCamera(), enemiesInitialized_, requestInitEnemies_);
}

void GameScene::UpdateEnemyAndWaveLogic(float scaledDeltaTime) {
	/// ──────────────── ロック状態判定 ───────────────
	const bool isClear = (flow_->IsInClear());
	const bool bossEntranceActive = (bossEntranceSeq_ && bossEntranceSeq_->IsActive());
	const bool locked = (flow_->IsGameplayLocked()) || isClear || bossEntranceActive || clearSequenceTriggered_;

	if (locked || !enemiesInitialized_) {
		// ロック中や敵未生成の状態では敵フェーズを進めない
		return;
	}

	/// ──────────────── 敵Wave更新 ───────────────
	enemyManager_->Update(scaledDeltaTime);

	/// ──────────────── ボス撃破時のクリア演出開始 ───────────────
	if (bossManager_ && bossManager_->IsBossDead()) {
		clearSequenceTriggered_ = true; // クリア演出を二重開始しないためのフラグ
		flow_->RequestStartClear();
		return;
	}

	/// ──────────────── 小型敵フェーズ終了後のボス登場開始 ───────────────
	if (!clearSequenceTriggered_ && enemyManager_->IsSmallEnemyPhaseFinished()) {
		if (bossManager_ && bossManager_->GetBoss() == nullptr) {
			if (bossEntranceSeq_) {
				if (!bossEntranceSeq_->IsActive()) {
					player_->SetShootingEnabled(false); // 登場演出中は射撃を止める
					bossEntranceSeq_->Start(bossManager_->GetSpawnPos());
				}
			} else {
				player_->SetShootingEnabled(false); // 即ボス戦へ入る場合も一度射撃を止める
				bossManager_->StartBattle();
			}
		}
	}

	/// ──────────────── ロックオン対象更新 ───────────────
	if (bossManager_->IsBossAlive()) {
		player_->SetEnemy(bossManager_->GetBoss()); // ボスがいる間はボスを優先ターゲットにする
	} else {
		enemyManager_->UpdateClosestEnemy();
	}
}

void GameScene::UpdateGameplaySystems(float rawDeltaTime, float scaledDeltaTime) {
	/// ──────────────── クリア中はゲームプレイ更新しない ───────────────
	if (flow_ && flow_->IsInClear()) {
		return;
	}

	/// ──────────────── ゲームプレイロック状態判定 ───────────────
	const bool isClear = (flow_->IsInClear());
	const bool bossEntranceActive = (bossEntranceSeq_ && bossEntranceSeq_->IsActive());
	const bool bossEntranceSpawned = (bossEntranceSeq_ && bossEntranceSeq_->HasSpawnedBoss());
	const bool locked = (flow_->IsGameplayLocked()) || isClear || bossEntranceActive || clearSequenceTriggered_;

	player_->SetControlEnabled(!locked && !player_->IsDead());
	player_->SetShootingEnabled(!locked && !player_->IsDead());

	/// ──────────────── ゲーム開始演出中はレティクル非表示 ───────────────
	const bool isGameplayLocked =
		(flow_ && flow_->IsGameplayLocked());
	// ゲーム開始演出が終わるまではレティクルを表示しない
	player_->SetReticleVisible(!isGameplayLocked);

	/// ──────────────── ボス登場演出更新 ───────────────
	if (bossEntranceSeq_ && bossEntranceSeq_->IsActive()) {
		bossEntranceSeq_->Update(rawDeltaTime, bossManager_.get());
	}

	/// ──────────────── スカイボックス更新 ───────────────
	//skybox_->UpdateRotation();

	/// ──────────────── プレイヤー更新 ───────────────
	player_->Update(scaledDeltaTime);

	/// ──────────────── ゲーム開始状態判定 ───────────────
	const bool isBeforeGameStart =
		(flow_ && flow_->IsGameplayLocked());

	const bool isStartVisible =
		(flow_ && flow_->GetIntro() && flow_->GetIntro()->IsStartVisible());

	/// ──────────────── ゲーム開始BGM再生 ───────────────
	if (isStartVisible && !gameStartedBGMPlayed_) {
		TKM::AudioManager::GetInstance()->PlaySound("playBGM", 0.1f, true);
		gameStartedBGMPlayed_ = true; // BGMを一度だけ鳴らす
	}

	/// ──────────────── 開幕ボス演出中のUI制御 ───────────────
	const bool isOpeningBossIntro =
		(flow_ && flow_->GetIntro() && flow_->GetIntro()->CanSkipBossIntro());

	ui_->SetIntroSkipUiActive(isOpeningBossIntro);
	ui_->SetGameplayHudVisible(!isBeforeGameStart);

	/// ──────────────── UI更新 ───────────────
	ui_->Update(scaledDeltaTime);

	// スキップゲージが最大まで溜まったら開幕演出をスキップする
	if (ui_->IsIntroSkipCompleted()) {
		// 開幕演出スキップのフラグを立てる
		if (flow_ && flow_->GetIntro()) {
			flow_->GetIntro()->SkipBossIntroToShowStart(
				TKM::CameraManager::GetInstance()->GetMainCamera() // 開幕演出スキップの要求をFlowに伝える
			);
		}
	}

	/// ──────────────── ボスマネージャ更新 ───────────────
	if (!bossEntranceActive || bossEntranceSpawned) {
		// ボス登場演出中でも、生成後はボス側の更新を許可する
		bossManager_->Update(scaledDeltaTime);
	}

	/// ──────────────── ポストエフェクト更新 ───────────────
	postFx_->Update(scaledDeltaTime, bossManager_.get());

	/// ──────────────── ライト更新 ───────────────
	directionalLight_->Update();

	/// ──────────────── 空気の流れエフェクト更新 ───────────────
	if (!isClear && !locked) {
		UpdateAirStreak(rawDeltaTime);
	}

	/// ──────────────── パーティクル更新 ───────────────
	ParticleManager::GetInstance()->Update(scaledDeltaTime);
}

void GameScene::UpdateTransitionsAndSceneChange(float rawDeltaTime) {
	/// ──────────────── 遷移要求取得 ───────────────
	const auto req = flow_->UpdateTransitions(rawDeltaTime, player_.get());

	/// ──────────────── タイトルへ遷移 ───────────────
	if (req == TKM::GameFlowController::TransitionRequest::ToTitle) {
		sceneManager_->SetNextScene(std::make_unique<TitleScene>(dxCommon_, srvManager_));
		return;
	}

	/// ──────────────── ゲームオーバーへ遷移 ───────────────
	if (req == TKM::GameFlowController::TransitionRequest::ToGameOver) {
		// ボス戦中に死亡したかを確認する
		const bool diedInBossBattle =
			bossManager_ && bossManager_->IsBattleActive();

		// GameOverSceneへ、ボス戦中に死亡したかを渡す
		sceneManager_->SetNextScene(
			std::make_unique<GameOverScene>(dxCommon_, srvManager_, diedInBossBattle)
		);
		return;
	}

	/// ──────────────── ゲームクリアへ遷移 ───────────────
	if (req == TKM::GameFlowController::TransitionRequest::ToGameClear) {
		sceneManager_->SetNextScene(std::make_unique<GameClearScene>(dxCommon_, srvManager_));
		return;
	}

	/// ──────────────── リスタート遷移 ───────────────
	if (req == TKM::GameFlowController::TransitionRequest::ToRestart) {
		sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_));
		return;
	}
}

void GameScene::HandleDebugKeysAndRequests() {
	/// ──────────────── デバッグキー処理 ───────────────
	if (Input::GetInstance()->TriggerKey(DIK_Y)) {
		player_->SetHP(0);
	}

	/// ──────────────── 敵初期化要求処理 ───────────────
	if (requestInitEnemies_) {
		enemyManager_->StartSmallEnemyPhase();
		enemiesInitialized_ = true;
		requestInitEnemies_ = false;
	}
}

void GameScene::EndFrameUpdate() {
	/// ──────────────── パフォーマンス情報更新 ───────────────
	UpdatePerformanceInfo();
}

bool GameScene::TryUpdatePauseAndMaybeEarlyReturn_(float rawDeltaTime, bool allowPauseOpen) {
	/// ──────────────── ポーズ未生成チェック ───────────────
	if (!pause_) { return false; }

	/// ──────────────── ポーズメニュー更新 ───────────────
	const auto cmd = pause_->Update(rawDeltaTime, allowPauseOpen);

	const bool isPausedNow = pause_->IsPaused();

	/// ──────────────── BGMポーズ・再開制御 ───────────────
	if (isPausedNow && !wasPausedLastFrame_) {
		// ポーズに入った瞬間だけBGMを一時停止する
		TKM::AudioManager::GetInstance()->PauseSound("playBGM");
		TKM::AudioManager::GetInstance()->PauseSound("bossPhaseBGM");
	} else if (!isPausedNow && wasPausedLastFrame_) {
		// ポーズ解除の瞬間だけBGMを再開する
		TKM::AudioManager::GetInstance()->ResumeSound("playBGM");
		TKM::AudioManager::GetInstance()->ResumeSound("bossPhaseBGM");
	}

	wasPausedLastFrame_ = isPausedNow;

	/// ──────────────── HUD透明度調整 ───────────────
	const float hudAlpha = isPausedNow ? 0.25f : 1.0f;
	ui_->SetHudAlpha(hudAlpha);

	/// ──────────────── ポーズメニューコマンド処理 ───────────────
	if (cmd == TKM::PauseMenuController::Command::ReturnToTitle) {
		flow_->RequestToTitleByIris();
	} else if (cmd == TKM::PauseMenuController::Command::Restart) {
		flow_->RequestRestartByIris();
	}

	/// ──────────────── ポーズ中でなければ通常更新へ戻る ───────────────
	if (!isPausedNow) { return false; }

	/// ──────────────── ポーズ中専用更新 ───────────────
	UpdatePausedOnly_(rawDeltaTime);
	return true;
}

void GameScene::UpdatePausedOnly_(float rawDeltaTime) {

	/// ──────────────── アクティブカメラ更新 ───────────────
	UpdateActiveCamera();

	/// ──────────────── 遷移更新 ───────────────
	UpdateTransitionsAndSceneChange(rawDeltaTime);

	/// ──────────────── デバッグキー・要求処理 ───────────────
	HandleDebugKeysAndRequests();
}

void GameScene::UpdateNormalGameplay_(float rawDeltaTime, float scaledDeltaTime) {
	/// ──────────────── アクティブカメラ更新 ───────────────
	UpdateActiveCamera();

	/// ──────────────── 敵・Waveロジック更新 ───────────────
	UpdateEnemyAndWaveLogic(scaledDeltaTime);

	/// ──────────────── クリア要求後の早期終了 ───────────────
	if (flow_ && flow_->IsInClear()) {
		// クリア状態に入ったら通常更新をここで止める
		UpdateTransitionsAndSceneChange(rawDeltaTime);
		HandleDebugKeysAndRequests();
		return;
	}

	/// ──────────────── ゲームプレイシステム更新 ───────────────
	UpdateGameplaySystems(rawDeltaTime, scaledDeltaTime);

	/// ──────────────── シーン遷移更新 ───────────────
	UpdateTransitionsAndSceneChange(rawDeltaTime);

	/// ──────────────── デバッグキー・要求処理 ───────────────
	HandleDebugKeysAndRequests();
}