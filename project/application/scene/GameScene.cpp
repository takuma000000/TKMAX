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
	TextureCatalog::LoadTextureCatalogs(); // テクスチャカタログのロード
	InitializeSprite();  // スプライトの作成＆初期化
	ModelCatalog::LoadModelCatalogs(dxCommon_); // モデルカタログのロード
	InitializeObjects(); // 3Dオブジェクトの作成＆初期化
	InitializeCamera();  // カメラの作成＆設定
	/// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize({ 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);
	/// ──────────────── ラインレンダラーの初期化 ───────────────
	LineRenderer::GetInstance()->Initialize(dxCommon_);
	/// ──────────────── パーティクルの初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, TKM::CameraManager::GetInstance()->GetMainCamera());
	// パーティクルグループの登録は ParticleGroupsCatalogクラス へ
	ParticleGroupsCatalog::RegisterScene(ParticleManager::GetInstance());
	// パーティクルエミッターの初期化
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
	bossEntranceSeq_ = std::make_unique<BossEntranceSequence>(); // ボス登場シーケンスの初期化
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
	clearSeq_->Initialize(player_.get(), bossManager_.get(), flow_.get(), dxCommon_, skybox_.get(), fireworkController_.get());
	flow_->BindClearSequence(clearSeq_.get()); // ゲームフローにクリアシーケンスをバインド
}

void GameScene::Finalize() {
	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();
	// 終了処理
	AudioManager::GetInstance()->Finalize();
	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();
	// ラインレンダラーの終了
	postFx_->Finalize();

	// パーティクルの終了
	ParticleManager::GetInstance()->ClearGroup("fw_launch"); // 花火打ち上げ
	ParticleManager::GetInstance()->ClearGroup("fw_flash"); // 花火閃光
	ParticleManager::GetInstance()->ClearGroup("fw_burst"); // 花火爆発
}

