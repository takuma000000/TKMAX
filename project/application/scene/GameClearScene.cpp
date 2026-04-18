#include "GameClearScene.h"
#include "Input.h"
#include "TitleScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "AudioCatalog.h"
#include <cmath>
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "ClearComedyStates.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using namespace TKM;

static void EmitTitleExplodeLike_(const Vector3& pos) {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) { return; }

	pm->Emit("titleExplode_core", pos, 28);
	pm->Emit("titleExplode_rays", pos, 140);
	pm->Emit("titleExplode_debris", pos, 90);
	pm->Emit("titleExplode_ring", pos, 2);
}

void GameClearScene::Initialize() {
	// ─────────────────────
	// 音声読み込み
	// ─────────────────────
	AudioCatalog::LoadResultAudios();

	// ─────────────────────
	// モデル・テクスチャ読み込み
	// ─────────────────────
	ModelCatalog::LoadModelCatalogs(dxCommon_); // モデルカタログのロード
	TextureCatalog::LoadTextureCatalogs(); // テクスチャカタログのロード

	// ─────────────────────
	// カメラ
	// ─────────────────────
	camera_ = std::make_unique<TKM::Camera>();
	camera_->SetRotate(cameraStartRot_);
	camera_->SetTranslate(cameraStartPos_);
	camera_->Update();

	cameraMoveTime_ = 0.0f;

	// ─────────────────────
	// パーティクルグループ
	// ─────────────────────
	ParticleManager::GetInstance()->ClearAllGroups();
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());
	// パーティクルグループの登録は ParticleGroupsCatalogクラス へ
	ParticleGroupsCatalog::RegisterScene(ParticleManager::GetInstance());

	// ─────────────────────
	// ライト
	// ─────────────────────
	dirLight_ = std::make_unique<DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	// ─────────────────────
	// スカイボックス
	// ─────────────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());

	// ─────────────────────
	// 自機（ジェットコースター演出）
	// ─────────────────────
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	player_->SetCamera(camera_.get());
	player_->SetControlEnabled(false);  // 入力&通常ゲーム処理を全部止める
	player_->SetReticleVisible(false);  // レティクルは要らないので非表示
	player_->SetEnableJetSmoke(true); // ジェットスモークは出す？出さない？
	player_->SetPosition(playerDisplayPos_); // クリア画面での表示位置
	player_->SetRotation(playerDisplayRot_); // クリア画面での表示回転
	// 時間リセット
	planeTime_ = 0.0f;

	// ─────────────────────
	// 「GAME CLEAR」スプライト
	// ─────────────────────
	clearSprite_ = std::make_unique<Sprite>();
	clearSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/clear.png");
	clearSprite_->SetAnchorPoint({ 0.5f, 0.5f }); // 中心を基準にする
	clearSprite_->SetPosition(clearSpriteStartPos_); // 演出開始位置
	clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 常に表示できる状態

	// ─────────────────────
	// 画面遷移アイリス（他シーンと同じ仕様）
	// ─────────────────────
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMaxScale_);

	// 入場は「覆った状態 → 0」へ（OutBack, 0.8s）
	irisScale_ = irisMaxScale_;
	iris_->SetSize({ irisScale_, irisScale_ });
	irisOpenTween_.Reset(
		/*start*/ irisMaxScale_,
		/*end*/   0.0f,
		/*sec*/   0.8f,
		Ease::Type::OutBack
	);
	irisOpening_ = true;
	irisClosing_ = false;

	// ─────────────────────
	clearMenu_ = std::make_unique<GameResultMenuController>();
	clearMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		WindowsAPI::kClientWidth_,
		WindowsAPI::kClientHeight_
	);

	// ─────────────────────
	// 決定時の水面波紋エフェクト
	// ─────────────────────
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();
	rippleEffect_->Initialize(dxCommon_);
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());

	// ─────────────────────
	// ポストエフェクト
	// ─────────────────────
	postFx_ = std::make_unique<TKM::PostEffectController>();
	postFx_->Initialize(dxCommon_, player_.get(), nullptr);
	// クリアシーンでは、放射ブラーを手動でONにしておく（決定時の一瞬だけ出す）
	blurReleased_ = false;

	// ─────────────────────
	// コミカル逃走演出用の状態管理クラス初期化
	// ─────────────────────
	clearComedyFallSlowRequested_ = false; // 転ぶ瞬間スロー未実行
	clearComedyTimeScale_.Initialize(); // タイムスケール初期化
	SetupClearComedyBossConfig_(); // コミカル逃走演出用のボス設定を行う
	clearComedySM_.Initialize(this);
}

