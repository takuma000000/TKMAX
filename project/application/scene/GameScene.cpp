#define NOMINMAX
#include "GameScene.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#include <limits>
#include <algorithm>
#include "MyMath.h"
#include <psapi.h>
#include <Input.h>

void GameScene::Initialize(){
	// ──────────────── NULLチェック ────────────────
	assert(this != nullptr && "this is nullptr in GameScene::Initialize");
	assert(dxCommon != nullptr && "dxCommon is nullptr in GameScene::Initialize");

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

	// ──────────────── パーティクルの初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon, srvManager, camera.get());
	ParticleManager::GetInstance()->CreateParticleGroup("uv", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	particleEmitter = std::make_unique<ParticleEmitter>();
	particleEmitter->Initialize("uv", { 0.0f,2.5f,10.0f });

	// 開幕用：うっすら光が吸い込まれるリング
	ParticleManager::GetInstance()->CreateParticleGroup(
		"irisOpen", "./resources/gradationLine.png",
		ParticleManager::ParticleType::RING
	);

	// リボンパーティクルのテスト用グループ（Aキーで出す）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"ribbonTest",
		"./resources/gradationLine.png",          // それっぽいテクスチャ。変えてOK
		ParticleManager::ParticleType::RIBBON     // ← ここがポイント
	);

	// 花火用：放射状に飛ぶ粒（通常クアッド）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"irisFire", "./resources/circle.png",
		ParticleManager::ParticleType::NORMAL
	);

	// 花火用：打ち上げ＆閃光＆爆発
	ParticleManager::GetInstance()->CreateParticleGroup("fw_launch", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	ParticleManager::GetInstance()->CreateParticleGroup("fw_flash", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	ParticleManager::GetInstance()->CreateParticleGroup("fw_burst", "./resources/firework_star.png", ParticleManager::ParticleType::NORMAL);

	// 空気の流れ(風)エフェクト
	ParticleManager::GetInstance()->CreateParticleGroup(
		"airStreak", "./resources/circle.png",
		ParticleManager::ParticleType::NORMAL
	);

	// 敵スポーン
	ParticleManager::GetInstance()->CreateParticleGroup(
		"enemySpawn", "./resources/circle.png",
		ParticleManager::ParticleType::NORMAL
	);

	// === ここから被弾エフェクト用 ===
// 中央の強いフラッシュ
	ParticleManager::GetInstance()->CreateParticleGroup(
		"enemyHit_flash", "./resources/circle2.png",
		ParticleManager::ParticleType::NORMAL);

	// 外側に広がるリング
	ParticleManager::GetInstance()->CreateParticleGroup(
		"enemyHit_ring", "./resources/gradationLine.png",
		ParticleManager::ParticleType::RING);

	// 放射状のレイ（細い光の筋）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"enemyHit_rays", "./resources/gradationLine.png",
		ParticleManager::ParticleType::NORMAL);

	// 小さいスパーク
	ParticleManager::GetInstance()->CreateParticleGroup(
		"enemyHit_spark", "./resources/circle2.png",
		ParticleManager::ParticleType::NORMAL);

	// === ここから LT弾ヒット用・さらにド派手版 ===
	// 爆心コア（まぶしい光の玉）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"lt_nova_core", "./resources/circle2.png",
		ParticleManager::ParticleType::NORMAL);

	// 球状ショックウェーブ（外側のエネルギー殻）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"lt_nova_wave", "./resources/gradationLine.png",
		ParticleManager::ParticleType::RING);

	// デブリ＆煙（暗い破片、煙っぽい粒）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"lt_nova_debris", "./resources/circle.png",
		ParticleManager::ParticleType::NORMAL);

	// 亀裂エフェクト（空間が裂けるような光の筋）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"lt_nova_crack", "./resources/circle2.png",
		ParticleManager::ParticleType::NORMAL);

	// 爆発バースト（明るい爆発の粒）
	ParticleManager::GetInstance()->CreateParticleGroup(
		"lt_nova_burst", "./resources/circle2.png",
		ParticleManager::ParticleType::NORMAL);

	// ──────────────── スカイボックスの初期化 ───────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon, srvManager, "resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera.get());

	iris_ = std::make_unique<Sprite>();
	iris_->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/circle2.png");

	// 画面中央に配置 & 対角長を最大に
	iris_->SetAnchorPoint({ 0.5f, 0.5f });
	iris_->SetPosition({ WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight * 0.5f });

	// 画面対角から最大スケールを計算
	const float diag = std::sqrt(
		float(WindowsAPI::kClientWidth) * float(WindowsAPI::kClientWidth) +
		float(WindowsAPI::kClientHeight) * float(WindowsAPI::kClientHeight)
	);
	irisMaxScale_ = diag * 2.0f;    // TitleSceneと対に合わせる

	irisStartScale_ = irisMaxScale_; // 最初は覆った状態
	irisEndScale_ = 0.0f;          // 最終的に消える
	irisScale_ = irisStartScale_;
	iris_->SetSize({ irisScale_, irisScale_ });
	irisTween_.Reset(/*start*/ irisMaxScale_, /*end*/ 0.0f, /*sec*/ 0.8f, Ease::Type::OutBack);

	// タイトル戻り用アイリス
	irisCloseScale_ = 0.0f;
	irisCloseTween_.Reset(0.0f, irisMaxScale_, 0.8f, Ease::Type::InBack);

	// ゲームスタート文字
	startSprite_ = std::make_unique<Sprite>();
	startSprite_->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/start.png");
	startSprite_->SetAnchorPoint({ 0.5f, 0.5f }); // 中央基準
	startSprite_->SetPosition({ startStartPos_.x, startStartPos_.y });
	startSprite_->SetSize({ 100, 100 }); // 画像サイズに合わせ調整
	startSprite_->SetColor({ 1,1,1,1 }); // アルファ1で開始
	startTween_.Reset(0.0f, 1.0f, startDuration_, Ease::Type::OutBack);
}

