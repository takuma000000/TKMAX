#include "GameOverScene.h"
#include "TextureManager.h"
#include "AudioCatalog.h"
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
		// 0.0f ～ 1.0f の乱数を作る
		const float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

		// 指定された最小値～最大値の範囲に変換して返す
		return minValue + (maxValue - minValue) * t;
	}
}

void GameOverScene::Initialize() {
	/// ──────────────── 音声読み込み ───────────────
	AudioCatalog::LoadResultAudios();

	/// ──────────────── モデル読み込み ───────────────
	ModelManager::GetInstance()->LoadModel("turtle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("turtle_flipper.obj", dxCommon_);

	/// ──────────────── テクスチャ読み込み ───────────────
	TextureManager::GetInstance()->LoadTexture("./resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/over.png");

	/// ──────────────── カメラ初期化 ───────────────
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->Update();

	/// ──────────────── ライト初期化 ───────────────
	dirLight_ = std::make_unique<DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	/// ──────────────── パーティクル初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());

	// ボスのジャンプに合わせて、画面上から崩れ落ちるような線状エフェクトを出すためのグループ
	ParticleManager::GetInstance()->CreateParticleGroup(
		"fallStreak",
		"./resources/texture/circle2.png",
		ParticleManager::ParticleType::NORMAL
	);

	/// ──────────────── ボス初期化 ───────────────
	boss_ = std::make_unique<BossEnemy>();
	boss_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	boss_->SetCamera(camera_.get());

	// ボスの基準位置。Update中の揺れやジャンプは毎フレームここを基準に加算する
	bossBasePos_ = { 0.0f, 2.0f, 42.0f };
	bossPos_ = bossBasePos_;
	bossRot_ = { 0.0f, 3.14f, 0.0f };

	boss_->SetPosition(bossPos_);
	boss_->SetRotate(bossRot_);
	boss_->SetScale({ 3.5f, 3.5f, 3.5f });

	// GameOver演出用なので、通常ゲーム中の移動・攻撃制御を止める
	boss_->SetLocked(true);
	boss_->SyncTransform();

	/// ──────────────── カメラ基準位置設定 ───────────────
	// カメラシェイク時もこの位置を基準に揺らす
	cameraBaseTranslate_ = { 0.0f, 2.2f, -13.5f };
	camera_->SetTranslate(cameraBaseTranslate_);
	camera_->SetRotate({ 0.04f, 0.0f, 0.0f });
	camera_->Update();

	/// ──────────────── スカイボックス初期化 ───────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());

	// GameOverらしく赤紫系に寄せる
	skybox_->SetColor({ 1.0f, 0.0f, 0.5f, 1.0f });

	/// ──────────────── アイリス初期化 ───────────────
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMaxScale_, "./resources/texture/circle2.png");

	// 最初は画面全体を覆う大きさにして、そこから開く
	irisScale_ = irisMaxScale_;
	iris_->SetSize({ irisScale_, irisScale_ });

	// GameOverシーン入場時のアイリス開き演出
	irisOpenTween_.Reset(
		irisMaxScale_,
		0.0f,
		kIrisDuration_,
		Ease::Type::OutBack
	);

	/// ──────────────── GAME OVERスプライト初期化 ───────────────
	overSprite_ = std::make_unique<Sprite>();
	overSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/over.png");
	overSprite_->SetAnchorPoint({ 0.5f, 0.5f });
	overSprite_->SetPosition({ WindowsAPI::GetClientWidth() * 0.5f, WindowsAPI::GetClientHeight() * 0.5f });

	// 最初は透明。Tweenでフェードインさせる
	overSprite_->SetColor({ 1, 1, 1, 0 });

	overActive_ = true;
	overAlpha_ = 1.0f;
	overScale_ = 1.0f;
	overGlowTimer_ = 0.0f;

	// 一度初期値でTweenを持たせてから、下で実際の演出用Tweenを設定する
	overAlphaTween_.Reset(1.0f, 1.0f, 0.01f, Ease::Type::Linear);
	overScaleTween_.Reset(1.0f, 1.0f, 0.01f, Ease::Type::Linear);

	// 表示開始時は透明から表示し、少し拡大しながら出す
	overAlphaTween_.Reset(0.0f, 1.0f, 0.7f, Ease::Type::OutQuad);
	overScaleTween_.Reset(0.8f, 1.0f, 0.7f, Ease::Type::OutBack);

	overActive_ = true;

	/// ──────────────── GameOverメニュー初期化 ───────────────
	overMenu_ = std::make_unique<GameResultMenuController>();
	overMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		static_cast<float>(WindowsAPI::GetClientWidth()),
		static_cast<float>(WindowsAPI::GetClientHeight())
	);
	// ボス戦中に死亡した場合だけ「ボス戦からやり直す」を表示する
	overMenu_->SetBossRetryVisible(diedInBossBattle_);

	/// ──────────────── ノイズエフェクト初期化 ───────────────
	noiseEffect_ = std::make_unique<TKM::NoiseEffect>();
	noiseEffect_->Initialize(dxCommon_);
	noiseEffect_->SetActive(true);
	noiseEffect_->SetIntensity(0.20f);
	noiseEffect_->SetFlash(0.04f);

	// 初回のノイズ発生タイミングもランダムにして、規則的に見えないようにする
	noiseNextInterval_ = RandomRange(0.02f, 0.8f);

	/// ──────────────── 水面波紋エフェクト初期化 ───────────────
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();
	rippleEffect_->Initialize(dxCommon_);

	// メニュー決定時などに使えるよう、DirectXCommon側に登録する
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());
}