void GameScene::Update() {
	float rawDeltaTime = 0.0f;   // 前フレームからの経過時間（秒） - ゲーム全体の更新に使用（ポーズ中も動かす）
	float scaledDeltaTime = 0.0f; // タイムスケール適用後の経過時間（秒） - ゲームプレイシステムの更新に使用（ポーズ中は動かさない）

	BeginFrameUpdate(rawDeltaTime, scaledDeltaTime); // フレーム開始処理

	// クリアシーケンス中は他の更新をスキップ
	if (flow_->UpdateClear(rawDeltaTime, scaledDeltaTime, postFx_.get(), ui_.get(), bossManager_.get(), TKM::CameraManager::GetInstance()->GetMainCamera(), player_.get())) {
		return;
	}

	UpdateFlow(); // ゲーム進行フロー更新

	// ──────────────── 空の色変更（イントロ / ボス登場 / ボス戦中） ────────────────
	if (skybox_) {
		const bool introBossRed =
			(flow_ && flow_->GetIntro() && flow_->GetIntro()->IsBossSkyRedPhase());

		const bool entranceActive =
			(bossEntranceSeq_ && bossEntranceSeq_->IsActive());

		const bool bossBattleRed =
			(bossManager_ && bossManager_->IsBattleActive() && !bossManager_->IsBossDead());

		if (entranceActive) {
			skybox_->SetColor(bossEntranceSeq_->GetSkyColor());
		} else if (bossBattleRed) {
			skybox_->SetColor({ 10.0f, 0.0f, 0.0f, 1.0f });
		} else if (introBossRed) {
			skybox_->SetColor({ 10.0f, 0.0f, 0.0f, 1.0f });
		} else {
			skybox_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	}

	// ──────────────── ゲームプレイのロック状態を判定 ────────────────
	const bool isClear = (flow_->IsInClear());
	const bool bossEntranceLocked = (bossEntranceSeq_ && bossEntranceSeq_->IsActive());
	const bool locked = (flow_->IsGameplayLocked()) || isClear || bossEntranceLocked;
	const bool allowPauseOpen = !locked;

	// ──────────────── ポーズUI更新（rawDeltaTimeでUIだけ動かす） ───────────────
	if (TryUpdatePauseAndMaybeEarlyReturn_(rawDeltaTime, allowPauseOpen)) {
		EndFrameUpdate(); // ゲームプレイシステムの更新をスキップする場合でも、タイムスケールの更新は行う（ポーズ中のUIアニメーション等に反映させるため）
		return;
	}

	// ──────────────── 通常ゲーム更新（scaledDeltaTimeで動かす） ───────────────
	UpdateNormalGameplay_(rawDeltaTime, scaledDeltaTime);
	// タイムスケールの更新は最後に行う（ゲームプレイシステムの更新が終わってから適用されるようにするため）
	EndFrameUpdate();
}

void GameScene::Draw() { Draw3D(); DrawSprite(); } // 3Dとスプライトの描画を分ける

void GameScene::Draw3D() {
	skybox_->Draw(); // スカイボックス描画

	// 3Dオブジェクト描画の共通設定
	Object3dCommon::GetInstance()->DrawSetCommon();
	player_->Draw(dxCommon_);
	enemyManager_->Draw(dxCommon_);
	flow_->DrawIntroBoss3D(dxCommon_);
	const bool isClear = (flow_->IsInClear());
	// クリアシーケンス中はボスを描画しない（撃破後の演出で、ボスが残っていると見栄えが悪いため）
	if (!isClear) {
		bossManager_->Draw(dxCommon_);
	}

	TKM::Camera* activeCamera = TKM::CameraManager::GetInstance()->GetActiveCamera(); // 今フレームのアクティブカメラを取得
	postFx_->DrawVolumes(activeCamera); // ポストエフェクトのボリューム描画（デバッグ用）zyaa

	ParticleManager::GetInstance()->Draw();

	player_->DrawTrails(dxCommon_); // プレイヤーの軌跡描画（パーティクルの後に描くことで、パーティクルの前に来るようにする）

#ifdef USE_IMGUI
	TKM::Camera* cam = TKM::CameraManager::GetInstance()->GetActiveCamera();
	if (cam) {
		LineRenderer::GetInstance()->Draw(cam->GetViewProjectionMatrix());
	}
#endif
}

void GameScene::DrawSprite() {
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	flow_->Draw(); // ゲームフローの描画（イントロシーケンス等）
	ui_->Draw(); // HUD描画
	pause_->Draw(); // ポーズメニュー描画
	bossManager_->DrawUI(); // ボスマネージャのUI描画（HPゲージ等）
}

void GameScene::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	bossManager_->SpawnEnemyBullet(pos, dir, speed, damage, lifeFrame); // ボスマネージャに委譲
}

