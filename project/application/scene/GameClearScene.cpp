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
	// パーティクルマネージャーを取得する
	auto* pm = TKM::ParticleManager::GetInstance();

	// パーティクルマネージャーが無ければ発生できない
	if (!pm) { return; }

	// 爆発の中心光を発生させる
	pm->Emit("titleExplode_core", pos, 28);

	// 放射状の光線を発生させる
	pm->Emit("titleExplode_rays", pos, 140);

	// 爆発の破片粒を発生させる
	pm->Emit("titleExplode_debris", pos, 90);

	// 衝撃波リングを発生させる
	pm->Emit("titleExplode_ring", pos, 2);
}

void GameClearScene::Initialize() {
	//=========================================================
	// 音声読み込み
	//=========================================================

	// リザルト系で使う音声を読み込む
	AudioCatalog::LoadResultAudios();

	//=========================================================
	// モデル・テクスチャ読み込み
	//=========================================================

	// モデルカタログを読み込む
	ModelCatalog::LoadModelCatalogs(dxCommon_);

	// テクスチャカタログを読み込む
	TextureCatalog::LoadTextureCatalogs();

	//=========================================================
	// カメラ初期化
	//=========================================================

	// クリアシーン用カメラを生成する
	camera_ = std::make_unique<TKM::Camera>();

	// カメラの開始回転を設定する
	camera_->SetRotate(cameraStartRot_);

	// カメラの開始位置を設定する
	camera_->SetTranslate(cameraStartPos_);

	// カメラ行列を更新する
	camera_->Update();

	// カメラ移動時間をリセットする
	cameraMoveTime_ = 0.0f;

	//=========================================================
	// パーティクル初期化
	//=========================================================

	// 既存のパーティクルグループをすべて消す
	ParticleManager::GetInstance()->ClearAllGroups();

	// パーティクルマネージャーを初期化する
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());

	// このシーンで使うパーティクルグループを登録する
	ParticleGroupsCatalog::RegisterScene(ParticleManager::GetInstance());

	//=========================================================
	// ライト初期化
	//=========================================================

	// 平行光源を生成する
	dirLight_ = std::make_unique<DirectionalLight>();

	// 平行光源の色・方向・強さを設定する
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	//=========================================================
	// スカイボックス初期化
	//=========================================================

	// スカイボックスを生成する
	skybox_ = std::make_unique<Skybox>();

	// スカイボックスを初期化する
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");

	// スカイボックスへカメラを設定する
	skybox_->SetCamera(camera_.get());

	//=========================================================
	// 自機初期化
	//=========================================================

	// クリア演出用のプレイヤーを生成する
	player_ = std::make_unique<Player>();

	// プレイヤーを初期化する
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// プレイヤーへカメラを設定する
	player_->SetCamera(camera_.get());

	// クリア演出中は操作を無効にする
	player_->SetControlEnabled(false);

	// クリア演出ではレティクルを表示しない
	player_->SetReticleVisible(false);

	// クリア演出用にジェットスモークを有効にする
	player_->SetEnableJetSmoke(true);

	// クリア画面での表示位置を設定する
	player_->SetPosition(playerDisplayPos_);

	// クリア画面での表示回転を設定する
	player_->SetRotation(playerDisplayRot_);

	// 機体演出用時間をリセットする
	planeTime_ = 0.0f;

	//=========================================================
	// GAME CLEAR スプライト初期化
	//=========================================================

	// GAME CLEAR表示用スプライトを生成する
	clearSprite_ = std::make_unique<Sprite>();

	// clear.pngでスプライトを初期化する
	clearSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/clear.png");

	// 中心基準で配置・演出できるようにする
	clearSprite_->SetAnchorPoint({ 0.5f, 0.5f });

	// 演出開始位置へ配置する
	clearSprite_->SetPosition(clearSpriteStartPos_);

	// 初期色を白・不透明にする
	clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	//=========================================================
	// 画面遷移アイリス初期化
	//=========================================================

	// 画面中央のアイリススプライトを生成する
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMaxScale_);

	// 初期状態では画面を覆うサイズにする
	irisScale_ = irisMaxScale_;

	// アイリスサイズを反映する
	iris_->SetSize({ irisScale_, irisScale_ });

	// 入場用のアイリスオープンTweenを設定する
	irisOpenTween_.Reset(
		/*start*/ irisMaxScale_,
		/*end*/   0.0f,
		/*sec*/   0.8f,
		Ease::Type::OutBack
	);

	// 入場アイリス中にする
	irisOpening_ = true;

	// 退場アイリスはまだ開始していない状態にする
	irisClosing_ = false;

	//=========================================================
	// クリアメニュー初期化
	//=========================================================

	// リザルトメニューを生成する
	clearMenu_ = std::make_unique<GameResultMenuController>();

	// リザルトメニューを初期化する
	clearMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		static_cast<float>(WindowsAPI::GetClientWidth()),
		static_cast<float>(WindowsAPI::GetClientHeight())
	);

	//=========================================================
	// 決定時の水面波紋エフェクト初期化
	//=========================================================

	// 水面波紋エフェクトを生成する
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();

	// 水面波紋エフェクトを初期化する
	rippleEffect_->Initialize(dxCommon_);

	// DirectXCommonへ水面波紋エフェクトを登録する
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());

	//=========================================================
	// ポストエフェクト初期化
	//=========================================================

	// ポストエフェクトコントローラーを生成する
	postFx_ = std::make_unique<TKM::PostEffectController>();

	// ポストエフェクトを初期化する
	postFx_->Initialize(dxCommon_, player_.get(), nullptr);

	// カメラ演出中のブラー解除フラグを初期化する
	blurReleased_ = false;

	//=========================================================
	// コミカル逃走演出初期化
	//=========================================================

	// 転倒スロー要求フラグを初期化する
	clearComedyFallSlowRequested_ = false;

	// コミカル演出用タイムスケールを初期化する
	clearComedyTimeScale_.Initialize();

	// コミカル逃走演出用のボス設定を行う
	SetupClearComedyBossConfig_();

	// コミカル逃走用ステートマシンを初期化する
	clearComedySM_.Initialize(this);
}