void GameScene::Finalize(){
	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();

	// 終了処理
	AudioManager::GetInstance()->Finalize();

	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();
}

void GameScene::Update(){
	// 入力処理
	Input::GetInstance()->Update();

	// 描画コール・メモリの初期化
	ResetDrawCallCount();
	UpdateMemory();

	// クリア演出中なら専用処理だけ回して終わり
	if (clearSequence_) {
		bool finished = UpdateClearSequence(dt);
		if (finished) {
			// アイリス閉じまで終わったので GameClearScene へ
			sceneManager_->SetNextScene(new GameClearScene(dxCommon, srvManager));
			return;
		}
		UpdatePerformanceInfo();
		return;
	}

	// --- 敵とWaveは「ゲーム開始後」だけ動かす ---
	if (!gameplayLocked_ && enemiesInitialized_) {

		// 敵の更新と削除
		UpdateEnemies();

		if (enemies_.empty()) {
			if (wavePhase_ != WavePhase::Done) {
				GoToNextWave();
			} else {
				if (!bossBattle_) {
					// ボス戦突入！
					bossBattle_ = true;
					boss_ = std::make_unique<BossEnemy>();
					boss_->Initialize(Object3dCommon::GetInstance(), dxCommon);
					boss_->SetParentScene(this);
					boss_->SetCamera(camera.get());
					boss_->SetPlayer([this]() { return player_->GetPosition(); });
					boss_->SetPosition({ 0, 0, 200 }); // 奥から出現
				} else {
					// ─── ボスが死んだらクリア演出開始 ───
					if (boss_ && boss_->IsDead()) {
						if (!clearSequence_) {
							StartClearSequence();
							return; // このフレームの通常処理はここで終わり
						}
					}
				}
			}
		}

		// 最も近い敵をプレイヤーに設定
		UpdateClosestEnemy();
	}

	ImGuiDebug();

	// プレイヤーと環境
	camera->Update();

	// Skybox 回転
	UpdateSkyboxRotationX();

	// ---- ground scroll ----
	UpdateGroundScroll();

	player_->Update();
	directionalLight_->Update();

	if (bossBattle_ && boss_) {
		boss_->Update();

		// P2突入時にBGMを1回だけ再生
		if (!bossP2BgmPlayed_) {
			int phase = boss_->GetPhase(); // P1=0, P2=1, P3=2

			if (phase == 1) { // P2
				AudioManager::GetInstance()->PlaySound("bossP2");
				bossP2BgmPlayed_ = true;   // 2回目以降は鳴らさない
			}
		}
	}

	// ゲームプレイ中だけ風エフェクト
	if (!clearSequence_ && !gameplayLocked_) {
		UpdateAirStreak(dt);
	}

	// これまで: if (irisOpening_) { ... emitFireworkPending_ の遅延 ... }
	if (irisOpening_) {
		// 共通の経過タイム：開始時刻からの積算
		emitOpenElapsed_ += dt;

		// ── リング（開始から emitOpenDelaySec_ 秒後に一度だけ） ──
		if (emitOpenBurst_ && emitOpenElapsed_ >= emitOpenDelaySec_) {
			emitOpenBurst_ = false;

			// カメラ前方の少し奥に発生させる
			const Matrix4x4 camW = camera->GetWorldMatrix();
			Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
			Vector3 camFwd = MyMath::Normalize(Vector3{ camW.m[2][0], camW.m[2][1], camW.m[2][2] });
			const float depth = 20.0f;

			Vector3 centerInFront = camPos + camFwd * depth;
			centerInFront.y -= 0.1f;

			// 吸い込みリングを即時発生
			ParticleManager::GetInstance()->Emit("irisOpen", centerInFront, 60);

			// 花火も同じ場所で出したいので座標を覚えておく
			lastEmitPos_ = centerInFront;

			// 旧仕様の「リング後からカウント」用は使わないためリセットだけ
			emitFireworkPending_ = true;        // フラグは立てたまま
			emitFireworkElapsed_ = 0.0f;        // 以後は使わない（念のため初期化）
		}

		// ── 花火（開始から emitFireworkDelaySec_ 秒後に一度だけ） ──
		// ※「リング後ではなく開始から」の基準に変更
		if (emitFireworkPending_ && emitOpenElapsed_ >= emitFireworkDelaySec_) {
			emitFireworkPending_ = false;

			// 同じ位置で出す（カメラ前を毎フレ計算したい場合は lastEmitPos_ ではなく再計算でもOK）
			ParticleManager::GetInstance()->Emit("irisFire", lastEmitPos_, 80);
		}

		// ── アイリスの見た目更新（従来どおり） ──
		irisScale_ = irisTween_.Update(0.016f);     // 実 deltaTime があるならそれを使うと安定
		iris_->SetSize({ irisScale_, irisScale_ });
		iris_->Update();

		if (irisShadow_) {
			irisShadow_->SetSize({ irisScale_ * 1.02f, irisShadow_->GetSize().y });
			irisShadow_->Update();
		}

		// ツイーン完了でオープニング終了
		if (irisTween_.Finished()) {
			irisOpening_ = false;
		}

		// ── カメラインロ：アイリスが終わったら一度だけ回転ツイーンを開始 ──
		if (!irisOpening_ && !camIntroActive_ && !camIntroDone_) {
			camIntroActive_ = true;
			// 横向き（camYawStart_）→ 正面（camYawEnd_）へ、OutBackで camIntroDuration_ 秒
			camYawTween_.Reset(camYawStart_, camYawEnd_, camIntroDuration_, Ease::Type::OutBack);
		}
	}

	// ── カメラインロ：ツイーンでカメラ回転を更新 ──
	if (camIntroActive_) {
		// 60FPS想定の固定デルタ。可変デルタがあるなら dt を使ってOK
		const float delta = 0.016f;

		float yawNow = camYawTween_.Update(delta);

		// 進捗0..1を安全に出す
		float denom = std::max(0.0001f, (camYawEnd_ - camYawStart_));
		float t01 = std::clamp((yawNow - camYawStart_) / denom, 0.0f, 1.0f);

		// ピッチも少しだけ動かしたい場合（固定で良ければ start=end に）
		float pitchNow = MyMath::Lerp(camPitchStart_, camPitchEnd_, t01);

		// カメラの回転を適用（位置は従来のFollowでOK）
		camera->SetRotate({ pitchNow, yawNow, 0.0f });

		if (camYawTween_.Finished()) {
			camIntroActive_ = false;
			camIntroDone_ = true;
			// 念のため最終姿勢を明示
			camera->SetRotate({ camPitchEnd_, camYawEnd_, 0.0f });
		}
	}

	// --- カメラアクションが終わったら、start.png を一度だけ出す ---
	if (camIntroDone_ && !startPlayed_) {
		startPlayed_ = true;          // 二度目以降は発火させない
		startVisible_ = true;
		startSlideIn_ = true;
		startFadeOut_ = false;        // 念のためリセット
		startHoldElapsed_ = 0.0f;
		startAlpha_ = 1.0f;
		startSprite_->SetColor({ 1,1,1,startAlpha_ });
		startSprite_->SetPosition({ startStartPos_.x, startEndPos_.y });
		startTween_.Reset(0.0f, 1.0f, startDuration_, Ease::Type::OutBack);
	}

	// スライドイン
	if (startSlideIn_) {
		startT_ = startTween_.Update(dt);

		// 発光：滑り込み中は PI を1周して明→通常へ
		if (startGlowOn_) {
			float glow = 1.0f + startGlowAmp_ * std::sin(startT_ * MyMath::GetPI());
			startSprite_->SetColor({ glow, glow, glow, startAlpha_ });          // 発光を白成分で乗算
		} else {
			startSprite_->SetColor({ 1,1,1,startAlpha_ });
		}

		float x = MyMath::Lerp(startStartPos_.x, startEndPos_.x, startT_);
		float y = startEndPos_.y;
		startSprite_->SetPosition({ x, y });
		startSprite_->Update();

		if (startTween_.Finished()) {
			startSlideIn_ = false;
			startHoldElapsed_ = 0.0f; // 到着後の静止タイマー開始
		}
	} else if (startVisible_) {

		// 中央での呼吸発光（だんだん弱くなる）
		if (!startFadeOut_ && startGlowOn_) {
			float t01 = (startHoldSec_ > 0.0f) ? std::min(startHoldElapsed_ / startHoldSec_, 1.0f) : 1.0f;
			float decay = 1.0f - 0.7f * t01; // 経過で発光を弱める
			float glow = 1.0f + decay * 0.20f * std::sin(startHoldElapsed_ * startGlowSpeed_);
			startSprite_->SetColor({ glow, glow, glow, startAlpha_ });
		}

		// 到着後：静止→フェードアウト
		if (!startFadeOut_) {
			startHoldElapsed_ += dt;
			if (startHoldElapsed_ >= startHoldSec_) {
				startFadeOut_ = true;
			}
		}
		if (startFadeOut_) {
			startAlpha_ -= dt / startFadeSec_;
			if (startAlpha_ <= 0.0f) {
				startAlpha_ = 0.0f;
				startVisible_ = false; // 完全に消す

				if (!enemiesInitialized_) {
					requestInitEnemies_ = true;   // ←追加！
				}

				gameplayLocked_ = false; // ゲームプレイ解放
			}
			startSprite_->SetColor({ 1,1,1,startAlpha_ });
		}
		startSprite_->Update();
	}

	// その他のオブジェクト・パーティクルの更新
	ParticleManager::GetInstance()->Update();

	for (auto it = bossBullets_.begin(); it != bossBullets_.end(); ) {
		(*it)->Update();
		if ((*it)->IsDead()) it = bossBullets_.erase(it);
		else ++it;
	}

	// Aキーでリボンパーティクルテスト
	if (Input::GetInstance()->TriggerKey(DIK_A)) {
		// プレイヤーのちょい前に出したければこんな感じでもOK
		Vector3 emitPos = { 0.0f, 2.5f, 20.0f };  // Zを少し奥にして見やすく
		ParticleManager::GetInstance()->Emit("ribbonTest", emitPos, 1); // 1本だけ出す
	}


	// ─── プレイヤー死亡時のGameOver遷移 ───
	if (player_ && player_->IsDead()) {
		// 死亡フェーズが始まった瞬間にカウント開始
		if (!playerDeathStarted_) {
			playerDeathStarted_ = true;
			playerDeathElapsed_ = 0.0f;
		} else {
			playerDeathElapsed_ += dt;

			// クルクル（FlyAway）開始から約4秒後にシーン遷移
			if (playerDeathElapsed_ >= 4.0f && !irisClosing_) {
				irisClosing_ = true;
				irisCloseTween_.Reset(0.0f, irisMaxScale_, 0.8f, Ease::Type::InBack);
			}
		}
	}

	// アイリス閉じ中は進行してGameOverへ
	if (irisClosing_) {
		irisCloseScale_ = irisCloseTween_.Update(0.016f);
		iris_->SetSize({ irisCloseScale_, irisCloseScale_ });
		iris_->Update();

		if (irisCloseTween_.Finished()) {
			sceneManager_->SetNextScene(new GameOverScene(dxCommon, srvManager));
			return;
		}
	}

	// ─── Tキーでタイトルに戻る（アイリス閉じ演出つき）───
	if (!irisClosing_ && Input::GetInstance()->TriggerKey(DIK_T)) {
		irisClosing_ = true;
		irisCloseTween_.Reset(0.0f, irisMaxScale_, 0.8f, Ease::Type::InBack);
	}

	if (irisClosing_) {
		irisCloseScale_ = irisCloseTween_.Update(0.016f);
		iris_->SetSize({ irisCloseScale_, irisCloseScale_ });
		iris_->Update();

		if (irisCloseTween_.Finished()) {
			sceneManager_->SetNextScene(new TitleScene(dxCommon, srvManager));
			return;
		}
	}

	// ─── キーボードのYキーでプレイヤーのHPを0にする（デバッグ用）───
	if (Input::GetInstance()->TriggerKey(DIK_Y)) {
		if (player_) player_->SetHP(0);
	}

	if (requestInitEnemies_) {
		InitializeEnemies();
		enemiesInitialized_ = true;
		requestInitEnemies_ = false;
	}

	// パフォーマンス情報・デバッグUI
	UpdatePerformanceInfo();
}

