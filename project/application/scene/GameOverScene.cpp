#include "GameOverScene.h"
#include "TextureManager.h"
#include "Input.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "TitleScene.h"
#include <SkyBox.h>
#include "GameScene.h"
#include <cmath>
#include <cstdlib>

using namespace TKM;

namespace {
	float RandomRange(float minValue, float maxValue) {
		const float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		return minValue + (maxValue - minValue) * t;
	}
}

void GameOverScene::Initialize() {
	ModelManager::GetInstance()->LoadModel("turtle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("turtle_flipper.obj", dxCommon_);
	TextureManager::GetInstance()->LoadTexture("./resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/over.png");

	// --- カメラ ---
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->Update();

	// ライト
	dirLight_ = std::make_unique<DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	// パーティクル
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());
	ParticleManager::GetInstance()->CreateParticleGroup(
		"fallStreak",
		"./resources/texture/circle2.png",
		ParticleManager::ParticleType::NORMAL
	);

	// --- Boss ---
	boss_ = std::make_unique<BossEnemy>();
	boss_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	boss_->SetCamera(camera_.get());

	bossBasePos_ = { 0.0f, 2.0f, 42.0f };
	bossPos_ = bossBasePos_;
	bossRot_ = { 0.0f, 3.14f, 0.0f };

	boss_->SetPosition(bossPos_);
	boss_->SetRotate(bossRot_);
	boss_->SetScale({ 3.5f, 3.5f, 3.5f });
	boss_->SetLocked(true);
	boss_->SyncTransform();

	cameraBaseTranslate_ = { 0.0f, 2.2f, -13.5f };
	camera_->SetTranslate(cameraBaseTranslate_);
	camera_->SetRotate({ 0.04f, 0.0f, 0.0f });
	camera_->Update();

	// --- skybox ---
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());
	skybox_->SetColor({ 1.0f, 0.0f, 0.5f, 1.0f });

	// --- Iris（Title / GameScene と同一仕様・共通ユーティリティ版）---

	// 画面中央に iris を作成し、irisMaxScale_（画面を覆える最大サイズ）を計算
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMaxScale_, "./resources/texture/circle2.png");

	// 入場は「覆った状態 → 0」へ（OutBack, 0.8s）
	irisScale_ = irisMaxScale_;
	iris_->SetSize({ irisScale_, irisScale_ });

	// Tween 初期化（覆った状態 → 0）
	irisOpenTween_.Reset(
		irisMaxScale_,         // start（画面全体を覆っている状態）
		0.0f,                  // end   （完全に開いた状態）
		kIrisDuration_,         // 時間（元コードをそのまま使用）
		Ease::Type::OutBack    // 開くときの跳ね返り感
	);

	// スプライト生成
	overSprite_ = std::make_unique<Sprite>();
	overSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/over.png");
	overSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	overSprite_->SetPosition({ WindowsAPI::kClientWidth_ * 0.5f, WindowsAPI::kClientHeight_ * 0.5f });
	// アルファ0で開始（見えない状態）
	overSprite_->SetColor({ 1, 1, 1, 0 });
	overActive_ = true;
	overAlpha_ = 1.0f;
	overScale_ = 1.0f;
	overGlowTimer_ = 0.0f;

	overAlphaTween_.Reset(1.0f, 1.0f, 0.01f, Ease::Type::Linear);
	overScaleTween_.Reset(1.0f, 1.0f, 0.01f, Ease::Type::Linear);

	// フェードイン（0→1, 0.7秒, OutQuad）& スケール（0.8→1.0, 0.7秒, OutBack）
	overAlphaTween_.Reset(0.0f, 1.0f, 0.7f, Ease::Type::OutQuad); // フェードイン
	overScaleTween_.Reset(0.8f, 1.0f, 0.7f, Ease::Type::OutBack); // スケールアップ

	overActive_ = true; // アニメ進行フラグON

	// --- ゲームオーバー後のメニュー ---
	overMenu_ = std::make_unique<GameResultMenuController>();
	overMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		WindowsAPI::kClientWidth_,
		WindowsAPI::kClientHeight_
	);

	// ノイズ
	noiseEffect_ = std::make_unique<TKM::NoiseEffect>();
	noiseEffect_->Initialize(dxCommon_);
	noiseEffect_->SetActive(true);
	noiseEffect_->SetIntensity(0.20f);
	noiseEffect_->SetFlash(0.04f);
	// 1回目も少しランダムにする
	noiseNextInterval_ = RandomRange(0.02f, 0.8f); // 次のノイズ発生までの時間
}

void GameOverScene::Finalize() {
	TKM::ParticleManager::GetInstance()->ClearGroup("fallStreak");
}