TKM::Camera* GameScene::UpdateActiveCamera() {
	// ──────────────── アクティブカメラの決定＆更新 ───────────────
	TKM::Camera* activeCamera = TKM::CameraManager::GetInstance()->Update(); // カメラマネージャーに更新を任せて、今フレームのアクティブカメラを取得
	if (!activeCamera) { return nullptr; } // 万が一カメラマネージャーからnullptrが返ってきたら更新をスキップ（通常はありえないはず）

	// ここで「今フレームのカメラ」を全部に渡す
	player_->SetCamera(activeCamera);
	skybox_->SetCamera(activeCamera); // スカイボックス適用
	enemyManager_->SetCamera(activeCamera); // 敵マネージャ適用
	bossManager_->SetCamera(activeCamera); // ボスマネージャ適用
	// パーティクルマネージャー適用
	ParticleManager::GetInstance()->SetCamera(activeCamera);
	postFx_->OnCameraUpdated(activeCamera); // カメラ更新通知

	return activeCamera; // 呼び出し元にも返す
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// ゲーム内のサウンドをロード＆再生する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeAudio() {
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite() {

	// ──────────────── ゲームフローの初期化 ───────────────
	flow_ = std::make_unique<TKM::GameFlowController>();
	flow_->Initialize(dxCommon_, TKM::Object3dCommon::GetInstance());
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
	const Vector3 mainRot = { 0.12f,-1.2f,0.0f }; // カメラの回転（ラジアン）
	const Vector3 mainPos = { 0.0f,0.0f,-30.0f }; // カメラの位置
	const Vector3 debugTarget = { 0.0f, 0.0f, 0.0f }; // デバッグカメラの注視点
	// カメラマネージャーの初期化
	TKM::CameraManager::GetInstance()->Initialize(mainRot, mainPos, debugTarget);

	player_->SetCamera(TKM::CameraManager::GetInstance()->GetMainCamera()); // プレイヤーに通常カメラを適用
}

void GameScene::ImGuiDebug() {
#ifdef USE_IMGUI
	/////////////////////////////////////////////////////
	player_->ImGuiDebug(); // プレイヤーのデバッグ表示
	/////////////////////////////////////////////////////
	if (bossManager_->GetBoss()) { // ボスマネージャ＆ボスが存在するなら
		bossManager_->GetBoss()->ImGuiDebug(); // ボスのデバッグ表示
	}
	/////////////////////////////////////////////////////
	enemyManager_->ImGuiDebug(); // 敵マネージャのデバッグ表示
	/////////////////////////////////////////////////////
	bool useDbg = TKM::CameraManager::GetInstance()->IsUsingDebugCamera();
	if (ImGui::Checkbox("オン/オフ", &useDbg)) {
		TKM::CameraManager::GetInstance()->SetUseDebugCamera(useDbg);
	}
	/////////////////////////////////////////////////////
	//skybox_->ImGuiUpdate(); // スカイボックスのデバッグ表示
	/////////////////////////////////////////////////////
	postFx_->ImGuiDebug(); // ポストエフェクトのデバッグ表示
	///////////////////////////////////////////////////////
	//ImGuiDebugGamepad(); // ゲームパッド入力デバッグ
	ImGuiDebugInfo(); // パフォーマンス情報デバッグ
	/////////////////////////////////////////////////////
#endif
}

void GameScene::UpdateAirStreak(float rawDeltaTime) {
	// プレイヤーの速度を取得
	airStreakTimer_ += rawDeltaTime;
	// どれくらいの密度で出すか（小さいほど密度↑）
	const float emitInterval = 0.02f; // 0.02秒ごと ≒ 1秒あたり50個
	// アクティブなカメラを取得
	auto* cam = TKM::CameraManager::GetInstance()->GetActiveCamera();
	if (!cam) { return; } // 万が一カメラが存在しない場合は出さない（通常はありえないはず）

	while (airStreakTimer_ >= emitInterval) { // 一定時間経過したら出す
		airStreakTimer_ -= emitInterval; // タイマーリセット

		// カメラ基準ベクトル
		const Matrix4x4 camW = cam->GetWorldMatrix();
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

void GameScene::BeginFrameUpdate(float& outRawDeltaTime, float& outScaledDeltaTime) {
	// 入力処理
	Input::GetInstance()->Update();
	// 毎フレームの最初に、前フレームのラインをクリア
	LineRenderer::GetInstance()->BeginFrame();
	// フレームタイム計測
	outRawDeltaTime = kFixedDeltaTime_; /// デフォルトデルタタイム（補間なし）
	timeScale_.Update(outRawDeltaTime); // タイムスケールコントローラーの更新
	outScaledDeltaTime = outRawDeltaTime * timeScale_.GetScale(); /// スローデルタタイム

	// 描画コール・メモリの初期化
	ResetDrawCallCount();
	UpdateMemory();
}

void GameScene::UpdateFlow() {
	flow_->Update(kFixedDeltaTime_, TKM::CameraManager::GetInstance()->GetActiveCamera(), enemiesInitialized_, requestInitEnemies_); // ゲームフローの更新（イントロシーケンスの進行管理など）
}

void GameScene::UpdateEnemyAndWaveLogic(float scaledDeltaTime) {
	const bool isClear = (flow_->IsInClear()); // クリア演出中かどうか
	const bool bossEntranceActive = (bossEntranceSeq_ && bossEntranceSeq_->IsActive());
	const bool locked = (flow_->IsGameplayLocked()) || isClear || bossEntranceActive;

	// --- 敵とWaveは「ゲーム開始後」だけ動かす ---
	if (!locked && enemiesInitialized_) {

		// 敵の更新（敵ロジックは EnemyManager に完全委譲）

		enemyManager_->Update(scaledDeltaTime);

		// 全Waveクリア → ボス戦開始 or クリア演出へ
		if (enemyManager_->IsAllWavesCleared()) {
			// まだボス戦始まっていないなら、即開始ではなく専用登場演出を挟む
			if (!bossManager_->IsBattleActive() && !bossManager_->IsBossDead()) {
				if (bossEntranceSeq_) {
					if (!bossEntranceSeq_->IsActive()) {
						bossEntranceSeq_->Start(bossManager_->GetSpawnPos());
					}
				} else {
					// 念のためのフォールバック
					bossManager_->StartBattle();
				}
			} else {
				// ボス撃破 → クリア演出へ
				if (bossManager_->IsBossDead()) {
					if (!isClear) {
						flow_->RequestStartClear();
						return;
					}
				}
			}
		}

		// ロックオン対象の更新
		if (bossManager_->IsBossAlive()) {
			player_->SetEnemy(bossManager_->GetBoss());
		} else {
			enemyManager_->UpdateClosestEnemy();
		}
	}
}

void GameScene::UpdateGameplaySystems(float rawDeltaTime, float scaledDeltaTime) {
	const bool isClear = (flow_->IsInClear()); // クリア演出中かどうか
	const bool bossEntranceActive = (bossEntranceSeq_ && bossEntranceSeq_->IsActive());
	const bool bossEntranceSpawned = (bossEntranceSeq_ && bossEntranceSeq_->HasSpawnedBoss());
	const bool locked = (flow_->IsGameplayLocked()) || isClear || bossEntranceActive; // ゲームプレイがロックされているかどうか

	player_->SetControlEnabled(!locked);

	if (bossEntranceSeq_ && bossEntranceSeq_->IsActive()) {
		bossEntranceSeq_->Update(rawDeltaTime, bossManager_.get());
	}

	// スカイボックスの回転更新
	skybox_->UpdateRotation();

	// プレイヤーの更新
	player_->Update(scaledDeltaTime);

	// ゲーム開始前かどうか（ゲーム開始前は通常HUDを表示せず、開幕ボス演出のSkipUIだけ表示する）
	const bool isBeforeGameStart =
		(flow_ && flow_->IsGameplayLocked());
	// 開幕ボス演出中かどうか
	const bool isOpeningBossIntro =
		(flow_ && flow_->GetIntro() && flow_->GetIntro()->CanSkipBossIntro());
	// 開幕ボス演出中だけ SkipUI を表示
	ui_->SetIntroSkipUiActive(isOpeningBossIntro);
	// ゲームスタート後だけ通常HUDを表示
	ui_->SetGameplayHudVisible(!isBeforeGameStart);
	// UIの更新
	ui_->Update(scaledDeltaTime, player_.get());

	// ボスマネージャの更新
	// 演出中でも、ボス生成後は本体のEnter移動を見せるため更新を回す
	if (!bossEntranceActive || bossEntranceSpawned) {
		bossManager_->Update(scaledDeltaTime);
	}
	// ポストエフェクトの更新
	postFx_->Update(scaledDeltaTime, bossManager_.get());

	// ライトの更新
	directionalLight_->Update();

	if (!isClear && !locked) {
		UpdateAirStreak(rawDeltaTime);
	}

	// その他のオブジェクト・パーティクルの更新
	ParticleManager::GetInstance()->Update(scaledDeltaTime);
}

void GameScene::UpdateTransitionsAndSceneChange(float rawDeltaTime) {
	// 遷移は enemyManager_ の有無に依存させない
	const auto req = flow_->UpdateTransitions(rawDeltaTime, player_.get());

	if (req == TKM::GameFlowController::TransitionRequest::ToTitle) { // タイトル戻りリクエスト
		sceneManager_->SetNextScene(std::make_unique<TitleScene>(dxCommon_, srvManager_)); // タイトルシーンをセット
		return;
	}

	if (req == TKM::GameFlowController::TransitionRequest::ToGameOver) { // ゲームオーバーリクエスト
		sceneManager_->SetNextScene(std::make_unique<GameOverScene>(dxCommon_, srvManager_)); // ゲームオーバーシーンをセット
		return;
	}

	if (req == TKM::GameFlowController::TransitionRequest::ToGameClear) { // ゲームクリアリクエスト
		sceneManager_->SetNextScene(std::make_unique<GameClearScene>(dxCommon_, srvManager_)); // ゲームクリアシーンをセット
		return;
	}
}

void GameScene::HandleDebugKeysAndRequests() {
	// ─── キーボードのYキーでプレイヤーのHPを0にする（デバッグ用）───
	if (Input::GetInstance()->TriggerKey(DIK_Y)) {
		player_->SetHP(0);
	}

	// ── 敵初期化要求が来ていたら実行 ──
	if (requestInitEnemies_) {
		enemyManager_->InitializeWaves();
		enemiesInitialized_ = true;
		requestInitEnemies_ = false;
	}
}

void GameScene::EndFrameUpdate() {
	// パフォーマンス情報・デバッグUI
	UpdatePerformanceInfo();
}

bool GameScene::TryUpdatePauseAndMaybeEarlyReturn_(float rawDeltaTime, bool allowPauseOpen) {
	if (!pause_) { return false; }

	// ポーズメニュー更新
	const auto cmd = pause_->Update(rawDeltaTime, allowPauseOpen);

	// HUD透明度調整
	const float hudAlpha = pause_->IsPaused() ? 0.25f : 1.0f;
	ui_->SetHudAlpha(hudAlpha);

	// ポーズメニューのコマンド処理
	if (cmd == TKM::PauseMenuController::Command::ReturnToTitle) {
		flow_->RequestToTitleByIris();
	} else if (cmd == TKM::PauseMenuController::Command::Restart) {
		sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_));
		return true; // シーン差し替え要求（このフレームは終了）
	}

	// ポーズ中はゲーム本体を止める。ただし「遷移（タイトル戻り等）」は回す
	if (!pause_->IsPaused()) { return false; }

	UpdatePausedOnly_(rawDeltaTime);
	return true;
}

void GameScene::UpdatePausedOnly_(float rawDeltaTime) {
	// デバッグ表示更新
	ImGuiDebug();
	// アクティブカメラの更新（ポーズ中も視点操作は許可）
	UpdateActiveCamera();
	// ポーズ中はゲーム更新をスキップするが、遷移は回す
	UpdateTransitionsAndSceneChange(rawDeltaTime);
	// デバッグキー＆リクエスト処理
	HandleDebugKeysAndRequests();
}

void GameScene::UpdateNormalGameplay_(float rawDeltaTime, float scaledDeltaTime) {
	// アクティブカメラの更新
	UpdateActiveCamera();
	// 敵やウェーブのロジック更新（タイムスケール適用）
	UpdateEnemyAndWaveLogic(scaledDeltaTime);
	// デバッグ表示更新
	ImGuiDebug();
	// ゲームプレイシステムの更新
	UpdateGameplaySystems(rawDeltaTime, scaledDeltaTime);
	// シーン遷移＆タイトル戻り等の更新（rawDeltaTime）
	UpdateTransitionsAndSceneChange(rawDeltaTime);
	// デバッグキー＆リクエスト処理
	HandleDebugKeysAndRequests();
}