void GameClearScene::Finalize() {
	// DirectXCommonから水面波紋エフェクトの参照を外す
	dxCommon_->SetWaterRippleEffect(nullptr);

	// ポストエフェクトの終了処理を行う
	postFx_->Finalize();

	// オーディオマネージャーの終了処理を行う
	AudioManager::GetInstance()->Finalize();
}

void GameClearScene::Update() {
	// 入力状態を更新する
	Input::GetInstance()->Update();

	//=========================================================
	// 水面波紋エフェクト更新
	//=========================================================

	// 決定時などに発生した水面波紋を更新する
	rippleEffect_->Update(dt_);

	//=========================================================
	// アイリス開き更新
	//=========================================================

	// 入場アイリス中なら開き演出を進める
	if (irisOpening_) {
		// Tweenに合わせてアイリスサイズを更新する
		irisScale_ = UpdateIrisScale(iris_.get(), irisOpenTween_, dt_);

		// 開き演出が終わったら入場完了にする
		if (irisOpenTween_.Finished()) {
			irisOpening_ = false;
		}
	}

	//=========================================================
	// クリアメニュー更新
	//=========================================================

	// アイリス中でなく、メニュー表示中なら入力を受け付ける
	if (!irisClosing_ && !irisOpening_ && isClearMenuVisible_) {
		// メニュー更新結果を受け取る
		const auto cmd = clearMenu_->Update(dt_);

		// リスタートが選ばれた場合
		if (cmd == GameResultMenuController::Command::Restart) {
			// 次の動作をリスタートにする
			nextAction_ = NextAction::Restart;

			// アイリス閉じを開始する
			irisClosing_ = true;

			// アイリス閉じTweenを開始する
			irisCloseTween_.Reset(/*start*/ 0.0f, /*end*/ irisMaxScale_, /*sec*/ 0.8f, Ease::Type::InBack);
		} else if (cmd == GameResultMenuController::Command::ReturnToTitle) {
			// 次の動作をタイトルへ戻るにする
			nextAction_ = NextAction::ReturnToTitle;

			// アイリス閉じを開始する
			irisClosing_ = true;

			// アイリス閉じTweenを開始する
			irisCloseTween_.Reset(/*start*/ 0.0f, /*end*/ irisMaxScale_, /*sec*/ 0.8f, Ease::Type::InBack);
		}
	}

	//=========================================================
	// アイリス閉じ更新
	//=========================================================

	// アイリス閉じ中なら遷移演出とシーン切り替えだけ行う
	if (irisClosing_) {
		// Tweenに合わせてアイリスサイズを更新する
		irisScale_ = UpdateIrisScale(iris_.get(), irisCloseTween_, dt_);

		// 閉じ演出が終わったら次のシーンへ進む
		if (irisCloseTween_.Finished()) {
			// リスタートならGameSceneへ戻る
			if (nextAction_ == NextAction::Restart) {
				sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_));
				return;
			}

			// それ以外はタイトルへ戻る
			sceneManager_->SetNextScene(std::make_unique<TitleScene>(dxCommon_, srvManager_));
			return;
		}
	}

	//=========================================================
	// カメラ演出更新
	//=========================================================

	// カメライントロが有効なら、開始位置から終了位置へ移動させる
	if (enableCameraIntro_) {
		// カメラ移動時間を進める
		cameraMoveTime_ += dt_;

		// カメラ移動の進行率を計算する
		float t = cameraMoveTime_ / cameraMoveDuration_;

		// 進行率を0〜1に収める
		t = std::clamp(t, 0.0f, 1.0f);

		// 位置用イージングを評価する
		float posT = Ease::Eval(cameraPosEaseType_, t);

		// 回転用イージングを評価する
		float rotT = Ease::Eval(cameraRotEaseType_, t);

		// カメラ位置を補間する
		Vector3 camPos = MyMath::Vector3Lerp(cameraStartPos_, cameraEndPos_, posT);

		// カメラ回転を補間する
		Vector3 camRot = MyMath::Vector3Lerp(cameraStartRot_, cameraEndRot_, rotT);

		// カメラ位置を反映する
		camera_->SetTranslate(camPos);

		// カメラ回転を反映する
		camera_->SetRotate(camRot);

		// カメラ移動中はGAME CLEARスプライトを非表示にする
		isClearSpriteVisible_ = false;

		//=====================================================
		// カメラブラー制御
		//=====================================================

		// ポストエフェクトがある場合だけブラーを制御する
		if (postFx_) {
			// まだブラーを解除していない間だけ判定する
			if (!blurReleased_) {
				// 終点へ到達するまでは放射ブラーを有効にする
				if (posT < 1.0f) {
					postFx_->SetRadialBlurManual(true, cameraBlurStrength_);
				} else {
					// 終点に到達したらブラーを解除する
					postFx_->SetRadialBlurManual(false, 0.0f);
					blurReleased_ = true;
				}
			} else {
				// 解除後は常にブラーをOFFにする
				postFx_->SetRadialBlurManual(false, 0.0f);
			}
		}

		//=====================================================
		// クリア祝福パーティクル更新
		//=====================================================

		{
			// a〜bの範囲のランダム値を返す
			auto frand = [](float a, float b) {
				return a + (b - a) * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
				};

			// 中盤で最大になる山なり強度を作る
			float peak = std::sin(t * MyMath::GetPI());

			// 強度を0〜1に収める
			peak = std::clamp(peak, 0.0f, 1.0f);

			// 祝福演出の中心位置を作る
			Vector3 celebrateCenter = playerDisplayPos_ + clearCelebrateOffset_ + clearParticleGlobalOffset_;

			// 各祝福パーティクルの発生タイマーを進める
			celebrateCoreTimer_ += dt_;
			celebrateSparkTimer_ += dt_;
			celebrateRayTimer_ += dt_;

			// 中心光の発生間隔を強度に応じて短くする
			float coreInterval = MyMath::Lerp(0.28f, 0.10f, peak);

			// スパークの発生間隔を強度に応じて短くする
			float sparkInterval = MyMath::Lerp(0.08f, 0.015f, peak);

			// レイの発生間隔を強度に応じて短くする
			float rayInterval = MyMath::Lerp(0.22f, 0.07f, peak);

			// 中心光の発生タイミングになったら出す
			if (celebrateCoreTimer_ >= coreInterval) {
				// タイマーをリセットする
				celebrateCoreTimer_ = 0.0f;

				// ランダムな発生位置を作る
				Vector3 p = celebrateCenter + Vector3{
					frand(-8.0f, 8.0f),
					frand(-2.0f, 6.0f),
					frand(-6.0f, 6.0f)
				};

				// 祝福コアを発生させる
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_core", p, 1);
			}

			// スパークの発生タイミングになったら出す
			if (celebrateSparkTimer_ >= sparkInterval) {
				// タイマーをリセットする
				celebrateSparkTimer_ = 0.0f;

				// ランダムな発生位置を作る
				Vector3 p = celebrateCenter + Vector3{
					frand(-16.0f, 16.0f),
					frand(-6.0f, 10.0f),
					frand(-12.0f, 12.0f)
				};

				// 強度に応じて発生数を増やす
				int count = static_cast<int>(MyMath::Lerp(8.0f, 22.0f, peak));

				// 祝福スパークを発生させる
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_spark", p, count);

				// タイトル爆発風演出を混ぜる確率を決める
				float explodeChance = MyMath::Lerp(0.03f, 0.10f, peak);

				// 一定確率でタイトル爆発風演出を混ぜる
				if (frand(0.0f, 1.0f) < explodeChance) {
					EmitTitleExplodeLike_(p);
				}
			}

			// レイの発生タイミングになったら出す
			if (celebrateRayTimer_ >= rayInterval) {
				// タイマーをリセットする
				celebrateRayTimer_ = 0.0f;

				// ランダムな発生位置を作る
				Vector3 p = celebrateCenter + Vector3{
					frand(-12.0f, 12.0f),
					frand(-4.0f, 8.0f),
					frand(-10.0f, 10.0f)
				};

				// 強度に応じて発生数を決める
				int count = static_cast<int>(MyMath::Lerp(1.0f, 3.0f, peak));

				// 祝福レイを発生させる
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_ray", p, count);
			}

			// カメラが終点に到達した瞬間だけ最大祝福バーストを出す
			if (!celebrateFinalBurstDone_ && posT >= 1.0f) {
				// 二重発生しないようにフラグを立てる
				celebrateFinalBurstDone_ = true;

				// 祝福コアをまとめて発生させる
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_core", celebrateCenter, 10);

				// 祝福スパークをまとめて発生させる
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_spark", celebrateCenter, 70);

				// 祝福レイをまとめて発生させる
				TKM::ParticleManager::GetInstance()->Emit("clearCelebrate_ray", celebrateCenter, 16);

				// タイトル爆発風演出も合わせて発生させる
				EmitTitleExplodeLike_(celebrateCenter);
			}
		}

		//=====================================================
		// カメラ演出終了判定
		//=====================================================

		// カメラ移動が最後まで到達したら通常状態へ移る
		if (t >= 1.0f) {
			// カメラ演出を終了する
			enableCameraIntro_ = false;

			// GAME CLEARスプライトはまだ出さない
			isClearSpriteVisible_ = false;

			// GAME CLEARポップ演出もまだ再生しない
			isClearSpritePopPlaying_ = false;

			// GAME CLEARポップ時間を初期化する
			clearSpritePopTime_ = 0.0f;

			// GAME CLEARスプライトを開始位置に戻す
			clearSprite_->SetPosition(clearSpriteStartPos_);

			// GAME CLEARスプライト色を白・不透明に戻す
			clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

			// メニューはまだ表示しない
			isClearMenuVisible_ = false;

			// コミカル逃走演出ステートが無ければ待機ステートへ入る
			if (clearComedySM_.GetState() == nullptr) {
				clearComedySM_.Change(std::make_unique<ClearComedyWaitAfterClearState>());
			}
		}
	} else {
		// カメラ演出終了後はカメラ位置を終了位置に固定する
		camera_->SetTranslate(cameraEndPos_);

		// カメラ演出終了後はカメラ回転を終了回転に固定する
		camera_->SetRotate(cameraEndRot_);

		// ブラーは常にOFFにする
		if (postFx_) {
			postFx_->SetRadialBlurManual(false, 0.0f);
		}
	}

	// カメラ行列を更新する
	camera_->Update();

	// パーティクル描画用カメラを更新する
	ParticleManager::GetInstance()->SetCamera(camera_.get());

	//=========================================================
	// ライト更新
	//=========================================================

	// 平行光源を更新する
	dirLight_->Update();

	//=========================================================
	// ポストエフェクト更新
	//=========================================================

	// ポストエフェクトを更新する
	postFx_->Update(dt_, nullptr);

	//=========================================================
	// パーティクル更新
	//=========================================================

	// パーティクルマネージャーを更新する
	TKM::ParticleManager::GetInstance()->Update(dt_);

	//=========================================================
	// デバッグ用 GAME CLEAR バースト常時発生
	//=========================================================

	// デバッグ発生が有効なら一定間隔でバーストを出す
	if (debugEmitClearBannerBurst_) {
		// デバッグ発生タイマーを進める
		debugEmitClearBannerBurstTimer_ += dt_;

		// 発生間隔に到達したらバーストを出す
		if (debugEmitClearBannerBurstTimer_ >= debugEmitClearBannerBurstInterval_) {
			// タイマーをリセットする
			debugEmitClearBannerBurstTimer_ = 0.0f;

			// バースト発生位置を作る
			Vector3 burstPos = playerDisplayPos_ + clearBannerBurstOffset_;

			// パーティクルマネージャーを取得する
			auto* pm = TKM::ParticleManager::GetInstance();

			// バースト中心を発生させる
			pm->Emit("clearBannerBurst_core", burstPos, 6);

			// 紙吹雪を発生させる
			pm->Emit("clearBannerBurst_confetti", burstPos, 70);

			// レイを発生させる
			pm->Emit("clearBannerBurst_ray", burstPos, 30);
		}
	}

	//=========================================================
	// Skybox回転
	//=========================================================

	// 2π定数
	constexpr float kTwoPi = 6.2831853f;

	// X軸回転を進める
	skyPitch_ -= skyRotSpeedX_;

	// 上限を超えたら一周戻す
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;

	// 下限を超えたら一周進める
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	// スカイボックスのX軸回転を反映する
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	//=========================================================
	// 自機クリア演出更新
	//=========================================================

	// プレイヤーをクリア画面用の表示位置に固定する
	player_->SetPosition(playerDisplayPos_);

	// プレイヤーをクリア画面用の表示回転に固定する
	player_->SetRotation(playerDisplayRot_);

	// 入力やゲーム処理は行わず、見た目だけ更新する
	player_->UpdateVisualOnly(dt_);

	//=========================================================
	// クリアシーン ライブ風ファイアー柱
	//=========================================================

	// ファイアー柱が有効なら一定間隔で噴射する
	if (clearStageFireActive_) {
		// ファイアー発生タイマーを進める
		clearStageFireTimer_ += dt_;

		// ファイアー発生間隔
		const float kFireInterval = 0.045f;

		// 発生タイミングになったら炎柱を出す
		if (clearStageFireTimer_ >= kFireInterval) {
			// タイマーをリセットする
			clearStageFireTimer_ = 0.0f;

			// パーティクルマネージャーを取得する
			auto* pm = TKM::ParticleManager::GetInstance();

			// 登録済みの各ファイアー位置から噴射する
			for (const Vector3& firePos : clearStageFirePositions_) {
				// 全体オフセットを加えた発生位置を作る
				Vector3 emitPos = firePos + clearParticleGlobalOffset_;

				// 炎柱本体を発生させる
				pm->Emit("clearStageFire_column", emitPos, 3);

				// 炎柱上部の光を発生させる
				pm->Emit("clearStageFire_top", emitPos, 2);
			}
		}
	}

	//=========================================================
	// GAME CLEAR スプライト表示演出
	//=========================================================

	// ポップ演出中なら位置を補間する
	if (isClearSpritePopPlaying_) {
		// ポップ演出時間を進める
		clearSpritePopTime_ += dt_;

		// ポップ演出の進行率を計算する
		float t = clearSpritePopTime_ / clearSpritePopDuration_;

		// 進行率を0〜1に収める
		t = Ease::Clamp01(t);

		// OutBackで勢いのある移動にする
		float moveT = Ease::OutBack(t);

		// 開始位置から中央位置へ補間する
		Vector2 pos = {
			MyMath::Lerp(clearSpriteStartPos_.x, clearSpriteCenterPos_.x, moveT),
			MyMath::Lerp(clearSpriteStartPos_.y, clearSpriteCenterPos_.y, moveT)
		};

		// スプライト位置を反映する
		clearSprite_->SetPosition(pos);

		// 演出が終わったら中央位置に固定する
		if (t >= 1.0f) {
			isClearSpritePopPlaying_ = false;
			clearSprite_->SetPosition(clearSpriteCenterPos_);
		}
	}

	// GAME CLEARスプライトを更新する
	clearSprite_->Update();

	//=========================================================
	// コミカル逃走演出更新
	//=========================================================

	// コミカル逃走ステートマシンを更新する
	clearComedySM_.Update(dt_);

	//=========================================================
	// パフォーマンス情報更新
	//=========================================================

	// FPSやメモリなどの情報を更新する
	UpdatePerformanceInfo();

	//=========================================================
	// ImGuiデバッグ更新
	//=========================================================

	// デバッグUIを更新する
	ImGuiDebug();
}

