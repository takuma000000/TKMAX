#include "GameClearScene.h"
#include "Input.h"
#include "TitleScene.h"
#include "ImGuiManager.h"
#include "SceneManager.h"
#include "AudioCatalog.h"

#include "ModelManager.h"
#include "Object3dCommon.h"
#include "WindowsAPI.h"

#include <cmath>

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
	// ちょい見下ろしで中央を見る
	camera_->SetRotate({ 0.1f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 3.0f, -20.0f });
	camera_->Update();

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
	// 画面左外からスタート
	player_->SetPosition(planeStart_);
	// 正面(+Z)向きで開始
	player_->SetRotation({ 0.0f, 0.0f, 0.0f });
	// お祝いだからジェット噴射ON
	player_->SetEnableJetSmoke(true);
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
	// カメラ・ライト更新
	// ─────────────────────
	camera_->Update(); // カメラは特に動かないけど、行列更新は必要
	dirLight_->Update(); // ライトも特に動かないけど、行列更新は必要

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
	// 自機ジェットコースター演出
	// ─────────────────────
	planeTime_ += dt_;

	// 左画面外 → 右画面外 への進行度（0〜1）
	float rawT = planeTime_ / planeDuration_;
	float t = std::min(rawT, 1.0f);

	// 少しイージング（0→1がヌルっとなる）：t^2(3-2t) = smoothstep
	float tSmooth = t * t * (3.0f - 2.0f * t);

	// 左から右へ：Xだけは一方通行でスーッと抜ける
	Vector3 pos;
	pos.x = MyMath::Lerp(planeStart_.x, planeEnd_.x, tSmooth);

	// wave は 0〜1 をループさせて、何度も上下グルグルさせる
	float wave = std::fmod(rawT, 1.0f);
	if (wave < 0.0f) wave += 1.0f;

	// 上下：ちょっと大きめに跳ねさせて「喜んでる」感じ
	pos.y = 1.0f + std::sin(wave * MyMath::GetPI() * 4.0f) * 2.0f;
	// 奥行き：手前/奥にふわっと
	pos.z = planeStart_.z + std::cos(wave * MyMath::GetPI() * 2.0f) * 2.5f;

	// 回転：ロールを大きめに、ピッチも加えてぐるぐる
	float roll = std::sin(wave * MyMath::GetPI() * 6.0f) * 1.6f; // くるくる
	float pitch = std::cos(wave * MyMath::GetPI() * 3.0f) * 0.5f; // ちょい前後
	float yaw = std::sin(wave * MyMath::GetPI() * 2.0f) * 0.3f; // 少し左右にも振る

	// t が 1 を超えたら、さらに少しだけ右方向へ飛び出して画面外へ消えていく
	if (rawT > 1.0f) {
		float extra = (rawT - 1.0f) * 15.0f; // 右へさらに移動
		pos.x = planeEnd_.x + extra;
	}

	// GameClearScene が計算した「画面外→画面外」の軌道＆くるくる回転を反映
	player_->SetPosition(pos);
	player_->SetRotation({ pitch, yaw, roll });
	// ゲームプレイ処理なしで行列だけ更新する
	player_->UpdateVisualOnly(dt_);
	clearSprite_->Update();

	// メニューはアイリス開閉中は更新しない（操作できないようにするため）
	UpdatePerformanceInfo();
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