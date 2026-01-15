#define NOMINMAX
#include "GameScene.h"
#include <limits>
#include <algorithm>
#include "MyMath.h"
#include <psapi.h>
#include <Input.h>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using namespace TKM;

void GameScene::Initialize() {
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

	// ──────────────── ラインレンダラーの初期化 ───────────────
	LineRenderer::GetInstance()->Initialize(dxCommon);

	// ──────────────── パーティクルの初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon, srvManager, camera.get());
	ParticleManager::GetInstance()->CreateParticleGroup("uv", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	particleEmitter = std::make_unique<ParticleEmitter>();
	particleEmitter->Initialize("uv", { 0.0f,2.5f,10.0f });

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
	skybox_->Initialize(dxCommon, srvManager, "resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera.get());
	// ──────────────── 敵マネージャの初期化 ───────────────
	enemyManager_ = std::make_unique<EnemyManager>();
	enemyManager_->Initialize(dxCommon, camera.get(), this, player_.get());
	enemyManager_->BindEnemies(&enemies_, &defeatedEnemyCount_, &maxEnemyCount_);
	// ──────────────── ボスマネージャの初期化 ───────────────
	if (!bossManager_) {
		bossManager_ = std::make_unique<BossManager>();
	}
	bossManager_->Initialize(dxCommon, camera.get(), this, player_.get());
	// ──────────────── 画面エフェクトの初期化 ───────────────
	// RadialBlurEffect の生成と初期化
	radialBlur_ = std::make_unique<TKM::RadialBlurEffect>();
	radialBlur_->Initialize(dxCommon);
	// DirectX 側に「このシーンの RadialBlurEffect」を登録
	dxCommon->SetRadialBlurEffect(radialBlur_.get());
	if (player_) {
		// プレイヤーから LT 発射時に通知してもらう
		player_->SetRadialBlurEffect(radialBlur_.get());
	}
	// VignettingEffect の生成と初期化
	vignetting_ = std::make_unique<TKM::VignettingEffect>();
	vignetting_->Initialize(dxCommon);
	// FogEffect の生成と初期化 ＆ 常時ON
	fog_ = std::make_unique<TKM::FogEffect>();
	fog_->Initialize(dxCommon);
	fog_->SetActive(false);                // ゲームシーン中はずっと有効にしたい
	dxCommon->SetFogEffect(fog_.get());   // DirectXCommon に登録
	// AuraEffect の生成と初期化
	aura_ = std::make_unique<TKM::AuraEffect>();
	aura_->Initialize(dxCommon);
	dxCommon->SetAuraEffect(aura_.get());
	// WaterRippleEffect の生成と初期化
	waterRipple_ = std::make_unique<TKM::WaterRippleEffect>();
	waterRipple_->Initialize(dxCommon); // 波紋エフェクトの初期化
	dxCommon->SetWaterRippleEffect(waterRipple_.get()); // DirectXCommon に登録
	bossManager_->SetWaterRippleEffect(waterRipple_.get()); // BossManager にも登録
	// FogVolume3D の生成と初期化
	fogVolume3D_ = std::make_unique<TKM::FogVolume3D>();
	fogVolume3D_->Initialize(dxCommon);
	// 初期パラメータ例
	auto& d = fogVolume3D_->GetDesc();
	d.centerWS = { 0.0f, 6.0f, 20.0f };
	d.halfSizeWS = { 900.0f, 220.0f, 900.0f };
	d.sliceCount = 80;     // まずこれくらいで板感減らす
	d.density = 0.19f;  // 濃すぎなら 0.015f まで落としてOK
	// SmokeVolume3D の生成と初期化
	smokeVolume3D_ = std::make_unique<TKM::SmokeVolume3D>();
	smokeVolume3D_->Initialize(dxCommon);
	// ──────────────── タイムスケールコントローラーの初期化 ───────────────
	timeScale_.Initialize();
	bossManager_->SetTimeScaleController(&timeScale_);
	clearSlowRequested_ = false; // クリアスロー未要求状態で開始
}

void GameScene::Finalize() {
	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();
	// 終了処理
	AudioManager::GetInstance()->Finalize();
	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();

	// ポストエフェクトの解除
	if (dxCommon) {
		dxCommon->SetRadialBlurEffect(nullptr); // RadialBlurEffect の解除
		dxCommon->SetVignettingEffect(nullptr); // VignettingEffect の解除
		dxCommon->SetFogEffect(nullptr); // FogEffect の解除
		dxCommon->SetAuraEffect(nullptr); // AuraEffect の解除
	}
}

void GameScene::Update() {
	// 入力処理
	Input::GetInstance()->Update();
	// 毎フレームの最初に、前フレームのラインをクリア
	LineRenderer::GetInstance()->BeginFrame();
	// フレームタイム計測
	const float rawDt = dt; /// デフォルトデルタタイム（補間なし）
	timeScale_.Update(rawDt); // タイムスケールコントローラーの更新
	const float scaledDt = rawDt * timeScale_.GetScale(); /// スローデルタタイム

	// 描画コール・メモリの初期化
	ResetDrawCallCount();
	UpdateMemory();

	// クリア演出中なら専用処理だけ回して終わり
	if (clearSequence_) {
		bool finished = UpdateClearSequence(scaledDt); // クリア演出シーケンスの更新
		// 終了したらシーン切り替え
		if (finished) {
			sceneManager_->SetNextScene(new GameClearScene(dxCommon, srvManager));
			return;
		}
		UpdatePerformanceInfo();
		return;
	}

	// --- 敵とWaveは「ゲーム開始後」だけ動かす ---
	if (!gameplayLocked_ && enemiesInitialized_) {

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
						if (!clearSequence_) {
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
		// ボスマネージャの更新
		if (bossManager_) {
			bossManager_->Update(scaledDt);
		}

		// 画面エフェクトの更新=================================
		if (radialBlur_) {
			radialBlur_->Update(scaledDt); // ラジアルブラーの更新
		}
		if (vignetting_) {
			// ボス戦中かどうかを BossManager から聞いてフラグを渡す
			bool bossWave =
				bossManager_ &&
				bossManager_->IsBattleActive() &&    // 戦闘中フラグ
				!bossManager_->IsBossDead();         // すでに死んでないか

			vignetting_->SetBossWave(bossWave);

			vignetting_->Update(scaledDt); // ビネット更新（ボス戦中ならフェードIN、終わったらOUT）
		}
		if (fog_) {
			fog_->Update(scaledDt); // フォグの更新
		}
		if (waterRipple_) {
			waterRipple_->Update(scaledDt); // 波紋の更新（スローに合わせてゆっくり進む）
		}
		if (fogVolume3D_) {
			fogVolume3D_->Update(scaledDt);
		}
		if (smokeVolume3D_) {
			smokeVolume3D_->Update(scaledDt);
		}
		// ==================================================

		// ライトの更新
		directionalLight_->Update();

		// ゲームプレイ中だけ風エフェクト
		if (!clearSequence_ && !gameplayLocked_) {
			UpdateAirStreak(dt);
		}

		if (irisOpening_) { // --- アイリスオープニング中の更新 ---
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
			if (emitFireworkPending_ && emitOpenElapsed_ >= emitFireworkDelaySec_) {
				emitFireworkPending_ = false;
				// 同じ位置で出す
				ParticleManager::GetInstance()->Emit("irisFire", lastEmitPos_, 80);
			}

			// ── アイリスの見た目更新（従来どおり） ──
			irisScale_ = irisTween_.Update(0.016f);
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
			// 60FPS想定の固定デルタ
			const float delta = 0.016f;
			// ツイーン更新で現在のヨー回転を取得
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
				camera->SetRotate({ camPitchEnd_, camYawEnd_, 0.0f }); // 念のため最終値セット
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
						requestInitEnemies_ = true;
					}

					gameplayLocked_ = false; // ゲームプレイ解放
				}
				startSprite_->SetColor({ 1,1,1,startAlpha_ });
			}
			startSprite_->Update();
		}

		// --- 操作ガイドUI：押してる時は赤 ---
		Input* in = Input::GetInstance();

		// RB / LB
		bool rbDown = in->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER);
		bool lbDown = in->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER);

		// LT（アナログ）
		bool ltDown = (in->GetLeftTrigger() > 30); // 30は好みで

		// 通常色 / 押下色
		const Vector4 idle = { 1.0f, 1.0f, 1.0f, 0.75f }; // 白（薄め）
		const Vector4 on = { 1.0f, 0.25f, 0.25f, 1.0f }; // 赤（ハッキリ）

		if (uiRB_) uiRB_->SetColor(rbDown ? on : idle);
		if (uiLB_) uiLB_->SetColor(lbDown ? on : idle);
		if (uiLT_) uiLT_->SetColor(ltDown ? on : idle);

		if (uiLT_) { uiLT_->Update(); }
		if (uiLB_) { uiLB_->Update(); }
		if (uiRB_) { uiRB_->Update(); }


		// その他のオブジェクト・パーティクルの更新
		ParticleManager::GetInstance()->Update(scaledDt);

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
					irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDurationSec, Ease::Type::InBack);
				}
			}
		}

		// アイリス閉じ中は進行してGameOverへ
		if (irisClosing_) {
			irisScale_ = UpdateIrisScale(iris_.get(), irisTween_, 0.016f);

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
	player_->Draw(dxCommon); // プレイヤーの描画

	if (enemyManager_) {
		enemyManager_->Draw(dxCommon); // 敵群の描画を EnemyManager に委譲
	}

	// クリア演出中はボス関連を描かない
	if (!clearSequence_) {
		if (bossManager_) {
			bossManager_->Draw(dxCommon);
		}
	}

	// FogVolume（空間霧）
	if (fogVolume3D_) {
		TKM::Camera* activeCamera = (useDebugCamera_ && debugCamera_) ? (TKM::Camera*)debugCamera_.get() : camera.get();
		if (activeCamera) {
			const Matrix4x4& camW = activeCamera->GetWorldMatrix();

			// ※あなたの行列の取り方（translationが m[3] なので、基底は row0/1/2 と仮定）
			Vector3 right{ camW.m[0][0], camW.m[0][1], camW.m[0][2] };
			Vector3 up{ camW.m[1][0], camW.m[1][1], camW.m[1][2] };
			Vector3 fwd{ camW.m[2][0], camW.m[2][1], camW.m[2][2] };

			Matrix4x4 vp = activeCamera->GetViewProjectionMatrix();
			fogVolume3D_->Draw(vp, right, up, fwd);
		}
	}
	// SmokeVolume（空間スモーク）
	if (smokeVolume3D_) {
		TKM::Camera* activeCamera = (useDebugCamera_ && debugCamera_) ? (TKM::Camera*)debugCamera_.get() : camera.get();
		if (activeCamera) {
			const Matrix4x4& camW = activeCamera->GetWorldMatrix();

			Vector3 right{ camW.m[0][0], camW.m[0][1], camW.m[0][2] };
			Vector3 up{ camW.m[1][0], camW.m[1][1], camW.m[1][2] };
			Vector3 fwd{ camW.m[2][0], camW.m[2][1], camW.m[2][2] };

			Matrix4x4 vp = activeCamera->GetViewProjectionMatrix();
			smokeVolume3D_->Draw(vp, right, up, fwd);
		}
	}

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

#ifdef USE_IMGUI
	// ライン描画
	Matrix4x4 vp;
	if (useDebugCamera_ && debugCamera_) {
		vp = debugCamera_->GetViewProjectionMatrix();
	} else {
		vp = camera->GetViewProjectionMatrix();
	}
	LineRenderer::GetInstance()->Draw(vp);
#endif

	// スプライトまとめ
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	// ---- 最前面の白円は Sprite パスで最後に描く ----
	if (irisOpening_ && iris_) {
		iris_->Draw(); // 開く
	}
	if (irisClosing_ && iris_) {
		iris_->Draw(); // 閉じる
	}
	if (startVisible_) {
		startSprite_->Draw(); // ゲームスタート文字
	}
	// 操作ガイドUI（常時表示）
	if (uiLT_) { uiLT_->Draw(); }
	if (uiLB_) { uiLB_->Draw(); }
	if (uiRB_) { uiRB_->Draw(); }

}