void GameScene::Draw(){
	if (skybox_) skybox_->Draw();

	// 3Dまとめ
	Object3dCommon::GetInstance()->DrawSetCommon();
	//for (auto& g : groundTiles_) g->Draw(dxCommon);
	player_->Draw(dxCommon);
	for (auto& enemy : enemies_) enemy->Draw(dxCommon);
	// クリア演出中はボス関連を描かない
	if (!clearSequence_) {
		if (bossBattle_ && boss_) {
			boss_->Draw(dxCommon);
		}
		for (auto& b : bossBullets_) {
			b->Draw(dxCommon);
		}
	}

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

	// スプライトまとめ
	SpriteCommon::GetInstance()->DrawSetCommon();
	// ---- 最前面の白円は Sprite パスで最後に描く ----
	if (irisOpening_ && iris_) {
		iris_->Draw();
	}

	if (irisClosing_ && iris_) {
		iris_->Draw(); // 閉じる
	}

	if (startVisible_) {
		startSprite_->Draw(); // ゲームスタート文字
	}
}

void GameScene::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame){
	auto b = std::make_unique<BossBullet>();
	b->Initialize(Object3dCommon::GetInstance(), dxCommon, camera.get(), pos, dir, speed, damage, lifeFrame);
	bossBullets_.push_back(std::move(b));
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// ゲーム内のサウンドをロード＆再生する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeAudio(){
	auto* audio = AudioManager::GetInstance();
	audio->Initialize();
	audio->LoadSound("bossP2", "FLASHness.wav"); // ボス戦フェーズ2用BGM ( FLASHness / NEURAY )
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要なテクスチャをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadTextures(){
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
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite(){
	sprite = std::make_unique<Sprite>();
	sprite->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/circle.png");
	sprite->SetPosition({ -1000.0f, 0.0f });
	sprite->SetParentScene(this);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要な3Dモデルをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadModels(){
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("terrain.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("ground.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("jett.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("enemy.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("reticle_big.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("reticle_normal.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("reticle_small.obj", dxCommon);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeObjects(){
	// --- ground: タイルを3枚並べる ---
	groundTiles_.clear();
	const int tileCount = 3;
	for (int i = 0; i < tileCount; ++i) {
		auto g = std::make_unique<Object3d>();
		g->Initialize(Object3dCommon::GetInstance(), dxCommon);
		g->SetModel("ground.obj");
		g->SetParentScene(this);
		g->SetTranslate({ 0.0f, -2.0f,  (float)i * groundTileLen_ }); // 少し下げる
		groundTiles_.push_back(std::move(g));
	}

	// player
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon);
	player_->SetPosition({ 0.0f, 0.0f, 0.0f });
	player_->SetParentScene(this);
	player_->SetEnemy(boss_.get()); // 最初はボスはいないのでnullptr
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを作成し、各オブジェクトに適用する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeCamera(){
	camera = std::make_unique<Camera>();
	camera->SetRotate({ camPitchStart_, camYawStart_, 0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-30.0f });

	ground_ = nullptr; // 既存は使わない（誤参照防止）
	for (auto& g : groundTiles_) {
		g->SetCamera(camera.get());
	}
	player_->SetCamera(camera.get());

	for (auto& enemy : enemies_) {
		enemy->SetCamera(camera.get());
	}
}

void GameScene::ImGuiDebug(){
#ifdef _DEBUG

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("情報");
	ImGui::Text("FPS : %.2f", fps_);
	ImGui::Separator();
	ImGui::Text("フレーム時間 : %.2f ms", frameTimeMs_);
	ImGui::Separator();
	ImGui::Text("DrawCall 回数 : %d", drawCallCount_);
	ImGui::Separator();
	// メモリ使用量取得
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		// WorkingSetSize = 実際にメモリ上に展開されているサイズ
		size_t memoryUsageKB = pmc.WorkingSetSize / 1024; // KB
		size_t memoryUsageMB = memoryUsageKB / 1024; // MB
		ImGui::Text("メモリ使用量 : %zu KB / %zu MB", memoryUsageKB, memoryUsageMB);
	}
	ImGui::Text("MB");
	ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤色
	ImGui::PlotLines(
		"メモリ推移",
		memoryHistory_.data(),
		kMemoryHistorySize,
		memoryHistoryIndex_,
		nullptr,
		0.0f,
		500.0f,
		ImVec2(0, 150)
	);
	ImGui::PopStyleColor();
	ImGui::Separator();
	ImGui::Text("アクティブ Sprite 数 : %d", Sprite::GetActiveCount());
	ImGui::Text("アクティブ Object3D 数 : %d", Object3d::GetActiveCount());
	ImGui::Separator();
	int totalParticles = 0;
	for (const auto& pair : ParticleManager::GetInstance()->GetParticleGroups()) {
		totalParticles += static_cast<int>(pair.second.particles.size());
	}
	ImGui::Text("アクティブ Particles: %d", totalParticles);
	ImGui::Text("パーティクルグループ数: %d", ParticleManager::GetInstance()->GetParticleGroups().size());
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	player_->ImGuiDebug();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	if (boss_) {
		boss_->ImGuiDebug();
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("プレイヤー弾ステータス");
	for (const auto& bullet : player_->GetBullets()) {
		ImGui::Text(bullet->IsHit() ? "true" : "false");
	}
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("地面");
	for (size_t i = 0; i < groundTiles_.size(); ++i) {
		ImGui::PushID(static_cast<int>(i)); // IDを分ける
		Vector3 t = groundTiles_[i]->GetTranslate();
		Vector3 r = groundTiles_[i]->GetRotate();
		Vector3 s = groundTiles_[i]->GetScale();
		if (ImGui::DragFloat3("位置", &t.x, 0.01f)) {
			groundTiles_[i]->SetTranslate(t);
		}
		if (ImGui::DragFloat3("回転", &r.x, 0.01f)) {
			groundTiles_[i]->SetRotate(r);
		}
		if (ImGui::DragFloat3("拡縮", &s.x, 0.01f)) {
			groundTiles_[i]->SetScale(s);
		}
		ImGui::Separator();
		ImGui::PopID();
	}
	ImGui::DragFloat("タイルの長さ", &groundTileLen_, 0.1f, 10.0f, 1000.0f); // 実寸に近い範囲で
	ImGui::DragFloat("スクロール速度", &groundScroll_, 0.01f, -5.0f, 5.0f);
	ImGui::DragFloat("オフセット", &groundOffset_, 0.1f, 0.0f, groundTileLen_ * groundTiles_.size());
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("敵ステータス");
	ImGui::Text("撃破数: %d / %d", defeatedEnemyCount_, maxEnemyCount_);

	for (size_t i = 0; i < enemies_.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		enemies_[i]->ImGuiDebug();
		ImGui::PopID();
	}

	float progress = 0.0f;
	if (maxEnemyCount_ > 0) {
		progress = static_cast<float>(defeatedEnemyCount_) / static_cast<float>(maxEnemyCount_);
	}
	ImGui::ProgressBar(progress, ImVec2(200, 20), "撃破振興数");
	ImGui::Separator();
	// ─────────────────────────────
	// Wave デバッグ用 UI
	// ─────────────────────────────
	// 現在のWavePhaseを表示
	const char* waveLabel = "";
	switch (wavePhase_) {
	case WavePhase::W1:  waveLabel = "Wave1";  break;
	case WavePhase::W2:  waveLabel = "Wave2";  break;
	case WavePhase::W3:  waveLabel = "Wave3";  break;
	case WavePhase::Done: waveLabel = "Bossフェーズ"; break;
	default:             waveLabel = "不明"; break;
	}
	ImGui::Text("現在のWave: %s", waveLabel);

	// 今の雑魚フェーズをスキップ（= 敵を消して、次フレームで GoToNextWave が走る）
	if (ImGui::Button("次のWaveへ")) {
		enemies_.clear();      // 今出ている雑魚を全部消す
		defeatedEnemyCount_ = 0; // カウントはデバッグだしゼロでもOK（お好み）
	}

	// いきなりボス戦に飛ぶ
	if (ImGui::Button("ボスWaveへ")) {
		enemies_.clear();          // 雑魚全削除
		wavePhase_ = WavePhase::Done; // Waveフェーズを「Done」にしてボスフェーズへ
		bossBattle_ = false;       // 念のためリセット
		if (boss_) {
			boss_.reset();         // 既にボスがいたら消す
		}
	}
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("カメラ");
	camera->ImGuiDebug();
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("ゲームパッド");
	auto DrawButtonBar = [](const char* label, bool isPressed, const ImVec4& color) {
		float value = isPressed ? 1.0f : 0.0f;
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
		ImGui::ProgressBar(value, ImVec2(200, 0), label);
		ImGui::PopStyleColor();
		};
	XINPUT_STATE state;
	DWORD result = XInputGetState(0, &state);
	if (result == ERROR_SUCCESS) {
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "接続中！！！");
	} else {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "接続されていません");
	}
	ImGui::Separator();
	DrawButtonBar("A", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A), ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // 緑
	DrawButtonBar("B", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_B), ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤
	DrawButtonBar("X", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_X), ImVec4(0.0f, 0.4f, 1.0f, 1.0f)); // 青
	DrawButtonBar("Y", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_Y), ImVec4(1.0f, 0.4f, 0.7f, 1.0f)); // ピンク
	DrawButtonBar("Start", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_START), ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // グレー
	DrawButtonBar("Back", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_BACK), ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 白
	DrawButtonBar("LB", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER), ImVec4(0.6f, 0.2f, 0.8f, 1.0f)); // 紫
	DrawButtonBar("RB", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER), ImVec4(1.0f, 0.6f, 0.0f, 1.0f)); // オレンジ
	// RT表示（シアン）
	float rtValue = static_cast<float>(Input::GetInstance()->GetRightTrigger()) / 255.0f;
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
	ImGui::ProgressBar(rtValue, ImVec2(200, 0), "RT");
	ImGui::PopStyleColor();
	// LT表示（マゼンタ）
	float ltValue = static_cast<float>(Input::GetInstance()->GetLeftTrigger()) / 255.0f;
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.0f, 1.0f, 1.0f));
	ImGui::ProgressBar(ltValue, ImVec2(200, 0), "LT");
	ImGui::PopStyleColor();
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("スカイボックス");
	ImGui::DragFloat("回転速度 X軸", &skyRotSpeedX_, 0.0001f, -0.02f, 0.02f);
	ImGui::Text("上下の傾き: %.3f rad", skyPitch_);
	ImGui::End();
	////////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // _DEBUG
}