void GameClearScene::Finalize() {
	dxCommon_->SetWaterRippleEffect(nullptr); // DirectXCommonから波紋エフェクトの参照を外す（安全のため）
	postFx_->Finalize(); // ポストエフェクトの終了処理（全シーン共通）
	AudioManager::GetInstance()->Finalize(); // オーディオマネージャの終了処理（全シーン共通）
}

void GameClearScene::Update() {
	Input::GetInstance()->Update();

	// ─────────────────────
	// 水面波紋エフェクト更新（決定時に呼ばれる）
	// ─────────────────────
	rippleEffect_->Update(dt_);

	// ─────────────────────
	// アイリス開き（入場）
	// ─────────────────────
	if (irisOpening_) {
		irisScale_ = UpdateIrisScale(iris_.get(), irisOpenTween_, dt_);
		if (irisOpenTween_.Finished()) {
			irisOpening_ = false;
		}
	}

	// ─────────────────────
	// クリアメニュー（リスタート/タイトル）
	// ─────────────────────
	if (!irisClosing_ && !irisOpening_ && isClearMenuVisible_) {
		const auto cmd = clearMenu_->Update(dt_);

		if (cmd == GameResultMenuController::Command::Restart) {
			nextAction_ = NextAction::Restart;
			irisClosing_ = true;
			irisCloseTween_.Reset(/*start*/ 0.0f, /*end*/ irisMaxScale_, /*sec*/ 0.8f, Ease::Type::InBack);
		} else if (cmd == GameResultMenuController::Command::ReturnToTitle) {
			nextAction_ = NextAction::ReturnToTitle;
			irisClosing_ = true;
			irisCloseTween_.Reset(/*start*/ 0.0f, /*end*/ irisMaxScale_, /*sec*/ 0.8f, Ease::Type::InBack);
		}
	}

	// アイリス閉じ中は、閉じ演出の更新と終了判定のみ行う
	if (irisClosing_) {
		irisScale_ = UpdateIrisScale(iris_.get(), irisCloseTween_, dt_); // 閉じ演出更新
		// 閉じ演出が終わったら、次のアクションへ
		if (irisCloseTween_.Finished()) {
			// 演出が終わったら、次のアクションへ
			if (nextAction_ == NextAction::Restart) {
				sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_)); // リスタート
				return;
			}
			// デフォルトはタイトル
			sceneManager_->SetNextScene(std::make_unique<TitleScene>(dxCommon_, srvManager_)); // タイトルへ
			return;
		}
	}

	// ─────────────────────
	// カメラ
	// ─────────────────────
	if (enableCameraIntro_) {
		cameraMoveTime_ += dt_;

		float t = cameraMoveTime_ / cameraMoveDuration_;
		t = std::clamp(t, 0.0f, 1.0f);

		// OutBackで一度終点に到達 → その後オーバーシュート
		float posT = Ease::Eval(cameraPosEaseType_, t);
		float rotT = Ease::Eval(cameraRotEaseType_, t);

		Vector3 camPos = MyMath::Vector3Lerp(cameraStartPos_, cameraEndPos_, posT);
		Vector3 camRot = MyMath::Vector3Lerp(cameraStartRot_, cameraEndRot_, rotT);

		camera_->SetTranslate(camPos);
		camera_->SetRotate(camRot);

		// ─────────────────────
		// クリアスプライト表示OFF
		// 「カメラが動いている間は」非表示
		// ─────────────────────
		isClearSpriteVisible_ = false;

		// ─────────────────────
		// ブラー制御
		// 「終点に一度到達するまで」ずっとON
		// posT が 1.0f に達した瞬間に解除
		// ─────────────────────
		if (postFx_) {
			if (!blurReleased_) {
				if (posT < 1.0f) {
					postFx_->SetRadialBlurManual(true, cameraBlurStrength_);
				} else {
					postFx_->SetRadialBlurManual(false, 0.0f);
					blurReleased_ = true;
				}
			} else {
				postFx_->SetRadialBlurManual(false, 0.0f);
			}
		}

		// ─────────────────────
		// クリア祝福パーティクル
		// カメラが寄ってくる間、祝福の光をド派手に弾けさせる
		// ─────────────────────
		{
			auto frand = [](float a, float b) {
				return a + (b - a) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
				};

			// 中盤が最高潮になる山
			float peak = std::sin(t * MyMath::GetPI());
			peak = std::clamp(peak, 0.0f, 1.0f);

			// 祝福の中心位置
			Vector3 celebrateCenter = playerDisplayPos_ + clearCelebrateOffset_ + clearParticleGlobalOffset_;

			celebrateCoreTimer_ += dt_;
			celebrateSparkTimer_ += dt_;
			celebrateRayTimer_ += dt_;

			float coreInterval = MyMath::Lerp(0.28f, 0.10f, peak);
			float sparkInterval = MyMath::Lerp(0.08f, 0.015f, peak);
			float rayInterval = MyMath::Lerp(0.22f, 0.07f, peak);

			if (celebrateCoreTimer_ >= coreInterval) {
				celebrateCoreTimer_ = 0.0f;

				Vector3 p = celebrateCenter + Vector3{
					frand(-8.0f, 8.0f),
					frand(-2.0f, 6.0f),
					frand(-6.0f, 6.0f)
				};

				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_core", p, 1);
			}

			if (celebrateSparkTimer_ >= sparkInterval) {
				celebrateSparkTimer_ = 0.0f;

				Vector3 p = celebrateCenter + Vector3{
					frand(-16.0f, 16.0f),
					frand(-6.0f, 10.0f),
					frand(-12.0f, 12.0f)
				};

				int count = static_cast<int>(MyMath::Lerp(8.0f, 22.0f, peak));
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_spark", p, count);

				// タイトルの敵消滅っぽい爆発も混ぜる
				float explodeChance = MyMath::Lerp(0.03f, 0.10f, peak);
				if (frand(0.0f, 1.0f) < explodeChance) {
					EmitTitleExplodeLike_(p);
				}
			}

			if (celebrateRayTimer_ >= rayInterval) {
				celebrateRayTimer_ = 0.0f;

				Vector3 p = celebrateCenter + Vector3{
					frand(-12.0f, 12.0f),
					frand(-4.0f, 8.0f),
					frand(-10.0f, 10.0f)
				};

				int count = static_cast<int>(MyMath::Lerp(1.0f, 3.0f, peak));
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_ray", p, count);
			}

			// 一度終点に到達した瞬間だけ、最大祝福バースト
			if (!celebrateFinalBurstDone_ && posT >= 1.0f) {
				celebrateFinalBurstDone_ = true;

				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_core", celebrateCenter, 10);
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_spark", celebrateCenter, 70);
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_ray", celebrateCenter, 16);

				EmitTitleExplodeLike_(celebrateCenter);
			}
		}

		// ─────────────────────
		// カメラ演出終了判定
		// 「最後まで到達したら」通常状態へ
		// ─────────────────────
		if (t >= 1.0f) {
			enableCameraIntro_ = false; // カメラ演出終了

			// ここではまだ GAME CLEAR を出さない
			isClearSpriteVisible_ = false;
			isClearSpritePopPlaying_ = false;
			clearSpritePopTime_ = 0.0f;
			clearSprite_->SetPosition(clearSpriteStartPos_);
			clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			isClearMenuVisible_ = false;

			// コミカル逃走演出の待機状態へ
			if (clearComedySM_.GetState() == nullptr) {
				clearComedySM_.Change(std::make_unique<ClearComedyWaitAfterClearState>());
			}
		}
	} else {
		camera_->SetTranslate(cameraEndPos_); // カメラ位置
		camera_->SetRotate(cameraEndRot_); // カメラ回転

		// ブラーは常にOFF
		if (postFx_) {
			postFx_->SetRadialBlurManual(false, 0.0f);
		}
	}
	camera_->Update();
	ParticleManager::GetInstance()->SetCamera(camera_.get());

	// ─────────────────────
	// ライト更新
	// ─────────────────────
	dirLight_->Update();

	// ─────────────────────
	// ポストエフェクト更新
	// ─────────────────────
	postFx_->Update(dt_, nullptr);

	// ─────────────────────
	// パーティクル更新
	// ─────────────────────
	TKM::ParticleManager::GetInstance()->Update(dt_);

	// ─────────────────────
	// デバッグ用：GAME CLEARバースト常時発生
	// ─────────────────────
	if (debugEmitClearBannerBurst_) {
		debugEmitClearBannerBurstTimer_ += dt_;

		if (debugEmitClearBannerBurstTimer_ >= debugEmitClearBannerBurstInterval_) {
			debugEmitClearBannerBurstTimer_ = 0.0f;

			Vector3 burstPos = playerDisplayPos_ + clearBannerBurstOffset_;
			auto* pm = TKM::ParticleManager::GetInstance();

			pm->Emit("clearBannerBurst_core", burstPos, 6);
			pm->Emit("clearBannerBurst_confetti", burstPos, 70);
			pm->Emit("clearBannerBurst_ray", burstPos, 30);
		}
	}

	// ─────────────────────
	// Skybox回転（GameOverSceneと同じノリ）
	// ─────────────────────
	constexpr float kTwoPi = 6.2831853f;
	skyPitch_ -= skyRotSpeedX_;
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	// X軸だけグルグル
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	// ─────────────────────
	// 自機クリア演出更新
	// ─────────────────────
	player_->SetPosition(playerDisplayPos_); // クリア画面での表示位置
	player_->SetRotation(playerDisplayRot_); // クリア画面での表示回転
	player_->UpdateVisualOnly(dt_); // 入力やゲームプレイ処理は全部止めて、見た目用の更新だけ行う

	// ─────────────────────
	// クリアシーン：ライブ風ファイアー柱
	// カメラ演出終了後に常時噴射
	// ─────────────────────
	if (clearStageFireActive_) {
		clearStageFireTimer_ += dt_;

		const float kFireInterval = 0.045f;

		if (clearStageFireTimer_ >= kFireInterval) {
			clearStageFireTimer_ = 0.0f;

			auto* pm = TKM::ParticleManager::GetInstance();

			for (const Vector3& firePos : clearStageFirePositions_) {
				Vector3 emitPos = firePos + clearParticleGlobalOffset_;
				pm->Emit("clearStageFire_column", emitPos, 3);
				pm->Emit("clearStageFire_top", emitPos, 2);
			}
		}
	}

	// ─────────────────────
	// 「GAME CLEAR」スプライトの表示演出更新
	// アルファフェードとサイズポップの両方を同時に行う
	// ─────────────────────
	if (isClearSpritePopPlaying_) {
		clearSpritePopTime_ += dt_;

		float t = clearSpritePopTime_ / clearSpritePopDuration_;
		t = Ease::Clamp01(t);

		float moveT = Ease::OutBack(t);

		Vector2 pos = {
			MyMath::Lerp(clearSpriteStartPos_.x, clearSpriteCenterPos_.x, moveT),
			MyMath::Lerp(clearSpriteStartPos_.y, clearSpriteCenterPos_.y, moveT)
		};

		clearSprite_->SetPosition(pos);

		if (t >= 1.0f) {
			isClearSpritePopPlaying_ = false;
			clearSprite_->SetPosition(clearSpriteCenterPos_);
		}
	}
	clearSprite_->Update();

	// ─────────────────────
	// コミカル逃走演出の状態更新
	// ─────────────────────
	clearComedySM_.Update(dt_);

	// ─────────────────────
	// パフォーマンス情報更新
	// ─────────────────────
	UpdatePerformanceInfo();
	// ─────────────────────
	// ImGuiデバッグ
	// ─────────────────────
	ImGuiDebug();
}