void GameScene::SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame) {
	if (bossManager_) {
		bossManager_->SpawnEnemyBullet(pos, dir, speed, damage, lifeFrame);
	}
}

TKM::Camera* GameScene::UpdateActiveCamera() {
	// ──────────────── アクティブカメラの決定＆更新 ───────────────
	TKM::Camera* activeCamera = camera.get();
	if (useDebugCamera_ && debugCamera_) {
		// デバッグカメラを更新
		debugCamera_->Update();
		activeCamera = debugCamera_.get();
	} else {
		// 通常カメラを更新
		camera->Update();
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

	// フォグエフェクト用にカメラ位置をセット
	if (fog_ && activeCamera) {
		const Matrix4x4& camW = activeCamera->GetWorldMatrix();
		Vector3 camPos{
			camW.m[3][0],
			camW.m[3][1],
			camW.m[3][2]
		};
		fog_->SetWorldPos(camPos);
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
	iris_ = CreateCenteredIrisSprite(dxCommon, irisMaxScale_);
	irisScale_ = irisMaxScale_;
	irisTween_.Reset(irisMaxScale_, 0.0f, kIrisDurationSec, Ease::Type::OutBack);

	// タイトル戻り用アイリス
	irisCloseScale_ = 0.0f;
	irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDurationSec, Ease::Type::InBack);

	// ゲームスタート文字
	startSprite_ = std::make_unique<Sprite>();
	startSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, "./resources/start.png");
	startSprite_->SetAnchorPoint({ 0.5f, 0.5f }); // 中央基準
	startSprite_->SetPosition({ startStartPos_.x, startStartPos_.y });
	startSprite_->SetSize({ 100, 100 }); // 画像サイズに合わせ調整
	startSprite_->SetColor({ 1,1,1,1 }); // アルファ1で開始
	startTween_.Reset(0.0f, 1.0f, startDuration_, Ease::Type::OutBack);

	// ---- 操作ガイドUI（右下） ----
	uiLT_ = std::make_unique<Sprite>();
	uiLB_ = std::make_unique<Sprite>();
	uiRB_ = std::make_unique<Sprite>();

	uiLT_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, "./resources/LT.png");
	uiLB_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, "./resources/LB.png");
	uiRB_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, "./resources/RB.png");

	// 右下基準（右下にピタッと寄せる）
	uiLT_->SetAnchorPoint({ 1.0f, 1.0f });
	uiLB_->SetAnchorPoint({ 1.0f, 1.0f });
	uiRB_->SetAnchorPoint({ 1.0f, 1.0f });

	// 画像でかいのでUI用に縮小（好みで調整）
	const Vector2 uiSize = { 260.0f, 150.0f };
	uiLT_->SetSize(uiSize);
	uiLB_->SetSize(uiSize);
	uiRB_->SetSize(uiSize);

	uiLT_->SetColor({ 1,1,1,0.85f });
	uiLB_->SetColor({ 1,1,1,0.85f });
	uiRB_->SetColor({ 1,1,1,0.85f });

	// 右下に積む（RBが一番下）
	const float w = (float)WindowsAPI::kClientWidth;
	const float h = (float)WindowsAPI::kClientHeight;
	const float margin = 20.0f;
	const float spacing = 10.0f;

	uiRB_->SetPosition({ w - margin, h - margin });
	uiLB_->SetPosition({ w - margin, h - margin - (uiSize.y + spacing) * 1.0f });
	uiLT_->SetPosition({ w - margin, h - margin - (uiSize.y + spacing) * 2.0f });

}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要な3Dモデルをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadModels() {
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
void GameScene::InitializeObjects() {
	// ──────────────── プレイヤーの初期化 ───────────────
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon);
	player_->SetPosition({ 0.0f, 0.0f, 0.0f });
	player_->SetParentScene(this);
	player_->SetEnemy(nullptr); // 最初はボスはいないのでnullptr
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを作成し、各オブジェクトに適用する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeCamera() {
	camera = std::make_unique<TKM::Camera>();
	camera->SetRotate({ camPitchStart_, camYawStart_, 0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-30.0f });

	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize(
		camera->GetTranslate(),        // 開始位置
		Vector3{ 0.0f, 0.0f, 0.0f }    // 初期座標
	);

	player_->SetCamera(camera.get()); // プレイヤーにカメラをセット
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
	camera->ImGuiDebug(); // カメラのデバッグ表示

	ImGui::Begin("デバッグカメラ");
	ImGui::Checkbox("オン/オフ", &useDebugCamera_);
	ImGui::Text("DebugCam: RMB rotate, LMB/Z, MMB/Y");
	ImGui::End();
	/////////////////////////////////////////////////////
	skybox_->ImGuiUpdate(); // スカイボックスのデバッグ表示
	/////////////////////////////////////////////////////
	// Fog のデバッグ
	if (fog_) {
		fog_->ImGuiDebug();
	}
	if (aura_) {
		aura_->ImGuiDebug();
	}
	if (fogVolume3D_) {
		fogVolume3D_->ImGuiDebug();
	}
	if (smokeVolume3D_) {
		smokeVolume3D_->ImGuiDebug();
	}
	/////////////////////////////////////////////////////
	ImGuiDebugGamepad(); // ゲームパッド入力デバッグ
	ImGuiDebugInfo(); // パフォーマンス情報デバッグ
	/////////////////////////////////////////////////////
#endif
}

void GameScene::StartClearSequence() {
	clearSequence_ = true;
	clearPhase_ = ClearPhase::CamZoom;
	clearTimer_ = 0.0f;

	// いったん通常ゲームをロックしておく
	gameplayLocked_ = true;

	// --- ボス、ボス弾、レティクルを消し、プレイヤー操作をロック ---
	if (bossManager_) {
		bossManager_->OnClearSequenceStart();
	}
	if (player_) {
		player_->SetControlEnabled(false); // 入力を全部無視
		player_->SetReticleVisible(false); // レティクル非表示
	}
	// ビネットを強制OFF（この後の演出では使わない）
	if (vignetting_) {
		vignetting_->SetBossWave(false);      // 内部フラグをOFF
	}
	if (dxCommon) {
		// DX 側からも登録解除して、ポストエフェクトチェーンから外す
		dxCommon->SetVignettingEffect(nullptr);
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

	// 念のためアイリス閉じ状態リセット
	irisClosing_ = false;
}

bool GameScene::UpdateClearSequence(float dt) {
	clearTimer_ += dt;

	// skyboxはずっと回し続ける
	if (skybox_) {
		skybox_->UpdateRotation(); // skybox回転更新
	}

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
			// プレイヤー飛び始め時に花火タイマーリセット
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
		static float fireInterval = randRange(0.8f, 1.6f); // 0.8〜1.6秒のどこか
		fireTimer += dt; // 経過時間を加算

		if (fireTimer >= fireInterval) { // 花火打ち上げタイミング到来
			fireTimer = 0.0f; // タイマーリセット
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

			for (int i = 0; i < kBurstCount; ++i) { // 複数発分ループ
				// スクリーン座標風の -1.0〜1.0
				float sx = randRange(-1.0f, 1.0f);    // 左右
				float sy = randRange(-0.8f, 0.8f);   // 上下（ちょい上下狭め）

				float depth = randRange(minDepth, maxDepth);

				// カメラ前方 depth の位置を中心に、Right/Up 方向でオフセット
				Vector3 center = camPos + camFwd * depth + camRight * (sx * halfWidth) + camUp * (sy * halfHeight);

				SpawnFirework(center); // 花火発生関数を呼ぶ
			}
		}

		// 一定距離進んだらアイリス閉じへ
		if (clearTimer_ >= clearPlayerFlyMinTime_ &&
			pos.z > clearPlayerStartPos_.z + clearPlayerFlyDistance_) {

			clearPhase_ = ClearPhase::IrisClose;
			clearTimer_ = 0.0f;

			irisClosing_ = true;
			irisCloseScale_ = 0.0f;
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDurationSec, Ease::Type::InBack);
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

	// フレームタイム計測
	const float rawDt = dt; /// デフォルトデルタタイム（補間なし）
	timeScale_.Update(rawDt); // タイムスケールコントローラーの更新
	const float scaledDt = rawDt * timeScale_.GetScale(); /// スローデルタタイム
	// パーティクルは普通に動かす
	ParticleManager::GetInstance()->Update(scaledDt);

	return false; // まだ演出継続中
}

void GameScene::SpawnFirework(const Vector3& center) {
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
		pm->Emit("fw_burst", burstPos, kFireworkBurstCount); // 一度に複数個出す
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