void GameClearScene::Draw() {
	//=========================================================
	// 背景描画
	//=========================================================

	// スカイボックスを描画する
	skybox_->Draw();

	//=========================================================
	// 3D描画
	//=========================================================

	// 3D描画共通設定を行う
	Object3dCommon::GetInstance()->DrawSetCommon();

	// プレイヤーを描画する
	player_->Draw(dxCommon_);

	// コミカル逃走演出のボスを描画する
	if (clearComedyBoss_) { clearComedyBoss_->Draw(dxCommon_); }

	// コミカル逃走演出の雑魚Aを描画する
	if (clearComedyMobA_) { clearComedyMobA_->Draw(dxCommon_); }

	// コミカル逃走演出の雑魚Bを描画する
	if (clearComedyMobB_) { clearComedyMobB_->Draw(dxCommon_); }

	//=========================================================
	// パーティクル描画
	//=========================================================

	// パーティクルを描画する
	TKM::ParticleManager::GetInstance()->Draw();

	//=========================================================
	// 2Dスプライト描画
	//=========================================================

	// 2Dスプライト描画共通設定を行う
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	// GAME CLEAR表示中なら描画する
	if (isClearSpriteVisible_) {
		clearSprite_->Draw();
	}

	// アイリスは一番手前に描画する
	if ((irisOpening_ || irisClosing_)) {
		iris_->Draw();
	}

	// クリアメニュー表示中なら描画する
	if (isClearMenuVisible_) {
		clearMenu_->Draw();
	}
}