void GameScene::UpdateMemory(){
	/// ───────────────────────────────────────────────
	/// ● 現在のメモリ使用量（MB）を取得し、履歴に記録する
	/// ───────────────────────────────────────────────

	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {

		// ───── 使用中メモリ（MB単位）を計算 ─────
		float memoryUsageMB = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);

		// ───── リングバッファ形式で履歴を更新 ─────
		memoryHistory_[memoryHistoryIndex_] = memoryUsageMB;
		memoryHistoryIndex_ = (memoryHistoryIndex_ + 1) % kMemoryHistorySize; // インデックスを循環
	}
}

void GameScene::UpdateEnemies(){
	/// ───────────────────────────────────────────────
	/// ● 敵の状態を更新し、死亡したものは削除＆カウント
	/// ───────────────────────────────────────────────

	for (auto it = enemies_.begin(); it != enemies_.end(); ) {
		Enemy* e = it->get();      // erase 前に生存中の生ポインタを保持
		e->Update();

		if (e->IsDead()) {
			// 死亡していたら
			player_->OnEnemyDestroyed(e); // プレイヤーに通知

			++defeatedEnemyCount_;          // 倒した数をカウント
			if (defeatedEnemyCount_ == 3) {
				player_->EnableSpecialAttack();
			}

			it = enemies_.erase(it);        // erase でイテレータが無効化されるので注意
		} else {
			++it;
		}
	}
}