void GameClearScene::Draw() {
	// --- 背景 ---
	skybox_->Draw(); // スカイボックス

	// --- 3D ---
	Object3dCommon::GetInstance()->DrawSetCommon();
	player_->Draw(dxCommon_); // プレイヤー

	// クリア後のコミカル逃走演出の敵キャラは、カメラ演出が終わってから出す
	if (clearComedyBoss_) { clearComedyBoss_->Draw(dxCommon_); } // ボス
	if (clearComedyMobA_) { clearComedyMobA_->Draw(dxCommon_); } // 雑魚A
	if (clearComedyMobB_) { clearComedyMobB_->Draw(dxCommon_); } // 雑魚B

	// --- パーティクル ---
	TKM::ParticleManager::GetInstance()->Draw();

	// --- 2Dスプライト（文字など）---
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	if (isClearSpriteVisible_) {
		clearSprite_->Draw(); // 「GAME CLEAR」スプライトは、カメラ演出が終わってから描画する
	}

	// アイリスは一番手前
	if ((irisOpening_ || irisClosing_)) {
		iris_->Draw();
	}

	if (isClearMenuVisible_) {
		clearMenu_->Draw();
	}
}

void GameClearScene::ImGuiDebug() {
#ifdef USE_IMGUI
	ImGui::Begin("プレイヤー情報");

	ImGui::Text("位置調整");
	ImGui::DragFloat3("表示位置", &playerDisplayPos_.x, 0.05f);
	ImGui::DragFloat3("表示回転", &playerDisplayRot_.x, 0.01f);

	ImGui::Separator();
	ImGui::Text("クリア演出パーティクル全体補正");
	ImGui::DragFloat3("全体発生オフセット", &clearParticleGlobalOffset_.x, 0.05f);

	ImGui::Separator();
	ImGui::Text("GAME CLEARバースト調整");
	ImGui::DragFloat3("バースト位置補正", &clearBannerBurstOffset_.x, 0.05f);
	ImGui::DragFloat("常時発生間隔", &debugEmitClearBannerBurstInterval_, 0.01f, 0.01f, 5.0f);
	ImGui::Checkbox("常時発生", &debugEmitClearBannerBurst_);

	if (ImGui::Button("1回発生")) {
		Vector3 burstPos = playerDisplayPos_ + clearBannerBurstOffset_;
		auto* pm = TKM::ParticleManager::GetInstance();

		pm->Emit("clearBannerBurst_core", burstPos, 6);
		pm->Emit("clearBannerBurst_confetti", burstPos, 70);
		pm->Emit("clearBannerBurst_ray", burstPos, 30);
	}

	ImGui::End();

	ImGuiDebugInfo(); // パフォーマンス情報デバッグ
#endif
}