void GameClearScene::ImGuiDebug() {
#ifdef USE_IMGUI
	// プレイヤー情報ウィンドウを開始する
	ImGui::Begin("プレイヤー情報");

	// プレイヤー表示位置・回転の調整項目を表示する
	ImGui::Text("位置調整");

	// プレイヤー表示位置を調整する
	ImGui::DragFloat3("表示位置", &playerDisplayPos_.x, 0.05f);

	// プレイヤー表示回転を調整する
	ImGui::DragFloat3("表示回転", &playerDisplayRot_.x, 0.01f);

	// 区切り線を表示する
	ImGui::Separator();

	// クリア演出パーティクル全体補正項目を表示する
	ImGui::Text("クリア演出パーティクル全体補正");

	// パーティクル全体発生位置のオフセットを調整する
	ImGui::DragFloat3("全体発生オフセット", &clearParticleGlobalOffset_.x, 0.05f);

	// 区切り線を表示する
	ImGui::Separator();

	// GAME CLEARバースト調整項目を表示する
	ImGui::Text("GAME CLEARバースト調整");

	// バースト発生位置の補正値を調整する
	ImGui::DragFloat3("バースト位置補正", &clearBannerBurstOffset_.x, 0.05f);

	// 常時発生間隔を調整する
	ImGui::DragFloat("常時発生間隔", &debugEmitClearBannerBurstInterval_, 0.01f, 0.01f, 5.0f);

	// 常時発生ON/OFFを切り替える
	ImGui::Checkbox("常時発生", &debugEmitClearBannerBurst_);

	// 1回だけバーストを発生させるボタン
	if (ImGui::Button("1回発生")) {
		// バースト発生位置を作る
		Vector3 burstPos = playerDisplayPos_ + clearBannerBurstOffset_;

		// パーティクルマネージャーを取得する
		auto* pm = TKM::ParticleManager::GetInstance();

		// バースト中心を発生させる
		pm->Emit("clearBannerBurst_core", burstPos, 6);

		// 紙吹雪を発生させる
		pm->Emit("clearBannerBurst_confetti", burstPos, 70);

		// レイを発生させる
		pm->Emit("clearBannerBurst_ray", burstPos, 30);
	}

	// プレイヤー情報ウィンドウを閉じる
	ImGui::End();

	// パフォーマンス情報デバッグを表示する
	ImGuiDebugInfo();
#endif
}

