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

/// <summary>
/// タイトルシーンの爆発演出に近いパーティクルをまとめて発生させます。
/// </summary>
/// <param name="pos">発生位置</param>
static void EmitTitleExplodeLike_(const Vector3& pos) {
	auto* pm = TKM::ParticleManager::GetInstance();

	// パーティクルマネージャが取得できない場合は発生処理を行わない
	if (!pm) {
		return;
	}

	// 爆発の中心部分
	pm->Emit("titleExplode_core", pos, 28);

	// 外側へ伸びる放射状の光
	pm->Emit("titleExplode_rays", pos, 140);

	// 飛び散る破片
	pm->Emit("titleExplode_debris", pos, 90);

	// 爆発の広がりを見せるリング
	pm->Emit("titleExplode_ring", pos, 2);
}

void GameClearScene::Initialize() {
	/// ──────────────── 音声読み込み ───────────────
	AudioCatalog::LoadResultAudios();

	/// ──────────────── モデル・テクスチャ読み込み ───────────────
	ModelCatalog::LoadModelCatalogs(dxCommon_);
	TextureCatalog::LoadTextureCatalogs();

	/// ──────────────── カメラ初期化 ───────────────
	camera_ = std::make_unique<TKM::Camera>();

	// クリア演出開始時のカメラ姿勢を設定
	camera_->SetRotate(cameraStartRot_);
	camera_->SetTranslate(cameraStartPos_);
	camera_->Update();

	// カメラ移動演出の経過時間を初期化
	cameraMoveTime_ = 0.0f;

	/// ──────────────── パーティクル初期化 ───────────────
	ParticleManager::GetInstance()->ClearAllGroups();
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());
	ParticleGroupsCatalog::RegisterScene(ParticleManager::GetInstance());

	/// ──────────────── ライト初期化 ───────────────
	dirLight_ = std::make_unique<DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	/// ──────────────── スカイボックス初期化 ───────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());

	/// ──────────────── 自機初期化 ───────────────
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// クリアシーンでは操作させず、見せるための自機として扱う
	player_->SetCamera(camera_.get());
	player_->SetControlEnabled(false);
	player_->SetReticleVisible(false);
	player_->SetEnableJetSmoke(true);

	// クリア演出用の表示位置・回転を反映
	player_->SetPosition(playerDisplayPos_);
	player_->SetRotation(playerDisplayRot_);

	// 自機演出用タイマーを初期化
	planeTime_ = 0.0f;

	/// ──────────────── GAME CLEARスプライト初期化 ───────────────
	clearSprite_ = std::make_unique<Sprite>();
	clearSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/clear.png");

	// 中央基準で扱えるようにする
	clearSprite_->SetAnchorPoint({ 0.5f, 0.5f });

	// 最初は画面外、または開始位置に置いておく
	clearSprite_->SetPosition(clearSpriteStartPos_);

	// 初期表示色は白・不透明
	clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	/// ──────────────── アイリス初期化 ───────────────
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMaxScale_);

	// 開始時は画面を覆うサイズから始める
	irisScale_ = irisMaxScale_;
	iris_->SetSize({ irisScale_, irisScale_ });

	// クリアシーン開始時のアイリス開き演出
	irisOpenTween_.Reset(
		/*start*/ irisMaxScale_,
		/*end*/   0.0f,
		/*sec*/   0.8f,
		Ease::Type::OutBack
	);

	irisOpening_ = true;
	irisClosing_ = false;

	/// ──────────────── クリアメニュー初期化 ───────────────
	clearMenu_ = std::make_unique<GameResultMenuController>();
	clearMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		static_cast<float>(WindowsAPI::GetClientWidth()),
		static_cast<float>(WindowsAPI::GetClientHeight())
	);

	/// ──────────────── 水面波紋エフェクト初期化 ───────────────
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();
	rippleEffect_->Initialize(dxCommon_);

	// DirectXCommon側に現在使用する波紋エフェクトを登録
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());

	/// ──────────────── ポストエフェクト初期化 ───────────────
	postFx_ = std::make_unique<TKM::PostEffectController>();
	postFx_->Initialize(dxCommon_, player_.get(), nullptr);

	// カメラ接近中のブラー解除フラグを初期化
	blurReleased_ = false;

	/// ──────────────── コミカル逃走演出初期化 ───────────────
	clearComedyFallSlowRequested_ = false;
	clearComedyTimeScale_.Initialize();

	// クリア後に登場するボスの設定を作成
	SetupClearComedyBossConfig_();

	// コミカル逃走演出用ステートマシンを初期化
	clearComedySM_.Initialize(this);
}