void GameScene::UpdateClosestEnemy(){
	/// ───────────────────────────────────────────────
	/// ● プレイヤーに最も近い敵を検出し、ターゲットとして設定する
	/// ───────────────────────────────────────────────

	 // ボス戦中は常にボスをターゲット
	if (bossBattle_ && boss_ && !boss_->IsDead()) {
		player_->SetEnemy(boss_.get());
		//player_->SetAllEnemies(nullptr); // LBの全体攻撃を封じたいなら
		return;
	}

	if (!player_) return; // プレイヤーが未初期化なら処理中止

	Enemy* closestEnemy = nullptr; // 最も近い敵（nullptrで初期化）
	float closestDistance = std::numeric_limits<float>::max(); // 距離の最小値（初期は最大値）
	Vector3 playerPos = player_->GetPosition(); // プレイヤーの現在位置を取得

	// ───── 敵リストを走査して、最も近い生存中の敵を探す ─────
	for (auto& enemy : enemies_) {
		if (!enemy->IsDead()) { // 死んでいない敵に限定
			float dist = MyMath::Length(enemy->GetWorldPosition() - playerPos); // 距離を計算

			// これまでで最も近いなら更新
			if (dist < closestDistance) {
				closestDistance = dist;
				closestEnemy = enemy.get();
			}
		}
	}

	// ───── 検出結果をプレイヤーに通知 ─────
	player_->SetEnemy(closestEnemy);          // 最も近い敵をターゲットとして設定
	player_->SetAllEnemies(&enemies_);        // 全敵リストを共有（全体攻撃などで利用）
}