void GameClearScene::SetupClearComedyBossConfig_() {
	clearComedyBossConfig_ = BossEnemyConfig{};

	// ここは実際のボス設定に合わせる
	clearComedyBossConfig_.model_ = "jerryfish_boss.obj";
	clearComedyBossConfig_.tentacleModel_ = "tentacle_boss.obj";
	clearComedyBossConfig_.hp_ = 1;
	clearComedyBossConfig_.scale_ = { 2.3f, 2.3f, 2.3f };
}

void GameClearScene::SpawnClearComedyActors_() {
	if (clearComedyActorsSpawned_) {
		return;
	}

	auto* pm = TKM::ParticleManager::GetInstance();

	// ---------------------
	// ボス
	// ---------------------
	clearComedyBoss_ = std::make_unique<BossEnemy>();
	clearComedyBoss_->SetConfig(&clearComedyBossConfig_);
	clearComedyBoss_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	clearComedyBoss_->SetCamera(camera_.get());
	clearComedyBoss_->SetParentScene(this);
	clearComedyBoss_->SetPosition(clearComedyBossStartPos_);
	clearComedyBoss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
	clearComedyBoss_->SetLocked(true);
	clearComedyBoss_->SyncTransform();

	Vector3 bossWarpPos = clearComedyBossStartPos_ + clearParticleGlobalOffset_;

	pm->Emit("clearComedyWarp_core", bossWarpPos, 6);
	pm->Emit("clearComedyWarp_ring", bossWarpPos, 5);
	pm->Emit("clearComedyWarp_streak", bossWarpPos, 64);
	pm->Emit("clearComedyWarp_spark", bossWarpPos, 42);
	pm->Emit("clearComedyWarp_glitter", bossWarpPos, 28);

	// ---------------------
	// 雑魚A
	// ---------------------
	clearComedyMobA_ = std::make_unique<Enemy>();
	clearComedyMobA_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	clearComedyMobA_->SetCamera(camera_.get());
	clearComedyMobA_->SetParentScene(this);
	clearComedyMobA_->SetModel("jerryfish.obj");
	clearComedyMobA_->SetTentacleModel("tentacle.obj");
	clearComedyMobA_->SetPosition(clearComedyMobAStartPos_);
	clearComedyMobA_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
	clearComedyMobA_->SetScale({ 1.2f, 1.2f, 1.2f });
	clearComedyMobA_->SetLocked(true);
	clearComedyMobA_->SyncTransform();

	Vector3 mobAWarpPos = clearComedyMobAStartPos_ + clearParticleGlobalOffset_;

	pm->Emit("clearComedyWarp_core", mobAWarpPos, 4);
	pm->Emit("clearComedyWarp_ring", mobAWarpPos, 4);
	pm->Emit("clearComedyWarp_streak", mobAWarpPos, 44);
	pm->Emit("clearComedyWarp_spark", mobAWarpPos, 28);
	pm->Emit("clearComedyWarp_glitter", mobAWarpPos, 18);

	// ---------------------
	// 雑魚B（転ぶ役）
	// ---------------------
	clearComedyMobB_ = std::make_unique<Enemy>();
	clearComedyMobB_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	clearComedyMobB_->SetCamera(camera_.get());
	clearComedyMobB_->SetParentScene(this);
	clearComedyMobB_->SetModel("jerryfish.obj");
	clearComedyMobB_->SetTentacleModel("tentacle.obj");
	clearComedyMobB_->SetPosition(clearComedyMobBStartPos_);
	clearComedyMobB_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
	clearComedyMobB_->SetScale({ 1.2f, 1.2f, 1.2f });
	clearComedyMobB_->SetLocked(true);
	clearComedyMobB_->SyncTransform();

	Vector3 mobBWarpPos = clearComedyMobBStartPos_ + clearParticleGlobalOffset_;

	pm->Emit("clearComedyWarp_core", mobBWarpPos, 4);
	pm->Emit("clearComedyWarp_ring", mobBWarpPos, 4);
	pm->Emit("clearComedyWarp_streak", mobBWarpPos, 44);
	pm->Emit("clearComedyWarp_spark", mobBWarpPos, 28);
	pm->Emit("clearComedyWarp_glitter", mobBWarpPos, 18);

	clearComedyActorsSpawned_ = true; // 二度とスポーンしないようにフラグを立てる
	clearComedyMobBFallEffectPlayed_ = false; // 転ぶ役の落下エフェクトはまだ再生してない状態
	clearComedyMobBSlipEffectPlayed_ = false; // 転ぶ役の滑るエフェクトはまだ再生してない状態
	clearComedyNoticeMarkPlayed_ = false; // 気づきマークはまだ出してない状態
}