void GameClearScene::Finalize() {
	/// ──────────────── 各種終了処理 ───────────────
	// このシーンで登録した波紋エフェクトを解除
	dxCommon_->SetWaterRippleEffect(nullptr);

	// ポストエフェクトを終了
	postFx_->Finalize();

	// オーディオを終了
	AudioManager::GetInstance()->Finalize();
}

void GameClearScene::Update() {
	/// ──────────────── 入力更新 ───────────────
	Input::GetInstance()->Update();

	/// ──────────────── 水面波紋エフェクト更新 ───────────────
	rippleEffect_->Update(dt_);

	/// ──────────────── アイリス開き更新 ───────────────
	if (irisOpening_) {
		// Tweenの進行に合わせてアイリスサイズを更新
		irisScale_ = UpdateIrisScale(iris_.get(), irisOpenTween_, dt_);

		// 開き切ったら開き演出を終了
		if (irisOpenTween_.Finished()) {
			irisOpening_ = false;
		}
	}

	/// ──────────────── クリアメニュー更新 ───────────────
	if (!irisClosing_ && !irisOpening_ && isClearMenuVisible_) {
		const auto cmd = clearMenu_->Update(dt_);

		// リスタートが選ばれた場合
		if (cmd == GameResultMenuController::Command::Restart) {
			nextAction_ = NextAction::Restart;
			irisClosing_ = true;

			// アイリスを閉じてからゲームシーンへ戻る
			irisCloseTween_.Reset(
				/*start*/ 0.0f,
				/*end*/ irisMaxScale_,
				/*sec*/ 0.8f,
				Ease::Type::InBack
			);
		}
		// タイトルへ戻るが選ばれた場合
		else if (cmd == GameResultMenuController::Command::ReturnToTitle) {
			nextAction_ = NextAction::ReturnToTitle;
			irisClosing_ = true;

			// アイリスを閉じてからタイトルシーンへ戻る
			irisCloseTween_.Reset(
				/*start*/ 0.0f,
				/*end*/ irisMaxScale_,
				/*sec*/ 0.8f,
				Ease::Type::InBack
			);
		}
	}

	/// ──────────────── アイリス閉じ更新 ───────────────
	if (irisClosing_) {
		// Tweenの進行に合わせてアイリスサイズを更新
		irisScale_ = UpdateIrisScale(iris_.get(), irisCloseTween_, dt_);

		// 閉じ切ったら次のシーンへ切り替える
		if (irisCloseTween_.Finished()) {
			if (nextAction_ == NextAction::Restart) {
				sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_));
				return;
			}

			sceneManager_->SetNextScene(std::make_unique<TitleScene>(dxCommon_, srvManager_));
			return;
		}
	}

	/// ──────────────── カメラ演出更新 ───────────────
	if (enableCameraIntro_) {
		// カメラ移動演出の経過時間を進める
		cameraMoveTime_ += dt_;

		// 0.0f～1.0fの進行率に変換
		float t = cameraMoveTime_ / cameraMoveDuration_;
		t = std::clamp(t, 0.0f, 1.0f);

		// 位置用・回転用で別々のイージングを使う
		float posT = Ease::Eval(cameraPosEaseType_, t);
		float rotT = Ease::Eval(cameraRotEaseType_, t);

		// 開始位置から終了位置へ補間
		Vector3 camPos = MyMath::Vector3Lerp(cameraStartPos_, cameraEndPos_, posT);

		// 開始回転から終了回転へ補間
		Vector3 camRot = MyMath::Vector3Lerp(cameraStartRot_, cameraEndRot_, rotT);

		camera_->SetTranslate(camPos);
		camera_->SetRotate(camRot);

		// カメラ接近中はGAME CLEAR文字をまだ見せない
		isClearSpriteVisible_ = false;

		/// ──────────────── カメラブラー制御 ───────────────
		if (postFx_) {
			// まだブラー解除が完了していない場合のみ制御する
			if (!blurReleased_) {
				// カメラ移動中はラジアルブラーをかける
				if (posT < 1.0f) {
					postFx_->SetRadialBlurManual(true, cameraBlurStrength_);
				}
				// 到着した瞬間にブラーを解除する
				else {
					postFx_->SetRadialBlurManual(false, 0.0f);
					blurReleased_ = true;
				}
			}
			// 一度解除した後は確実にOFFを維持する
			else {
				postFx_->SetRadialBlurManual(false, 0.0f);
			}
		}

		/// ──────────────── クリア祝福パーティクル更新 ───────────────
		{
			// 指定範囲内のランダム値を返すローカル関数
			auto frand = [](float a, float b) {
				return a + (b - a) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
				};

			// 演出の盛り上がりをsinで作る
			float peak = std::sin(t * MyMath::GetPI());
			peak = std::clamp(peak, 0.0f, 1.0f);

			// 祝福パーティクルの中心位置
			Vector3 celebrateCenter = playerDisplayPos_ + clearCelebrateOffset_ + clearParticleGlobalOffset_;

			// 各パーティクル用タイマーを進める
			celebrateCoreTimer_ += dt_;
			celebrateSparkTimer_ += dt_;
			celebrateRayTimer_ += dt_;

			// 盛り上がるほど発生間隔を短くする
			float coreInterval = MyMath::Lerp(0.28f, 0.10f, peak);
			float sparkInterval = MyMath::Lerp(0.08f, 0.015f, peak);
			float rayInterval = MyMath::Lerp(0.22f, 0.07f, peak);

			// 中心系パーティクル
			if (celebrateCoreTimer_ >= coreInterval) {
				celebrateCoreTimer_ = 0.0f;

				Vector3 p = celebrateCenter + Vector3{
					frand(-8.0f, 8.0f),
					frand(-2.0f, 6.0f),
					frand(-6.0f, 6.0f)
				};

				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_core", p, 1);
			}

			// キラキラ系パーティクル
			if (celebrateSparkTimer_ >= sparkInterval) {
				celebrateSparkTimer_ = 0.0f;

				Vector3 p = celebrateCenter + Vector3{
					frand(-16.0f, 16.0f),
					frand(-6.0f, 10.0f),
					frand(-12.0f, 12.0f)
				};

				// 盛り上がるほど一度に出す量を増やす
				int count = static_cast<int>(MyMath::Lerp(8.0f, 22.0f, peak));
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_spark", p, count);

				// 一定確率でタイトル爆発風の派手な演出も混ぜる
				float explodeChance = MyMath::Lerp(0.03f, 0.10f, peak);
				if (frand(0.0f, 1.0f) < explodeChance) {
					EmitTitleExplodeLike_(p);
				}
			}

			// 放射状の光パーティクル
			if (celebrateRayTimer_ >= rayInterval) {
				celebrateRayTimer_ = 0.0f;

				Vector3 p = celebrateCenter + Vector3{
					frand(-12.0f, 12.0f),
					frand(-4.0f, 8.0f),
					frand(-10.0f, 10.0f)
				};

				// 盛り上がるほど本数を増やす
				int count = static_cast<int>(MyMath::Lerp(1.0f, 3.0f, peak));
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_ray", p, count);
			}

			// カメラ到着時に一度だけ大きな締めバーストを出す
			if (!celebrateFinalBurstDone_ && posT >= 1.0f) {
				celebrateFinalBurstDone_ = true;

				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_core", celebrateCenter, 10);
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_spark", celebrateCenter, 70);
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_ray", celebrateCenter, 16);

				EmitTitleExplodeLike_(celebrateCenter);
			}
		}

		/// ──────────────── カメラ演出終了判定 ───────────────
		if (t >= 1.0f) {
			// カメラ接近演出を終了
			enableCameraIntro_ = false;

			// GAME CLEAR表示演出を開始前の状態に戻す
			isClearSpriteVisible_ = false;
			isClearSpritePopPlaying_ = false;
			clearSpritePopTime_ = 0.0f;
			clearSprite_->SetPosition(clearSpriteStartPos_);
			clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

			// まだメニューは見せない
			isClearMenuVisible_ = false;

			// コミカル逃走演出の最初のステートへ入る
			if (clearComedySM_.GetState() == nullptr) {
				clearComedySM_.Change(std::make_unique<ClearComedyWaitAfterClearState>());
			}
		}
	} else {
		// カメラ演出後は終了位置に固定する
		camera_->SetTranslate(cameraEndPos_);
		camera_->SetRotate(cameraEndRot_);

		// 演出終了後にブラーが残らないようにする
		if (postFx_) {
			postFx_->SetRadialBlurManual(false, 0.0f);
		}
	}

	/// ──────────────── カメラ・パーティクルカメラ更新 ───────────────
	camera_->Update();

	// パーティクル描画に使うカメラを現在のシーンカメラへ更新
	ParticleManager::GetInstance()->SetCamera(camera_.get());

	/// ──────────────── ライト更新 ───────────────
	dirLight_->Update();

	/// ──────────────── ポストエフェクト更新 ───────────────
	postFx_->Update(dt_, nullptr);

	/// ──────────────── パーティクル更新 ───────────────
	TKM::ParticleManager::GetInstance()->Update(dt_);

	/// ──────────────── デバッグ用GAME CLEARバースト更新 ───────────────
	if (debugEmitClearBannerBurst_) {
		debugEmitClearBannerBurstTimer_ += dt_;

		// 指定間隔ごとに確認用バーストを発生させる
		if (debugEmitClearBannerBurstTimer_ >= debugEmitClearBannerBurstInterval_) {
			debugEmitClearBannerBurstTimer_ = 0.0f;

			Vector3 burstPos = playerDisplayPos_ + clearBannerBurstOffset_;
			auto* pm = TKM::ParticleManager::GetInstance();

			pm->Emit("clearBannerBurst_core", burstPos, 6);
			pm->Emit("clearBannerBurst_confetti", burstPos, 70);
			pm->Emit("clearBannerBurst_ray", burstPos, 30);
		}
	}

	/// ──────────────── スカイボックス回転更新 ───────────────
	constexpr float kTwoPi = 6.2831853f;

	// X軸方向にゆっくり回転させる
	skyPitch_ -= skyRotSpeedX_;

	// 角度が範囲外に出すぎないように補正
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	/// ──────────────── 自機クリア演出更新 ───────────────
	// ImGuiなどで調整した表示位置・回転を毎フレーム反映
	player_->SetPosition(playerDisplayPos_);
	player_->SetRotation(playerDisplayRot_);

	// 操作なしで見た目だけ更新する
	player_->UpdateVisualOnly(dt_);

	/// ──────────────── クリアシーンライブ風ファイアー柱更新 ───────────────
	if (clearStageFireActive_) {
		clearStageFireTimer_ += dt_;

		const float kFireInterval = 0.045f;

		// 一定間隔でステージ下側から炎柱を発生させる
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

	/// ──────────────── GAME CLEARスプライト表示演出更新 ───────────────
	if (isClearSpritePopPlaying_) {
		clearSpritePopTime_ += dt_;

		// 表示演出の進行率
		float t = clearSpritePopTime_ / clearSpritePopDuration_;
		t = Ease::Clamp01(t);

		// 少し飛び出すような動きにする
		float moveT = Ease::OutBack(t);

		Vector2 pos = {
			MyMath::Lerp(clearSpriteStartPos_.x, clearSpriteCenterPos_.x, moveT),
			MyMath::Lerp(clearSpriteStartPos_.y, clearSpriteCenterPos_.y, moveT)
		};

		clearSprite_->SetPosition(pos);

		// 演出が終わったら中央位置に固定
		if (t >= 1.0f) {
			isClearSpritePopPlaying_ = false;
			clearSprite_->SetPosition(clearSpriteCenterPos_);
		}
	}

	clearSprite_->Update();

	/// ──────────────── コミカル逃走演出更新 ───────────────
	clearComedySM_.Update(dt_);

	/// ──────────────── パフォーマンス情報更新 ───────────────
	UpdatePerformanceInfo();

	/// ──────────────── ImGuiデバッグ更新 ───────────────
	ImGuiDebug();
}

