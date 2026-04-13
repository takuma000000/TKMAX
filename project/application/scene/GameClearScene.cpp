#include "GameClearScene.h"
#include "Input.h"
#include "TitleScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "AudioCatalog.h"
#include <cmath>
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "WindowsAPI.h"

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
	ModelManager::GetInstance()->LoadModel("turtle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("turtle_flipper.obj", dxCommon_);

	TextureManager::GetInstance()->LoadTexture("./resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/clear.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/restart_pause.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/title_pause.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/gradationLine.png");

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
	clearSprite_->SetPosition({ WindowsAPI::kClientWidth_ * 0.5f, WindowsAPI::kClientHeight_ * 0.5f }); // 画面中央に配置
	clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f }); // 最初は透明

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
		// カメラ演出終了判定
		// 「最後まで到達したら」通常状態へ
		// ─────────────────────
		if (t >= 1.0f) {
			enableCameraIntro_ = false; // カメラ演出終了
			isClearSpriteVisible_ = true; // カメラ演出が終わったらクリアスプライト表示ON
			isClearSpriteFadePlaying_ = true; // 表示演出開始
			clearSpriteFadeTime_ = 0.0f; // 演出時間リセット
			clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f }); // 演出開始時は透明
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
	postFx_->Update(dt_, nullptr);
	// ─────────────────────

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
	// クリアスプライト表示演出
	// サイズは触らず、アルファだけイージング
	// ─────────────────────
	if (isClearSpriteVisible_) { // カメラ演出が終わってから表示する
		// 演出中はアルファをイージングで変化させる
		if (isClearSpriteFadePlaying_) {
			clearSpriteFadeTime_ += dt_; // 演出時間を進める

			// 演出時間を0.0～1.0の範囲に正規化
			float t = clearSpriteFadeTime_ / clearSpriteFadeDuration_;
			t = std::clamp(t, 0.0f, 1.0f);
			// イージングでアルファ値を計算
			float alpha = Ease::Eval(clearSpriteFadeEaseType_, t);
			clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
			// 演出終了判定
			if (t >= 1.0f) {
				isClearSpriteFadePlaying_ = false;
				clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			}
		} else {
			clearSprite_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // 演出が終わったら完全に不透明
		}
	}
	clearSprite_->Update();

	// メニューはアイリス開閉中は更新しない（操作できないようにするため）
	UpdatePerformanceInfo();

	// ─────────────────────
	// ImGuiデバッグ
	// ─────────────────────
	ImGuiDebug();
}

void GameClearScene::Draw() {
	// --- 3D ---
	Object3dCommon::GetInstance()->DrawSetCommon();
	player_->Draw(dxCommon_);
	skybox_->Draw();

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