void GameClearScene::SetupClearComedyBossConfig_() {
	// ボス設定を初期化する
	clearComedyBossConfig_ = BossEnemyConfig{};

	// コミカル逃走用ボスモデルを設定する
	clearComedyBossConfig_.model_ = "jerryfish_boss.obj";

	// コミカル逃走用ボス触手モデルを設定する
	clearComedyBossConfig_.tentacleModel_ = "tentacle_boss.obj";

	// コミカル逃走用なのでHPを1にする
	clearComedyBossConfig_.hp_ = 1;

	// コミカル逃走用ボスの表示スケールを設定する
	clearComedyBossConfig_.scale_ = { 2.3f, 2.3f, 2.3f };
}

void GameClearScene::SpawnClearComedyActors_() {
	// すでに生成済みなら二重生成しない
	if (clearComedyActorsSpawned_) {
		return;
	}

	// パーティクルマネージャーを取得する
	auto* pm = TKM::ParticleManager::GetInstance();

	//=========================================================
	// コミカル逃走用ボス生成
	//=========================================================

	// ボスを生成する
	clearComedyBoss_ = std::make_unique<BossEnemy>();

	// コミカル逃走用ボス設定を渡す
	clearComedyBoss_->SetConfig(&clearComedyBossConfig_);

	// ボスを初期化する
	clearComedyBoss_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// ボスへカメラを設定する
	clearComedyBoss_->SetCamera(camera_.get());

	// ボスへ親シーンを設定する
	clearComedyBoss_->SetParentScene(this);

	// ボスを開始位置に配置する
	clearComedyBoss_->SetPosition(clearComedyBossStartPos_);

	// ボスを正面向きにする
	clearComedyBoss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

	// 演出用にロック状態にする
	clearComedyBoss_->SetLocked(true);

	// 生成直後のTransformを同期する
	clearComedyBoss_->SyncTransform();

	// ボスのワープ演出位置を作る
	Vector3 bossWarpPos = clearComedyBossStartPos_ + clearParticleGlobalOffset_;

	// ボスのワープ中心光を発生させる
	pm->Emit("clearComedyWarp_core", bossWarpPos, 6);

	// ボスのワープリングを発生させる
	pm->Emit("clearComedyWarp_ring", bossWarpPos, 5);

	// ボスのワープストリークを発生させる
	pm->Emit("clearComedyWarp_streak", bossWarpPos, 64);

	// ボスのワープスパークを発生させる
	pm->Emit("clearComedyWarp_spark", bossWarpPos, 42);

	// ボスのワープ余韻を発生させる
	pm->Emit("clearComedyWarp_glitter", bossWarpPos, 28);

	//=========================================================
	// コミカル逃走用 雑魚A生成
	//=========================================================

	// 雑魚Aを生成する
	clearComedyMobA_ = std::make_unique<Enemy>();

	// 雑魚Aを初期化する
	clearComedyMobA_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// 雑魚Aへカメラを設定する
	clearComedyMobA_->SetCamera(camera_.get());

	// 雑魚Aへ親シーンを設定する
	clearComedyMobA_->SetParentScene(this);

	// 雑魚Aのモデルを設定する
	clearComedyMobA_->SetModel("jerryfish.obj");

	// 雑魚Aの触手モデルを設定する
	clearComedyMobA_->SetTentacleModel("tentacle.obj");

	// 雑魚Aを開始位置に配置する
	clearComedyMobA_->SetPosition(clearComedyMobAStartPos_);

	// 雑魚Aを正面向きにする
	clearComedyMobA_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

	// 雑魚Aのスケールを設定する
	clearComedyMobA_->SetScale({ 1.2f, 1.2f, 1.2f });

	// 演出用にロック状態にする
	clearComedyMobA_->SetLocked(true);

	// 生成直後のTransformを同期する
	clearComedyMobA_->SyncTransform();

	// 雑魚Aのワープ演出位置を作る
	Vector3 mobAWarpPos = clearComedyMobAStartPos_ + clearParticleGlobalOffset_;

	// 雑魚Aのワープ中心光を発生させる
	pm->Emit("clearComedyWarp_core", mobAWarpPos, 4);

	// 雑魚Aのワープリングを発生させる
	pm->Emit("clearComedyWarp_ring", mobAWarpPos, 4);

	// 雑魚Aのワープストリークを発生させる
	pm->Emit("clearComedyWarp_streak", mobAWarpPos, 44);

	// 雑魚Aのワープスパークを発生させる
	pm->Emit("clearComedyWarp_spark", mobAWarpPos, 28);

	// 雑魚Aのワープ余韻を発生させる
	pm->Emit("clearComedyWarp_glitter", mobAWarpPos, 18);

	//=========================================================
	// コミカル逃走用 雑魚B生成
	//=========================================================

	// 雑魚Bを生成する
	clearComedyMobB_ = std::make_unique<Enemy>();

	// 雑魚Bを初期化する
	clearComedyMobB_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// 雑魚Bへカメラを設定する
	clearComedyMobB_->SetCamera(camera_.get());

	// 雑魚Bへ親シーンを設定する
	clearComedyMobB_->SetParentScene(this);

	// 雑魚Bのモデルを設定する
	clearComedyMobB_->SetModel("jerryfish.obj");

	// 雑魚Bの触手モデルを設定する
	clearComedyMobB_->SetTentacleModel("tentacle.obj");

	// 雑魚Bを開始位置に配置する
	clearComedyMobB_->SetPosition(clearComedyMobBStartPos_);

	// 雑魚Bを正面向きにする
	clearComedyMobB_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

	// 雑魚Bのスケールを設定する
	clearComedyMobB_->SetScale({ 1.2f, 1.2f, 1.2f });

	// 演出用にロック状態にする
	clearComedyMobB_->SetLocked(true);

	// 生成直後のTransformを同期する
	clearComedyMobB_->SyncTransform();

	// 雑魚Bのワープ演出位置を作る
	Vector3 mobBWarpPos = clearComedyMobBStartPos_ + clearParticleGlobalOffset_;

	// 雑魚Bのワープ中心光を発生させる
	pm->Emit("clearComedyWarp_core", mobBWarpPos, 4);

	// 雑魚Bのワープリングを発生させる
	pm->Emit("clearComedyWarp_ring", mobBWarpPos, 4);

	// 雑魚Bのワープストリークを発生させる
	pm->Emit("clearComedyWarp_streak", mobBWarpPos, 44);

	// 雑魚Bのワープスパークを発生させる
	pm->Emit("clearComedyWarp_spark", mobBWarpPos, 28);

	// 雑魚Bのワープ余韻を発生させる
	pm->Emit("clearComedyWarp_glitter", mobBWarpPos, 18);

	// 生成済みフラグを立てる
	clearComedyActorsSpawned_ = true;

	// 雑魚B転倒エフェクト未再生状態にする
	clearComedyMobBFallEffectPlayed_ = false;

	// 雑魚Bスリップエフェクト未再生状態にする
	clearComedyMobBSlipEffectPlayed_ = false;

	// 気づきマーク未再生状態にする
	clearComedyNoticeMarkPlayed_ = false;
}