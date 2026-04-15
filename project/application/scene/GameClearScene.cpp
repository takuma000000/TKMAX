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

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using namespace TKM;

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
	// パーティクルグループ
	// ─────────────────────
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, TKM::CameraManager::GetInstance()->GetMainCamera());
	// パーティクルグループの登録は ParticleGroupsCatalogクラス へ
	ParticleGroupsCatalog::RegisterScene(ParticleManager::GetInstance());

	// ─────────────────────
	// カメラ
	// ─────────────────────
	camera_ = std::make_unique<TKM::Camera>();
	camera_->SetRotate(cameraStartRot_); // カメラ回転
	camera_->SetTranslate(cameraStartPos_); // カメラ位置
	camera_->Update();

	cameraMoveTime_ = 0.0f; // カメラ移動開始からの経過時間

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
	// クリア後コミカル逃走演出
	// ─────────────────────
	clearComedyPhase_ = ClearComedyPhase::None; // 最初は何もしてない状態
	clearComedyFallSlowRequested_ = false; // 転ぶ瞬間スロー未実行
	clearComedyTimeScale_.Initialize(); // タイムスケール初期化
	SetupClearComedyBossConfig_(); // コミカル逃走演出用のボス設定を行う
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
	if (!irisClosing_ && !irisOpening_) {
		const auto cmd = clearMenu_->Update(dt_);

		// コマンドに応じてアイリス閉じ開始
		if (cmd == GameResultMenuController::Command::Restart) {
			nextAction_ = NextAction::Restart; // リスタート
			irisClosing_ = true; // アイリス閉じ開始
			irisCloseTween_.Reset(/*start*/ 0.0f, /*end*/ irisMaxScale_, /*sec*/ 0.8f, Ease::Type::InBack); // 閉じはInBackで
		} else if (cmd == GameResultMenuController::Command::ReturnToTitle) { // タイトルに戻る
			nextAction_ = NextAction::ReturnToTitle; // タイトルに戻る
			irisClosing_ = true; // アイリス閉じ開始
			irisCloseTween_.Reset(/*start*/ 0.0f, /*end*/ irisMaxScale_, /*sec*/ 0.8f, Ease::Type::InBack); // 閉じはInBackで
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
			Vector3 celebrateCenter = playerDisplayPos_ + Vector3{ 8.0f, 4.5f, 14.0f };

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
			}
		}

		// ─────────────────────
		// カメラ演出終了判定
		// 「最後まで到達したら」通常状態へ
		// ─────────────────────
		if (t >= 1.0f) {
			enableCameraIntro_ = false; // カメラ演出終了
			isClearSpriteVisible_ = true; // スプライト表示開始
			isClearSpritePopPlaying_ = true; // 位置ポップ演出開始
			clearSpritePopTime_ = 0.0f; // 演出時間リセット
			clearSprite_->SetPosition(clearSpriteStartPos_); // 少し下からスタート
			clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		}
	} else {
		camera_->SetTranslate(cameraEndPos_); // カメラ位置
		camera_->SetRotate(cameraEndRot_); // カメラ回転

		// ブラーは常にOFF
		if (postFx_) {
			postFx_->SetRadialBlurManual(false, 0.0f);
		}

		// クリアスプライト表示ON（カメラ演出が終わったら表示する）
		isClearSpriteVisible_ = true;
	}
	camera_->Update();

	// ─────────────────────
	// ライト更新
	// ─────────────────────
	dirLight_->Update();

	// ─────────────────────
	// ポストエフェクト更新
	// ─────────────────────
	postFx_->Update(dt_, nullptr);

	// ─────────────────────
	// ーティクル更新
	// ─────────────────────
	TKM::ParticleManager::GetInstance()->Update(dt_);

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
	// クリア後のコミカル逃走演出更新
	// クリアスプライトの位置ポップ演出が終わったら、少し待ってから逃走演出開始
	// ─────────────────────
	if (isClearSpriteVisible_ &&
		!isClearSpritePopPlaying_ &&
		clearComedyPhase_ == ClearComedyPhase::None) {
		clearComedyPhase_ = ClearComedyPhase::WaitAfterClear; // クリア後少し待つフェーズへ
		clearComedyTimer_ = 0.0f; // フェーズ開始からの経過時間リセット
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
	// クリア後のコミカル逃走演出更新
	// フェーズ管理して、ボスと雑魚を順番に出す
	// ─────────────────────
	UpdateClearComedy_();

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
	clearMenu_->Draw();
}