void GameOverScene::Finalize() {
	/// ──────────────── 各種終了処理 ───────────────
	TKM::ParticleManager::GetInstance()->ClearAllGroups();

	// このシーン専用で使ったポストエフェクト参照を外す
	dxCommon_->SetWaterRippleEffect(nullptr);
	dxCommon_->SetNoiseEffect(nullptr);

	AudioManager::GetInstance()->Finalize();
}

void GameOverScene::Update() {
	/// ──────────────── 入力更新 ───────────────
	Input::GetInstance()->Update();

	/// ──────────────── ボスタイマー更新 ───────────────
	// 常時揺れ用タイマー
	bossAnimTimer_ += dt_;

	// 定期ジャンプの間隔計測用タイマー
	bossJumpTimer_ += dt_;

	/// ──────────────── ボス常時アニメ更新 ───────────────
	// 複数のsin波を足して、単調ではない上下揺れにする
	float idleFloatY =
		std::sinf(bossAnimTimer_ * 2.6f) * 0.55f +
		std::sinf(bossAnimTimer_ * 5.4f + 0.7f) * 0.18f;

	// 左右に揺らして、勝ち誇っているような動きにする
	float idleSwingX =
		std::sinf(bossAnimTimer_ * 1.8f) * 0.9f;

	// Z回転で体を傾け、楽しそうに揺れている印象を作る
	float idleRotZ =
		std::sinf(bossAnimTimer_ * 2.1f) * 0.08f +
		std::sinf(bossAnimTimer_ * 4.8f + 0.4f) * 0.03f;

	// 正面向きを基準に、少しだけ左右へ首を振る
	float idleRotY =
		3.14159265f + std::sinf(bossAnimTimer_ * 1.1f) * 0.08f;

	// 毎フレーム基準位置から作り直すことで、揺れの加算が蓄積しないようにする
	bossPos_ = bossBasePos_;
	bossPos_.x += idleSwingX;
	bossPos_.y += idleFloatY;

	bossRot_ = { 0.0f, idleRotY, idleRotZ };

	/// ──────────────── 定期ジャンプ開始判定 ───────────────
	if (!bossJumping_ && bossJumpTimer_ >= bossJumpInterval_) {
		// ジャンプ状態へ入る
		bossJumping_ = true;

		// 次回ジャンプのため、間隔タイマーをリセット
		bossJumpTimer_ = 0.0f;

		// ジャンプの経過時間を0から開始
		bossJumpElapsed_ = 0.0f;

		// 着地時のカメラシェイクを1回だけ発生させるためのフラグを戻す
		bossLandingShakeTriggered_ = false;
	}

	/// ──────────────── ジャンプ中処理 ───────────────
	if (bossJumping_) {
		bossJumpElapsed_ += dt_;

		// ジャンプの進行度を0.0～1.0で扱う
		float t = bossJumpElapsed_ / bossJumpDuration_;
		if (t > 1.0f) {
			t = 1.0f;
		}

		// 0→1→0の山なり値を作ってジャンプの高さに使う
		float jumpArc = 4.0f * t * (1.0f - t);
		bossPos_.y += jumpArc * bossJumpHeight_;

		// ジャンプ中だけ少し大きく傾けて、跳ねている感じを出す
		bossRot_.z += std::sinf(t * 3.14159265f) * 0.10f;

		// 着地に近づくほど慌てた見た目にする
		float panic01 = (t < 0.55f) ? 0.35f : 0.85f;
		boss_->SetIntroPanic(true, panic01);

		// 着地直前に1回だけカメラシェイクを開始する
		if (!bossLandingShakeTriggered_ && t >= 0.92f) {
			bossLandingShakeTriggered_ = true;
			cameraShakeTimer_ = cameraShakeDuration_;
		}

		// ジャンプ時間が終わったら通常の揺れ状態へ戻す
		if (bossJumpElapsed_ >= bossJumpDuration_) {
			bossJumping_ = false;
			bossJumpElapsed_ = 0.0f;
			boss_->SetIntroPanic(false, 0.0f);
		}
	} else {
		// ジャンプしていない間はパニック表情を解除する
		boss_->SetIntroPanic(false, 0.0f);
	}

	/// ──────────────── ボス見た目更新 ───────────────
	// 触手を強めに動かし、勝ち誇っているような見た目にする
	boss_->SetTentacleCharge(true, 0.78f);

	boss_->SetPosition(bossPos_);
	boss_->SetRotate(bossRot_);

	// SetPosition / SetRotate の結果を内部Transformへ反映する
	boss_->SyncTransform();
	boss_->Update(dt_);

	/// ──────────────── ボスジャンプ連動ストリーク ───────────────
	if (bossJumping_) {
		fallEmitTimer_ += dt_;
		fallFrameToggle_ ^= 1;

		// 毎フレーム出すと重くなりやすいので、1フレームおきに発生させる
		if (fallFrameToggle_ == 0) {
			while (fallEmitTimer_ >= fallEmitInterval_) {
				fallEmitTimer_ -= fallEmitInterval_;

				// カメラ基準で「画面内に落ちてくる位置」を作るため、カメラの向きを取得する
				const Matrix4x4 camW = camera_->GetWorldMatrix();
				Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
				Vector3 camRight = MyMath::Normalize({ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
				Vector3 camUp = MyMath::Normalize({ camW.m[1][0], camW.m[1][1], camW.m[1][2] });
				Vector3 camFwd = MyMath::Normalize({ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

				// 1回の発生タイミングで複数本出して、崩れ落ちる密度を作る
				const int kSpawnPerStep = 8;

				for (int i = 0; i < kSpawnPerStep; ++i) {
					// 横方向のばらつき。画面全体に散らす
					float xSpread = ((rand() % 5600) - 2800) / 100.0f;

					// カメラ前方の奥行き。手前すぎず奥にも出す
					float zDepth = 18.0f + (rand() % 1800) / 40.0f;

					// カメラ上方向に高めの位置を作り、上から降ってくるように見せる
					float yHigh = 10.0f + (rand() % 500) / 20.0f;

					// カメラの右・上・前方向を使って、画面基準のワールド座標に変換する
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
		// ジャンプしていない間は発生タイマーをリセットして、次回ジャンプ時に余計な分が出ないようにする
		fallEmitTimer_ = 0.0f;
	}

	/// ──────────────── カメラシェイク更新 ───────────────
	// 毎フレーム基準位置から作り直すことで、揺れが蓄積しないようにする
	Vector3 camPos = cameraBaseTranslate_;

	if (cameraShakeTimer_ > 0.0f) {
		cameraShakeTimer_ -= dt_;

		// 残り時間の割合を使って、時間経過で揺れを弱くしていく
		float ratio = cameraShakeTimer_ / cameraShakeDuration_;
		if (ratio < 0.0f) {
			ratio = 0.0f;
		}

		// 残り時間に応じて揺れを弱める
		float amp = cameraShakeAmp_ * ratio;

		// X/Y/Zで揺れ方を少し変え、単調な揺れに見えないようにする
		camPos.x += std::sinf(bossAnimTimer_ * 95.0f) * amp;
		camPos.y += std::cosf(bossAnimTimer_ * 121.0f) * amp * 0.65f;
		camPos.z += std::sinf(bossAnimTimer_ * 83.0f + 0.6f) * amp * 0.35f;
	}

	camera_->SetTranslate(camPos);
	camera_->Update();

	/// ──────────────── 基本システム更新 ───────────────
	dirLight_->Update();
	overSprite_->Update();
	ParticleManager::GetInstance()->Update(dt_);

	/// ──────────────── アイリス開き更新 ───────────────
	if (irisOpening_) {
		irisScale_ = UpdateIrisScale(iris_.get(), irisOpenTween_, dt_);

		// 入場演出が終わったら通常表示状態へ移行する
		if (irisOpenTween_.Finished()) {
			irisOpening_ = false;
		}
	}

	/// ──────────────── GameOverメニュー更新 ───────────────
	if (!irisClosing_ && !irisOpening_) {
		const auto cmd = overMenu_->Update(dt_);

		if (cmd == GameResultMenuController::Command::Restart) {
			// はじめから再開
			nextAction_ = NextAction::Restart;
			irisClosing_ = true;
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDuration_, Ease::Type::InBack);
		} else if (cmd == GameResultMenuController::Command::RestartFromBoss) {
			// ボス戦から再開
			nextAction_ = NextAction::RestartFromBoss;
			irisClosing_ = true;
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDuration_, Ease::Type::InBack);
		} else if (cmd == GameResultMenuController::Command::ReturnToTitle) {
			// タイトルへ戻る
			nextAction_ = NextAction::ReturnToTitle;
			irisClosing_ = true;
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDuration_, Ease::Type::InBack);
		}
	}

	/// ──────────────── アイリス閉じ更新 ───────────────
	if (irisClosing_) {
		UpdateIrisScale(iris_.get(), irisCloseTween_, dt_);

		if (irisCloseTween_.Finished()) {
			if (nextAction_ == NextAction::Restart) {
				// はじめから再開
				sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_));
				return;
			}
			if (nextAction_ == NextAction::RestartFromBoss) {
				// ボス戦から再開
				sceneManager_->SetNextScene(std::make_unique<GameScene>(dxCommon_, srvManager_, true));
				return;
			}
			// タイトルへ戻る
			sceneManager_->SetNextScene(std::make_unique<TitleScene>(dxCommon_, srvManager_));
			return;
		}
	}

	/// ──────────────── スカイボックス回転更新 ───────────────
	constexpr float kTwoPi = 6.2831853f;

	skyPitch_ -= skyRotSpeedX_;

	// 角度が範囲外に出続けないように、0～2πの範囲に戻す
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	/// ──────────────── GAME OVERスプライト更新 ───────────────
	if (overSprite_) {
		// Tweenで透明度と拡大率を更新する
		overAlpha_ = overAlphaTween_.Update(dt_);
		overScale_ = overScaleTween_.Update(dt_);

		// 赤い明滅用の時間を進める
		overGlowTimer_ += dt_;

		// 0.0～1.0の明滅値を作る
		float s = 0.5f + 0.5f * std::sin(overGlowTimer_ * 3.8f);

		// powで暗い時間を長めにし、急に光るような印象にする
		float t01 = std::pow(s, 2.3f);

		const Vector4 dark = { 0.35f, 0.00f, 0.00f, overAlpha_ };
		const Vector4 bright = { 1.60f, 0.08f, 0.02f, overAlpha_ };

		// 暗い赤から明るい赤へ補間して、危険感のある明滅にする
		Vector4 color = {
			dark.x + (bright.x - dark.x) * t01,
			dark.y + (bright.y - dark.y) * t01,
			dark.z + (bright.z - dark.z) * t01,
			overAlpha_
		};

		// 緑と青を抑えて、赤の印象を強くする
		color.y *= 0.6f;
		color.z *= 0.3f;

		overSprite_->SetColor(color);

		const float baseW = 800.0f;
		const float baseH = 800.0f;

		overSprite_->SetSize({ baseW * overScale_, baseH * overScale_ });
		overSprite_->Update();

		rippleEffect_->Update(dt_);
	}

	/// ──────────────── 不定期ノイズ更新 ───────────────
	if (!isNoisePlaying_) {
		// ノイズが鳴っていない間は、次の発生までの時間を計測する
		noiseIntervalTimer_ += dt_;

		if (noiseIntervalTimer_ >= noiseNextInterval_) {
			// ノイズ開始時に各タイマーをリセットする
			noiseIntervalTimer_ = 0.0f;
			noiseDurationTimer_ = 0.0f;
			isNoisePlaying_ = true;

			// 毎回ノイズの長さと強さを変えて、規則的に見えないようにする
			noiseCurrentDuration_ = RandomRange(0.1f, 0.4f);

			const float intensity = RandomRange(0.72f, 1.02f);
			const float flash = RandomRange(0.02f, 0.08f);

			noiseEffect_->SetActive(true);
			noiseEffect_->SetIntensity(intensity);
			noiseEffect_->SetFlash(flash);
		}
	} else {
		// ノイズ再生中は、終了までの時間を計測する
		noiseDurationTimer_ += dt_;

		if (noiseDurationTimer_ >= noiseCurrentDuration_) {
			// ノイズを止め、次の発生待ち状態へ戻す
			noiseDurationTimer_ = 0.0f;
			isNoisePlaying_ = false;

			noiseEffect_->SetActive(false);
			noiseEffect_->SetIntensity(0.0f);
			noiseEffect_->SetFlash(0.0f);

			// 次回の発生間隔もランダムにして、一定周期に見えないようにする
			float nextInterval = RandomRange(0.05f, 1.2f);

			noiseNextInterval_ = nextInterval;
		}
	}

	noiseEffect_->Update(dt_);
}

void GameOverScene::Draw() {
	/// ──────────────── 3D描画 ───────────────
	Object3dCommon::GetInstance()->DrawSetCommon();

	boss_->Draw(dxCommon_);
	skybox_->Draw();

	/// ──────────────── パーティクル描画 ───────────────
	ParticleManager::GetInstance()->Draw();

	/// ──────────────── 2D描画 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	if ((irisOpening_ || irisClosing_)) {
		iris_->Draw();
	}

	overSprite_->Draw();
	overMenu_->Draw();
}