void GameOverScene::Update() {
	Input::GetInstance()->Update();

	bossAnimTimer_ += dt_;
	bossJumpTimer_ += dt_;

	// -------------------------------------------------
	// ボス常時アニメ（喜んでるっぽい浮遊・揺れ）
	// -------------------------------------------------
	float idleFloatY =
		std::sinf(bossAnimTimer_ * 2.6f) * 0.55f +
		std::sinf(bossAnimTimer_ * 5.4f + 0.7f) * 0.18f;

	float idleSwingX =
		std::sinf(bossAnimTimer_ * 1.8f) * 0.9f;

	float idleRotZ =
		std::sinf(bossAnimTimer_ * 2.1f) * 0.08f +
		std::sinf(bossAnimTimer_ * 4.8f + 0.4f) * 0.03f;

	float idleRotY =
		3.14159265f + std::sinf(bossAnimTimer_ * 1.1f) * 0.08f;

	bossPos_ = bossBasePos_;
	bossPos_.x += idleSwingX;
	bossPos_.y += idleFloatY;

	bossRot_ = { 0.0f, idleRotY, idleRotZ };

	// -------------------------------------------------
	// 定期ジャンプ開始
	// -------------------------------------------------
	if (!bossJumping_ && bossJumpTimer_ >= bossJumpInterval_) {
		bossJumping_ = true;
		bossJumpTimer_ = 0.0f;
		bossJumpElapsed_ = 0.0f;
		bossLandingShakeTriggered_ = false;
	}

	// -------------------------------------------------
	// ジャンプ中処理
	// -------------------------------------------------
	if (bossJumping_) {
		bossJumpElapsed_ += dt_;
		float t = bossJumpElapsed_ / bossJumpDuration_;
		if (t > 1.0f) {
			t = 1.0f;
		}

		// 放物線っぽいジャンプ
		float jumpArc = 4.0f * t * (1.0f - t);
		bossPos_.y += jumpArc * bossJumpHeight_;

		// ジャンプ中は少しテンション高めに傾ける
		bossRot_.z += std::sinf(t * 3.14159265f) * 0.10f;

		// 着地直前で触手を強める
		float panic01 = (t < 0.55f) ? 0.35f : 0.85f;
		boss_->SetIntroPanic(true, panic01);

		// 着地シェイク
		if (!bossLandingShakeTriggered_ && t >= 0.92f) {
			bossLandingShakeTriggered_ = true;
			cameraShakeTimer_ = cameraShakeDuration_;
		}

		if (bossJumpElapsed_ >= bossJumpDuration_) {
			bossJumping_ = false;
			bossJumpElapsed_ = 0.0f;
			boss_->SetIntroPanic(false, 0.0f);
		}
	} else {
		boss_->SetIntroPanic(false, 0.0f);
	}

	// -------------------------------------------------
	// 常時うねり（喜んでる感じ）
	// -------------------------------------------------
	boss_->SetTentacleCharge(true, 0.78f);

	boss_->SetPosition(bossPos_);
	boss_->SetRotate(bossRot_);
	boss_->SyncTransform();
	boss_->Update(dt_);
	// -------------------------------------------------
	// ボスのジャンプに連動して、画面上から崩れ落ちるストリークを出す
	// -------------------------------------------------
	if (bossJumping_) {
		fallEmitTimer_ += dt_;
		fallFrameToggle_ ^= 1;

		if (fallFrameToggle_ == 0) {
			while (fallEmitTimer_ >= fallEmitInterval_) {
				fallEmitTimer_ -= fallEmitInterval_;

				const Matrix4x4 camW = camera_->GetWorldMatrix();
				Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
				Vector3 camRight = MyMath::Normalize({ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
				Vector3 camUp = MyMath::Normalize({ camW.m[1][0], camW.m[1][1], camW.m[1][2] });
				Vector3 camFwd = MyMath::Normalize({ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

				const int kSpawnPerStep = 8;

				for (int i = 0; i < kSpawnPerStep; ++i) {
					// 画面上側の広い範囲から降らせる
					float xSpread = ((rand() % 5600) - 2800) / 100.0f; // -28 ～ +28
					float zDepth = 18.0f + (rand() % 1800) / 40.0f;    // 18 ～ 63
					float yHigh = 10.0f + (rand() % 500) / 20.0f;      // 10 ～ 35

					Vector3 spawn =
						camPos +
						camRight * xSpread +
						camFwd * zDepth +
						camUp * yHigh;

					ParticleManager::GetInstance()->Emit("fallStreak", spawn, 1);
				}
			}
		}
	} else {
		fallEmitTimer_ = 0.0f;
	}

	// -------------------------------------------------
	// カメラシェイク
	// -------------------------------------------------
	Vector3 camPos = cameraBaseTranslate_;
	if (cameraShakeTimer_ > 0.0f) {
		cameraShakeTimer_ -= dt_;

		float ratio = cameraShakeTimer_ / cameraShakeDuration_;
		if (ratio < 0.0f) {
			ratio = 0.0f;
		}

		float amp = cameraShakeAmp_ * ratio;
		camPos.x += std::sinf(bossAnimTimer_ * 95.0f) * amp;
		camPos.y += std::cosf(bossAnimTimer_ * 121.0f) * amp * 0.65f;
		camPos.z += std::sinf(bossAnimTimer_ * 83.0f + 0.6f) * amp * 0.35f;
	}

	camera_->SetTranslate(camPos);
	camera_->Update();
	dirLight_->Update();
	overSprite_->Update();
	ParticleManager::GetInstance()->Update(dt_);

	// 「GAME OVER」表示の更新
	if (irisOpening_) {
		irisScale_ = UpdateIrisScale(iris_.get(), irisOpenTween_, dt_); // 虹彩絞りの更新（開き演出）

		// 開き演出が終わったらフラグを下ろす（以降は常に表示される状態）
		if (irisOpenTween_.Finished()) {
			irisOpening_ = false;
		}
	}

	// 「GAME OVER」表示の更新
	if (!irisClosing_ && !irisOpening_) {
		const auto cmd = overMenu_->Update(dt_);

		// コマンドに応じて次のアクションをセットし、虹彩絞りの閉じ演出を開始
		if (cmd == GameResultMenuController::Command::Restart) {
			nextAction_ = NextAction::Restart;
			irisClosing_ = true;
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDuration_, Ease::Type::InBack);
		} else if (cmd == GameResultMenuController::Command::ReturnToTitle) { // タイトルに戻る
			nextAction_ = NextAction::ReturnToTitle;
			irisClosing_ = true;
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDuration_, Ease::Type::InBack);
		}
	}

	// 虹彩絞りの閉じ演出更新と、演出終了後のシーン遷移
	if (irisClosing_) {
		UpdateIrisScale(iris_.get(), irisCloseTween_, dt_); // 虹彩絞りの更新（閉じ演出）

		// 閉じ演出が終わったら次のシーンへ遷移
		if (irisCloseTween_.Finished()) {

			// 次のアクションに応じてシーン遷移
			if (nextAction_ == NextAction::Restart) {
				sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_)); // リスタート
				return;
			}
			// タイトルへ戻る
			sceneManager_->SetNextScene(std::make_unique<TitleScene>(dxCommon_, srvManager_)); // タイトルへ
			return;
		}
	}

	// ─── スカイボックス回転 ───
	constexpr float kTwoPi = 6.2831853f;
	skyPitch_ -= skyRotSpeedX_;
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;
	// Xだけ回す
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	// ─── GAME OVER スプライト更新 ───
	if (overSprite_) {
		overAlpha_ = overAlphaTween_.Update(dt_);
		overScale_ = overScaleTween_.Update(dt_);
		overGlowTimer_ += dt_;

		float s = 0.5f + 0.5f * std::sin(overGlowTimer_ * 3.8f);
		float t01 = std::pow(s, 2.3f);

		const Vector4 dark = { 0.35f, 0.00f, 0.00f, overAlpha_ };
		const Vector4 bright = { 1.60f, 0.08f, 0.02f, overAlpha_ };

		Vector4 color = {
			dark.x + (bright.x - dark.x) * t01,
			dark.y + (bright.y - dark.y) * t01,
			dark.z + (bright.z - dark.z) * t01,
			overAlpha_
		};

		color.y *= 0.6f;
		color.z *= 0.3f;

		overSprite_->SetColor(color);

		const float baseW = 800.0f;
		const float baseH = 800.0f;
		overSprite_->SetSize({ baseW * overScale_, baseH * overScale_ });

		overSprite_->Update();
	}

	//=========================================================
	// 不定期なノイズ発生
	// 毎回、次に来るまでの時間・継続時間・強さをランダム化する
	//=========================================================
	if (!isNoisePlaying_) {
		noiseIntervalTimer_ += dt_;

		if (noiseIntervalTimer_ >= noiseNextInterval_) {
			noiseIntervalTimer_ = 0.0f;
			noiseDurationTimer_ = 0.0f;
			isNoisePlaying_ = true;

			// 今回のノイズの長さと見た目を毎回変える
			noiseCurrentDuration_ = RandomRange(0.1f, 0.4f); // 現在のノイズ継続時間

			const float intensity = RandomRange(0.72f, 1.02f);
			const float flash = RandomRange(0.02f, 0.08f);

			noiseEffect_->SetActive(true);
			noiseEffect_->SetIntensity(intensity);
			noiseEffect_->SetFlash(flash);
		}
	} else {
		noiseDurationTimer_ += dt_;

		if (noiseDurationTimer_ >= noiseCurrentDuration_) {
			noiseDurationTimer_ = 0.0f;
			isNoisePlaying_ = false;

			noiseEffect_->SetActive(false);
			noiseEffect_->SetIntensity(0.0f);
			noiseEffect_->SetFlash(0.0f);

			// 次に来るまでの時間も毎回変える
			// 短めに来るときもあれば、しばらく来ないときもある
			float nextInterval = RandomRange(0.05f, 1.2f);

			noiseNextInterval_ = nextInterval;
		}
	}

	noiseEffect_->Update(dt_);
}

void GameOverScene::Draw() {
	// 3D
	Object3dCommon::GetInstance()->DrawSetCommon();
	boss_->Draw(dxCommon_);
	skybox_->Draw();

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

	// 2D
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	if ((irisOpening_ || irisClosing_)) {
		iris_->Draw(); // 常に最前面
	}
	overSprite_->Draw(); // 「GAME OVER」も常に最前面
	overMenu_->Draw(); // メニューは最後に描いて常に最前面（ただしirisよりは後ろ）
}