void GameClearScene::Draw() {
	/// ──────────────── 背景描画 ───────────────
	skybox_->Draw();

	/// ──────────────── 3D描画 ───────────────
	Object3dCommon::GetInstance()->DrawSetCommon();

	// クリア演出用の自機を描画
	player_->Draw(dxCommon_);

	// コミカル逃走演出用の敵キャラを描画
	if (clearComedyBoss_) {
		clearComedyBoss_->Draw(dxCommon_);
	}

	if (clearComedyMobA_) {
		clearComedyMobA_->Draw(dxCommon_);
	}

	if (clearComedyMobB_) {
		clearComedyMobB_->Draw(dxCommon_);
	}

	/// ──────────────── パーティクル描画 ───────────────
	TKM::ParticleManager::GetInstance()->Draw();

	/// ──────────────── 2Dスプライト描画 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	// GAME CLEAR文字
	if (isClearSpriteVisible_) {
		clearSprite_->Draw();
	}

	// シーン開始・終了時のアイリス
	if ((irisOpening_ || irisClosing_)) {
		iris_->Draw();
	}

	// クリアメニュー
	if (isClearMenuVisible_) {
		clearMenu_->Draw();
	}
}

void GameClearScene::ImGuiDebug() {
#ifdef USE_IMGUI
	/// ──────────────── プレイヤー情報デバッグ ───────────────
	ImGui::Begin("プレイヤー情報");

	// クリア画面で表示する自機の位置・回転を調整
	ImGui::Text("位置調整");
	ImGui::DragFloat3("表示位置", &playerDisplayPos_.x, 0.05f);
	ImGui::DragFloat3("表示回転", &playerDisplayRot_.x, 0.01f);

	ImGui::Separator();

	// クリア演出パーティクル全体の位置補正
	ImGui::Text("クリア演出パーティクル全体補正");
	ImGui::DragFloat3("全体発生オフセット", &clearParticleGlobalOffset_.x, 0.05f);

	ImGui::Separator();

	// GAME CLEAR周りのバースト演出調整
	ImGui::Text("GAME CLEARバースト調整");
	ImGui::DragFloat3("バースト位置補正", &clearBannerBurstOffset_.x, 0.05f);
	ImGui::DragFloat("常時発生間隔", &debugEmitClearBannerBurstInterval_, 0.01f, 0.01f, 5.0f);
	ImGui::Checkbox("常時発生", &debugEmitClearBannerBurst_);

	// ボタンを押した瞬間だけバーストを出す
	if (ImGui::Button("1回発生")) {
		Vector3 burstPos = playerDisplayPos_ + clearBannerBurstOffset_;
		auto* pm = TKM::ParticleManager::GetInstance();

		pm->Emit("clearBannerBurst_core", burstPos, 6);
		pm->Emit("clearBannerBurst_confetti", burstPos, 70);
		pm->Emit("clearBannerBurst_ray", burstPos, 30);
	}

	ImGui::End();

	/// ──────────────── パフォーマンス情報デバッグ ───────────────
	ImGuiDebugInfo();
#endif
}