void GameScene::InitializeEnemies() {
	enemies_.clear();
	defeatedEnemyCount_ = 0;   // ついでに進捗をリセット
	maxEnemyCount_ = 0;        // 全Wave合計で加算していく

	wavePhase_ = WavePhase::W1; // Wave1から
	SpawnCurrentWave();         // 最初のWaveだけ出す（ここでmaxEnemyCount_も加算）

	if (!enemies_.empty()) {
		player_->SetEnemy(enemies_.front().get());
		player_->SetAllEnemies(&enemies_);
	}
}

void GameScene::SpawnCurrentWave() {
	auto camPtr = camera ? camera.get() : nullptr;

	switch (wavePhase_) {
	case WavePhase::W1: {
		// W1: 直進停止（密度で圧）＋HP控えめ
		EnemySpawner::SpawnLine(
			enemies_, 5, /*y*/5.0f, /*z*/60.0f, -20.0f, 10.0f,
			dxCommon, camPtr, this,
			[&](Enemy& e) {
				e.SetBehavior(EnemyBehavior::StraightStop);
				e.SetVelocity({ 0,0,-0.25f });
				e.SetStopZ(60.0f);
				e.SetHP(2);
				e.SetScale({ 1.1f,1.1f,1.1f });
			}
		);
		maxEnemyCount_ += 5;
		break;
	}
	case WavePhase::W2: {
		// W2: サイン蛇行で避けにくく（重なり防止で位相＆停止Zを個体別にオフセット）
		int idx = 0;                 // 個体インデックス（ラムダ内でインクリメント）
		const float phaseStep = 0.7f; // 位相刻み（ラジアン）
		const float stopStep = 0.6f; // 停止Zのズラし量

		EnemySpawner::SpawnV(
			enemies_, 3, /*y*/6.0f, /*z*/80.0f, 0.0f, 8.0f, 6.0f,
			dxCommon, camPtr, this,
			[&](Enemy& e) {
				e.SetBehavior(EnemyBehavior::SineX);
				e.SetVelocity({ 0,0,-0.22f });
				e.SetSineParams(/*ampX*/6.0f, /*freq*/1.6f);

				// ★ここが追加：個体ごとに位相と停止Zを少しずつズラす
				e.SetSinePhase(phaseStep * float(idx));
				e.SetStopZ(60.0f + stopStep * float(idx % 3));

				e.SetHP(3);
				++idx;
			}
		);
		maxEnemyCount_ += 7; // 中央1 + 左右3*2
		break;
	}
	case WavePhase::W3: {
		// W3: 追尾＋左右ストレーフ混在で圧を上げる
		EnemySpawner::SpawnColumn(
			enemies_, 6, /*x*/25.0f, 100.0f, 10.0f, 4.0f, 0.5f,
			dxCommon, camPtr, this,
			[&](Enemy& e) {
				// 交互にパターン変える例
				static int idx = 0;
				if ((idx++ % 2) == 0) {
					e.SetBehavior(EnemyBehavior::ChasePlayer);
					e.SetVelocity({ 0,0,-0.20f });
					e.SetStopZ(34.0f);
					// 追尾用にプレイヤー位置の参照を渡す
					e.SetPlayer([this]() { return player_->GetPosition(); });
					e.SetHP(3);
				} else {
					e.SetBehavior(EnemyBehavior::StrafeLtoR);
					e.SetVelocity({ 0,0,-0.25f });
					e.SetStopZ(60.0f);
					e.SetStrafeX(-18.0f, 18.0f, 0.45f);
					e.SetHP(4);
				}
			}
		);
		maxEnemyCount_ += 6;
		break;
	}
	case WavePhase::Done:
		break;
	}
}