void GameClearScene::ImGuiDebug() {
#ifdef USE_IMGUI
	ImGui::Begin("プレイヤー情報");

	ImGui::Text("位置調整");
	ImGui::DragFloat3("表示位置", &playerDisplayPos_.x, 0.05f);
	ImGui::DragFloat3("表示回転", &playerDisplayRot_.x, 0.01f);

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

	// フラグを立てて、二度とスポーンしないようにする
	clearComedyActorsSpawned_ = true;
}

void GameClearScene::UpdateClearComedy_() {
	clearComedyTimeScale_.Update(dt_);
	const float comedyDt = dt_ * clearComedyTimeScale_.GetScale();

	if (clearComedyPhase_ == ClearComedyPhase::None || clearComedyPhase_ == ClearComedyPhase::Done) {
		return;
	}

	clearComedyTimer_ += comedyDt;

	switch (clearComedyPhase_) {
	case ClearComedyPhase::WaitAfterClear:
		if (clearComedyTimer_ >= 0.65f) {
			SpawnClearComedyActors_();
			clearComedyPhase_ = ClearComedyPhase::Spawn;
			clearComedyTimer_ = 0.0f;
		}
		break;

	case ClearComedyPhase::Spawn:
		if (clearComedyBoss_) {
			clearComedyBoss_->SetIntroPanic(true, 0.35f);
			clearComedyBoss_->Update(comedyDt);
		}
		if (clearComedyMobA_) {
			clearComedyMobA_->Update(comedyDt);
		}
		if (clearComedyMobB_) {
			clearComedyMobB_->Update(comedyDt);
		}

		if (clearComedyTimer_ >= 0.65f) {
			clearComedyPhase_ = ClearComedyPhase::SlowNotice;
			clearComedyTimer_ = 0.0f;
		}
		break;

	case ClearComedyPhase::SlowNotice:

		if (clearComedyBoss_) {
			clearComedyBoss_->SetIntroPanic(true, 1.0f);
			clearComedyBoss_->Update(comedyDt);
		}
		if (clearComedyMobA_) {
			clearComedyMobA_->Update(comedyDt);
		}
		if (clearComedyMobB_) {
			clearComedyMobB_->Update(comedyDt);
		}

		if (clearComedyTimer_ >= 0.75f) {
			// RunAway の開始位置を、この瞬間の見た目位置で確定
			if (clearComedyBoss_) {
				clearComedyBossRunStartPos_ = clearComedyBoss_->GetWorldPosition();
			}
			if (clearComedyMobA_) {
				clearComedyMobARunStartPos_ = clearComedyMobA_->GetWorldPosition();
			}
			if (clearComedyMobB_) {
				clearComedyMobBRunStartPos_ = clearComedyMobB_->GetWorldPosition();
			}

			clearComedyFallSlowRequested_ = false; // 転ぶ瞬間スローを次フェーズで使う
			clearComedyPhase_ = ClearComedyPhase::RunAway;
			clearComedyTimer_ = 0.0f;
		}
		break;

	case ClearComedyPhase::RunAway:
	{
		float t = std::clamp(clearComedyTimer_ / 1.35f, 0.0f, 1.0f);
		float bossMoveT = Ease::Eval(Ease::Type::InQuad, std::clamp(clearComedyTimer_ / 1.80f, 0.0f, 1.0f));
		float mobMoveT = Ease::Eval(Ease::Type::InQuad, t);

		if (clearComedyBoss_) {
			Vector3 pos = MyMath::Vector3Lerp(clearComedyBossRunStartPos_, clearComedyBossEscapePos_, bossMoveT);
			clearComedyBoss_->SetPosition(pos);
			clearComedyBoss_->SetRotate({ 0.0f, -0.9f, 0.0f });
			clearComedyBoss_->SetIntroPanic(true, 0.75f);
			clearComedyBoss_->SyncTransform();
			clearComedyBoss_->Update(comedyDt);
		}

		if (clearComedyMobA_) {
			Vector3 pos = MyMath::Vector3Lerp(clearComedyMobARunStartPos_, clearComedyMobAEscapePos_, mobMoveT);
			clearComedyMobA_->SetPosition(pos);
			clearComedyMobA_->SetRotate({ 0.0f, -0.9f, 0.0f });
			clearComedyMobA_->SyncTransform();
			clearComedyMobA_->Update(comedyDt);
		}

		if (clearComedyMobB_) {
			Vector3 pos = MyMath::Vector3Lerp(clearComedyMobBRunStartPos_, clearComedyMobBFallPos_, mobMoveT);
			clearComedyMobB_->SetPosition(pos);
			clearComedyMobB_->SetRotate({ 0.0f, -0.9f, 0.0f });
			clearComedyMobB_->SyncTransform();
			clearComedyMobB_->Update(comedyDt);
		}

		if (clearComedyTimer_ >= 1.35f) {
			// 次フェーズ開始位置を「今いる位置」で確定
			if (clearComedyBoss_) {
				clearComedyBossRecoverStartPos_ = clearComedyBoss_->GetWorldPosition();
			}
			if (clearComedyMobA_) {
				clearComedyMobARecoverStartPos_ = clearComedyMobA_->GetWorldPosition();
			}
			if (clearComedyMobB_) {
				clearComedyMobBRecoverStartPos_ = clearComedyMobB_->GetWorldPosition();
			}

			clearComedyPhase_ = ClearComedyPhase::FallDown;
			clearComedyTimer_ = 0.0f;
		}
	}
	break;

	case ClearComedyPhase::FallDown:
	{
		float t = std::clamp(clearComedyTimer_ / 0.85f, 0.0f, 1.0f);

		// ボスは待たずにそのまま退場方向へ進む
		if (clearComedyBoss_) {
			float bossMoveT = Ease::Eval(Ease::Type::InQuad, t);
			Vector3 bossPos = MyMath::Vector3Lerp(
				clearComedyBossRecoverStartPos_,
				clearComedyBossExitPos_,
				bossMoveT
			);
			clearComedyBoss_->SetPosition(bossPos);
			clearComedyBoss_->SetRotate({ 0.0f, -1.00f, 0.0f });
			clearComedyBoss_->SetIntroPanic(false, 0.0f);
			clearComedyBoss_->SyncTransform();
			clearComedyBoss_->Update(comedyDt);
		}

		// 雑魚Aも待たずにそのまま退場方向へ進む
		if (clearComedyMobA_) {
			float mobAMoveT = Ease::Eval(Ease::Type::InQuad, t);
			Vector3 mobAPos = MyMath::Vector3Lerp(
				clearComedyMobARecoverStartPos_,
				clearComedyMobAExitPos_,
				mobAMoveT
			);
			clearComedyMobA_->SetPosition(mobAPos);
			clearComedyMobA_->SetRotate({ 0.0f, -1.00f, 0.0f });
			clearComedyMobA_->SyncTransform();
			clearComedyMobA_->Update(comedyDt);
		}

		// 転ぶ役だけ、漫画みたいに「ポン → ズコーーー」
		if (clearComedyMobB_) {

			Vector3 startPos = clearComedyMobBRecoverStartPos_;

			// まず一瞬浮くターゲット
			Vector3 popPos = startPos + Vector3{ 0.0f, 1.4f, 0.8f };

			// 最終的な転倒位置
			Vector3 slamPos = startPos + Vector3{ 0.0f, -1.8f, 2.8f };

			Vector3 pos{};
			Vector3 rot{};

			if (t < 0.35f) {

				if (!clearComedyFallSlowRequested_) {
					clearComedyTimeScale_.RequestSlowAdvanced(0.20f, 1.7f, 0.05f, 0.25f);
					clearComedyFallSlowRequested_ = true;
				}

				// -----------------------------
				// 前半：一瞬ふわっと浮く
				// -----------------------------
				float u = t / 0.35f;
				float jumpT = Ease::Eval(Ease::Type::OutQuad, u);

				pos = MyMath::Vector3Lerp(startPos, popPos, jumpT);

				// 少し前のめりになりながら浮く
				rot.x = MyMath::Lerp(0.0f, -0.35f, jumpT);
				rot.y = MyMath::Lerp(-0.9f, -0.75f, jumpT);
				rot.z = MyMath::Lerp(0.0f, 0.35f, jumpT);
			} else {
				// -----------------------------
				// 後半：ズコーーーーっと落ちる
				// -----------------------------
				float u = (t - 0.35f) / 0.65f;
				float slamT = Ease::Eval(Ease::Type::InExpo, u);

				pos = MyMath::Vector3Lerp(popPos, slamPos, slamT);

				// 一気に横倒れ
				rot.x = MyMath::Lerp(-0.35f, 0.15f, slamT);
				rot.y = MyMath::Lerp(-0.75f, clearComedyMobBFallRot_.y, slamT);
				rot.z = MyMath::Lerp(0.35f, 1.95f, slamT);
			}

			clearComedyMobB_->SetPosition(pos);
			clearComedyMobB_->SetRotate(rot);
			clearComedyMobB_->SyncTransform();
			clearComedyMobB_->Update(comedyDt);
		}

		if (clearComedyTimer_ >= 0.85f) {
			// ボスと雑魚Aはここで退場済みにする
			clearComedyBoss_.reset();
			clearComedyMobA_.reset();

			// 転んだ雑魚だけ、ここから起き上がり開始位置を取る
			if (clearComedyMobB_) {
				clearComedyMobBRecoverStartPos_ = clearComedyMobB_->GetWorldPosition();
			}

			clearComedyPhase_ = ClearComedyPhase::StandUp;
			clearComedyTimer_ = 0.0f;
		}
	}
	break;

	case ClearComedyPhase::StandUp:
	{
		float t = std::clamp(clearComedyTimer_ / 1.0f, 0.0f, 1.0f);
		float standT = Ease::Eval(Ease::Type::OutBack, t);

		if (clearComedyMobB_) {
			// 位置は動かさない。その場で起き上がる
			clearComedyMobB_->SetPosition(clearComedyMobBRecoverStartPos_);

			Vector3 rot = {
				0.0f,
				MyMath::Lerp(clearComedyMobBFallRot_.y, -1.05f, standT),
				MyMath::Lerp(1.95f, 0.0f, standT)
			};

			clearComedyMobB_->SetRotate(rot);
			clearComedyMobB_->SyncTransform();
			clearComedyMobB_->Update(comedyDt);
		}

		if (clearComedyTimer_ >= 1.0f) {
			// 起き上がり終わった地点を逃走開始位置にする
			if (clearComedyMobB_) {
				clearComedyMobBRecoverStartPos_ = clearComedyMobB_->GetWorldPosition();
			}

			clearComedyPhase_ = ClearComedyPhase::RecoverRun;
			clearComedyTimer_ = 0.0f;
		}
	}
	break;

	case ClearComedyPhase::RecoverRun:
	{
		float t = std::clamp(clearComedyTimer_ / 1.00f, 0.0f, 1.0f);
		float moveT = Ease::Eval(Ease::Type::InCubic, t);

		// 起き上がった後に逃走
		if (clearComedyMobB_) {
			Vector3 pos = MyMath::Vector3Lerp(
				clearComedyMobBRecoverStartPos_,
				clearComedyMobBExitPos_,
				moveT
			);

			clearComedyMobB_->SetPosition(pos);
			clearComedyMobB_->SetRotate({ 0.0f, -1.05f, 0.0f });
			clearComedyMobB_->SyncTransform();
			clearComedyMobB_->Update(comedyDt);
		}

		if (clearComedyTimer_ >= 1.00f) {
			clearComedyMobB_.reset();

			clearComedyActorsSpawned_ = false;
			clearComedyPhase_ = ClearComedyPhase::Done;
			clearComedyTimer_ = 0.0f;
		}
	}
	break;

	case ClearComedyPhase::Done:
	case ClearComedyPhase::None:
		break;
	}
}