void GameClearScene::SetupClearComedyBossConfig_() {
	/// ──────────────── コミカル逃走用ボス設定 ───────────────
	// いったん初期値でリセットしてから必要な項目だけ設定する
	clearComedyBossConfig_ = BossEnemyConfig{};

	// クリア後のコミカル演出で使うボスモデル
	clearComedyBossConfig_.model_ = "jerryfish_boss.obj";

	// ボスの触手モデル
	clearComedyBossConfig_.tentacleModel_ = "tentacle_boss.obj";

	// 演出用なのでHPは最低値にしておく
	clearComedyBossConfig_.hp_ = 1;

	// クリア画面上で見栄えするように大きめにする
	clearComedyBossConfig_.scale_ = { 2.3f, 2.3f, 2.3f };
}

void GameClearScene::SpawnClearComedyActors_() {
	/// ──────────────── 二重生成防止 ───────────────
	if (clearComedyActorsSpawned_) {
		return;
	}

	auto* pm = TKM::ParticleManager::GetInstance();

	/// ──────────────── コミカル逃走用ボス生成 ───────────────
	clearComedyBoss_ = std::make_unique<BossEnemy>();

	// 先に設定を渡してから初期化する
	clearComedyBoss_->SetConfig(&clearComedyBossConfig_);
	clearComedyBoss_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// クリアシーンのカメラ・シーン情報を渡す
	clearComedyBoss_->SetCamera(camera_.get());
	clearComedyBoss_->SetParentScene(this);

	// 出現位置と向きを設定
	clearComedyBoss_->SetPosition(clearComedyBossStartPos_);
	clearComedyBoss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

	// 演出中に通常挙動で動かないようロックする
	clearComedyBoss_->SetLocked(true);

	// 設定した位置・回転・スケールを内部Transformへ反映
	clearComedyBoss_->SyncTransform();

	// ボス出現位置にワープ演出を出す
	Vector3 bossWarpPos = clearComedyBossStartPos_ + clearParticleGlobalOffset_;

	pm->Emit("clearComedyWarp_core", bossWarpPos, 6);
	pm->Emit("clearComedyWarp_ring", bossWarpPos, 5);
	pm->Emit("clearComedyWarp_streak", bossWarpPos, 64);
	pm->Emit("clearComedyWarp_spark", bossWarpPos, 42);
	pm->Emit("clearComedyWarp_glitter", bossWarpPos, 28);

	/// ──────────────── コミカル逃走用 雑魚A生成 ───────────────
	clearComedyMobA_ = std::make_unique<Enemy>();
	clearComedyMobA_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// クリアシーン用の参照設定
	clearComedyMobA_->SetCamera(camera_.get());
	clearComedyMobA_->SetParentScene(this);

	// 雑魚Aの見た目設定
	clearComedyMobA_->SetModel("jerryfish.obj");
	clearComedyMobA_->SetTentacleModel("tentacle.obj");

	// 雑魚Aの配置設定
	clearComedyMobA_->SetPosition(clearComedyMobAStartPos_);
	clearComedyMobA_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
	clearComedyMobA_->SetScale({ 1.2f, 1.2f, 1.2f });

	// 演出制御用に通常挙動を止める
	clearComedyMobA_->SetLocked(true);
	clearComedyMobA_->SyncTransform();

	// 雑魚A出現位置にワープ演出を出す
	Vector3 mobAWarpPos = clearComedyMobAStartPos_ + clearParticleGlobalOffset_;

	pm->Emit("clearComedyWarp_core", mobAWarpPos, 4);
	pm->Emit("clearComedyWarp_ring", mobAWarpPos, 4);
	pm->Emit("clearComedyWarp_streak", mobAWarpPos, 44);
	pm->Emit("clearComedyWarp_spark", mobAWarpPos, 28);
	pm->Emit("clearComedyWarp_glitter", mobAWarpPos, 18);

	/// ──────────────── コミカル逃走用 雑魚B生成 ───────────────
	clearComedyMobB_ = std::make_unique<Enemy>();
	clearComedyMobB_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// クリアシーン用の参照設定
	clearComedyMobB_->SetCamera(camera_.get());
	clearComedyMobB_->SetParentScene(this);

	// 雑魚Bの見た目設定
	clearComedyMobB_->SetModel("jerryfish.obj");
	clearComedyMobB_->SetTentacleModel("tentacle.obj");

	// 雑魚Bの配置設定
	clearComedyMobB_->SetPosition(clearComedyMobBStartPos_);
	clearComedyMobB_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
	clearComedyMobB_->SetScale({ 1.2f, 1.2f, 1.2f });

	// 演出制御用に通常挙動を止める
	clearComedyMobB_->SetLocked(true);
	clearComedyMobB_->SyncTransform();

	// 雑魚B出現位置にワープ演出を出す
	Vector3 mobBWarpPos = clearComedyMobBStartPos_ + clearParticleGlobalOffset_;

	pm->Emit("clearComedyWarp_core", mobBWarpPos, 4);
	pm->Emit("clearComedyWarp_ring", mobBWarpPos, 4);
	pm->Emit("clearComedyWarp_streak", mobBWarpPos, 44);
	pm->Emit("clearComedyWarp_spark", mobBWarpPos, 28);
	pm->Emit("clearComedyWarp_glitter", mobBWarpPos, 18);

	/// ──────────────── コミカル逃走演出フラグ初期化 ───────────────
	// 以降、同じ敵を重複生成しない
	clearComedyActorsSpawned_ = true;

	// 各演出の一回再生フラグを初期化
	clearComedyMobBFallEffectPlayed_ = false;
	clearComedyMobBSlipEffectPlayed_ = false;
	clearComedyNoticeMarkPlayed_ = false;
}	