void GameScene::GoToNextWave() {
	if (wavePhase_ == WavePhase::W1) {
		wavePhase_ = WavePhase::W2;
		SpawnCurrentWave();
	} else if (wavePhase_ == WavePhase::W2) {
		wavePhase_ = WavePhase::W3;
		SpawnCurrentWave();
	} else if (wavePhase_ == WavePhase::W3) {
		wavePhase_ = WavePhase::Done; // 最終Waveまで終了
	}

	defeatedEnemyCount_ = 0;
	maxEnemyCount_ = 0;
}

void GameScene::UpdateSkyboxRotationX(){
	constexpr float kTwoPi = 6.2831853f;

	// X軸回転を更新
	skyPitch_ -= skyRotSpeedX_;
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	// Skybox に適用
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });
}

void GameScene::UpdateGroundScroll() {
	const int   N = static_cast<int>(groundTiles_.size());
	if (N == 0) return;

	const float L = groundTileLen_;
	const float speed = groundScroll_;
	const float epsilon = 0.001f; // タイル間にごく小さな隙間を入れてZ-fighting防止

	// 累積オフセット更新
	groundOffset_ += speed;
	const float loop = N * L;
	if (groundOffset_ >= loop) groundOffset_ -= loop;
	if (groundOffset_ < 0.0f)  groundOffset_ += loop;

	// いまどのタイルが先頭か（整数部）と端数（小数部）
	const int   k = static_cast<int>(groundOffset_ / L);
	const float frac = groundOffset_ - static_cast<float>(k) * L;

	// 配置
	for (int j = 0; j < N; ++j) {
		const int idx = (k + j) % N;
		float z = -L + j * L - frac - epsilon * j;

		Vector3 t = groundTiles_[idx]->GetTranslate();
		t.z = z;
		groundTiles_[idx]->SetTranslate(t);
		groundTiles_[idx]->Update();
	}
}

void GameScene::StartClearSequence(){
	clearSequence_ = true;
	clearPhase_ = ClearPhase::CamZoom;
	clearTimer_ = 0.0f;

	// いったん通常ゲームをロックしておく
	gameplayLocked_ = true;

	// --- ボス、ボス弾、レティクルを消し、プレイヤー操作をロック ---
	bossBattle_ = false;            // もうボス戦ではない
	bossBullets_.clear();           // 画面上のボス弾を全部削除
	if (boss_) {
		boss_.reset();              // ボス本体も破棄（描画/更新されなくなる）
	}
	if (player_) {
		player_->SetControlEnabled(false); // 入力を全部無視
		player_->SetReticleVisible(false); // レティクル非表示
	}

	// カメラの開始位置
	clearCamStartPos_ = camera->GetTranslate(); // Camera に Getter あり :contentReference[oaicite:4]{index=4}

	// プレイヤー方向に少し寄せる
	Vector3 camPos = camera->GetTranslate();
	Vector3 playerPos = player_->GetPosition();

	// Zはプレイヤーの少し手前まで寄せる（-30 → プレイヤーZ-15くらい）
	float targetZ = MyMath::Lerp(camPos.z, playerPos.z - 15.0f, 1.0f);
	clearCamTargetPos_ = {
		camPos.x,
		camPos.y + 2.0f, // ちょい上から見下ろす
		targetZ
	};

	// プレイヤーのスタート位置
	clearPlayerStartPos_ = player_->GetPosition();

	// ここで「どこまで飛ぶか」を決めておく（+Z方向に一定距離）
	clearPlayerTargetPos_ = clearPlayerStartPos_; // いったん同じ位置に
	clearPlayerTargetPos_.z += clearPlayerFlyDistance_; // +Z方向に進む

	// 念のためアイリス閉じ状態リセット
	irisClosing_ = false;
}

