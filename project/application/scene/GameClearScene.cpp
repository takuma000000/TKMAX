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
	clearSprite_->SetColor({ 1,1,1,1 }); // 白で表示

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
}

void GameClearScene::Finalize() {
	dxCommon_->SetWaterRippleEffect(nullptr);

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
	if (enableCameraIntro_) { // ジェットコースター演出
		cameraMoveTime_ += dt_; // 移動開始からの経過時間を更新
		
		// tは0〜1の範囲で、移動開始から終了までの割合を表す
		float t = cameraMoveTime_ / cameraMoveDuration_;
		// 念のため0〜1にクランプ（オーバーしたら最後まで行ったことにする）
		t = std::clamp(t, 0.0f, 1.0f);

		// 位置は OutBack で少し通り過ぎて戻る
		float posT = Ease::Eval(cameraPosEaseType_, t);
		// 回転は OutSine で自然に止める
		float rotT = Ease::Eval(cameraRotEaseType_, t);

		Vector3 camPos = MyMath::Vector3Lerp(cameraStartPos_, cameraEndPos_, posT); // 線形補間でカメラ位置を計算
		Vector3 camRot = MyMath::Vector3Lerp(cameraStartRot_, cameraEndRot_, rotT); // 線形補間でカメラ回転を計算

		camera_->SetTranslate(camPos); // カメラ位置を設定
		camera_->SetRotate(camRot); // カメラ回転を設定
	} else { // ジェットコースター演出オフなら最初から最後の位置・回転
		camera_->SetTranslate(cameraEndPos_); // カメラ位置
		camera_->SetRotate(cameraEndRot_); // カメラ回転
	}
	camera_->Update();

	// ─────────────────────
	// ライト更新
	// ─────────────────────
	dirLight_->Update();

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

	// クリア表示はアイリス開閉中もずっと出てるので、スプライトも更新しておく
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
	clearSprite_->Draw();
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