bool GameScene::UpdateClearSequence(float dt){
	clearTimer_ += dt;

	// skyboxはずっと回し続ける
	UpdateSkyboxRotationX();

	// 花火用タイマー（クリア演出中だけ使うローカル static）
	static float fireTimer = 0.0f;

	switch (clearPhase_) {

	case ClearPhase::CamZoom: // カメラ寄せ
	{
		// 1秒かけて寄る
		float t = std::clamp(clearTimer_ / 1.0f, 0.0f, 1.0f);

		// カメラ位置を線形補間
		Vector3 camPos = MyMath::Vector3Lerp(clearCamStartPos_, clearCamTargetPos_, t);
		camera->SetTranslate(camPos);
		camera->Update();

		if (t >= 1.0f) {
			clearPhase_ = ClearPhase::PlayerFly;
			clearTimer_ = 0.0f;

			// ★プレイヤー飛び始め時に花火タイマーリセット
			fireTimer = 0.0f;
		}
		break;
	}

	case ClearPhase::PlayerFly:
	{
		// カメラは寄った位置で固定
		camera->SetTranslate(clearCamTargetPos_);
		camera->Update();

		// プレイヤーを奥(+Z想定)へ進める
		Vector3 pos = player_->GetPosition();
		pos.z += clearPlayerSpeed_ * dt;
		player_->SetPosition(pos);

		// 入力処理などは行わず、見た目用に行列だけ更新
		player_->UpdateVisualOnly();

		// ============================
		// 花火演出（打ち上げ花火版）
		// ============================

		// ランダム範囲ヘルパー
		auto randRange = [](float min, float max) {
			return min + (max - min) * MyMath::Rand01();
			};

		// 「次の花火が上がるまでの時間」をランダムで決める用
		// （このstaticはこの関数の中で1回だけ初期化されて生き続ける）
		static float fireInterval = randRange(0.8f, 1.6f); // 0.8〜1.6秒のどこか

		fireTimer += dt;

		if (fireTimer >= fireInterval) {
			fireTimer = 0.0f;
			// 次回用に、また別の間隔をランダム決定
			fireInterval = randRange(0.8f, 1.6f);

			// ─────────────────────────────
			// カメラ基準で「画面内っぽい範囲」にランダム配置
			// ─────────────────────────────
			const Matrix4x4 camW = camera->GetWorldMatrix();
			Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
			Vector3 camFwd = MyMath::Normalize({ camW.m[2][0], camW.m[2][1], camW.m[2][2] });
			Vector3 camRight = MyMath::Normalize({ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
			Vector3 camUp = MyMath::Normalize({ camW.m[1][0], camW.m[1][1], camW.m[1][2] });

			// 画面のアスペクト比に合わせた「横：縦」の広がり
			float aspect = static_cast<float>(WindowsAPI::kClientWidth) /
				static_cast<float>(WindowsAPI::kClientHeight);
			const float halfHeight = 25.0f;              // 画面の上下方向の半分くらい（調整ポイント）
			const float halfWidth = halfHeight * aspect; // アスペクト比に合わせた横幅

			// どれくらい「奥」に花火を出すか（カメラ前方方向）
			const float minDepth = 80.0f;   // カメラからの最小距離
			const float maxDepth = 140.0f;  // カメラからの最大距離

			// 一度に何発分の花火を出すか（単発）
			const int kBurstCount = 1;

			for (int i = 0; i < kBurstCount; ++i) {
				// スクリーン座標風の -1.0〜1.0
				float sx = randRange(-1.0f, 1.0f);    // 左右
				float sy = randRange(-0.8f, 0.8f);   // 上下（ちょい上下狭め）

				float depth = randRange(minDepth, maxDepth);

				// カメラ前方 depth の位置を中心に、Right/Up 方向でオフセット
				Vector3 center =
					camPos +
					camFwd * depth +
					camRight * (sx * halfWidth) +
					camUp * (sy * halfHeight);

				SpawnFirework(center);
			}
		}

		// ============================
		// ここまで花火
		// ============================

		// 一定距離進んだらアイリス閉じへ
		if (clearTimer_ >= clearPlayerFlyMinTime_ &&
			pos.z > clearPlayerStartPos_.z + clearPlayerFlyDistance_) {

			clearPhase_ = ClearPhase::IrisClose;
			clearTimer_ = 0.0f;

			irisClosing_ = true;
			irisCloseScale_ = 0.0f;
			irisCloseTween_.Reset(0.0f, irisMaxScale_, 0.8f, Ease::Type::InBack);
		}
		break;
	}

	case ClearPhase::IrisClose: // アイリス閉じ
	{
		if (irisClosing_ && iris_) {
			irisCloseScale_ = irisCloseTween_.Update(dt);
			iris_->SetSize({ irisCloseScale_, irisCloseScale_ });
			iris_->Update();

			if (irisCloseTween_.Finished()) {
				// クリア演出完了 → true を返す
				return true;
			}
		}
		break;
	}

	default:
		break;
	}

	// パーティクルは普通に動かす
	ParticleManager::GetInstance()->Update();

	return false; // まだ演出継続中
}

void GameScene::SpawnFirework(const Vector3& center){
	auto pm = ParticleManager::GetInstance();

	// =========================
	// 1. 打ち上がる光の筋
	// =========================
	{
		Vector3 launchPos = center;   // 非 const のコピーを作る
		launchPos.y -= 40.0f;        // 少し下から飛ばす
		pm->Emit("fw_launch", launchPos, 1);
	}

	// =========================
	// 2. 爆発フラッシュ
	// =========================
	{
		Vector3 flashPos = center;
		pm->Emit("fw_flash", flashPos, 1);
	}

	// =========================
	// 3. 花火本体（放射）
	// =========================
	{
		Vector3 burstPos = center;
		pm->Emit("fw_burst", burstPos, 60);   // 本体の粒の数はお好みで
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
		const Matrix4x4 camW = camera->GetWorldMatrix();
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