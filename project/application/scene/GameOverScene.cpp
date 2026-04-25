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
		// 0.0〜1.0の乱数を作る
		const float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

		// minValue〜maxValueの範囲に変換して返す
		return minValue + (maxValue - minValue) * t;
	}
}

void GameOverScene::Initialize() {
	// リザルト系で使う音声を読み込む
	AudioCatalog::LoadResultAudios();

	// GameOverシーンで使うモデルを読み込む
	ModelManager::GetInstance()->LoadModel("turtle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("turtle_flipper.obj", dxCommon_);

	// GameOverシーンで使うテクスチャを読み込む
	TextureManager::GetInstance()->LoadTexture("./resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/over.png");

	//=========================================================
	// カメラ初期化
	//=========================================================

	// GameOverシーン用カメラを生成する
	camera_ = std::make_unique<Camera>();

	// カメラの初期回転を設定する
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });

	// カメラの初期位置を設定する
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });

	// カメラ行列を更新する
	camera_->Update();

	//=========================================================
	// ライト初期化
	//=========================================================

	// 平行光源を生成する
	dirLight_ = std::make_unique<DirectionalLight>();

	// 平行光源の色・方向・強さを設定する
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	//=========================================================
	// パーティクル初期化
	//=========================================================

	// パーティクルマネージャーを初期化する
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());

	// ボスジャンプ時に降らせるストリーク用グループを作る
	ParticleManager::GetInstance()->CreateParticleGroup(
		"fallStreak",
		"./resources/texture/circle2.png",
		ParticleManager::ParticleType::NORMAL
	);

	//=========================================================
	// ボス初期化
	//=========================================================

	// GameOver演出用ボスを生成する
	boss_ = std::make_unique<BossEnemy>();

	// ボスを初期化する
	boss_->Initialize(Object3dCommon::GetInstance(), dxCommon_);

	// ボスにカメラを設定する
	boss_->SetCamera(camera_.get());

	// ボスの基準位置を設定する
	bossBasePos_ = { 0.0f, 2.0f, 42.0f };

	// ボスの現在位置を基準位置に合わせる
	bossPos_ = bossBasePos_;

	// ボスの初期回転を設定する
	bossRot_ = { 0.0f, 3.14f, 0.0f };

	// ボス位置を反映する
	boss_->SetPosition(bossPos_);

	// ボス回転を反映する
	boss_->SetRotate(bossRot_);

	// ボスを大きめに表示する
	boss_->SetScale({ 3.5f, 3.5f, 3.5f });

	// 演出用にロック状態にする
	boss_->SetLocked(true);

	// 初期Transformを同期する
	boss_->SyncTransform();

	// カメラの基準位置を設定する
	cameraBaseTranslate_ = { 0.0f, 2.2f, -13.5f };

	// カメラを基準位置へ配置する
	camera_->SetTranslate(cameraBaseTranslate_);

	// 少し見上げるような回転にする
	camera_->SetRotate({ 0.04f, 0.0f, 0.0f });

	// カメラ行列を更新する
	camera_->Update();

	//=========================================================
	// スカイボックス初期化
	//=========================================================

	// スカイボックスを生成する
	skybox_ = std::make_unique<Skybox>();

	// スカイボックスを初期化する
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");

	// スカイボックスへカメラを設定する
	skybox_->SetCamera(camera_.get());

	// GameOverっぽく赤紫系の色を乗せる
	skybox_->SetColor({ 1.0f, 0.0f, 0.5f, 1.0f });

	//=========================================================
	// アイリス初期化
	//=========================================================

	// 画面中央にアイリスを作成し、画面を覆える最大サイズを取得する
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMaxScale_, "./resources/texture/circle2.png");

	// 初期状態では画面全体を覆うサイズにする
	irisScale_ = irisMaxScale_;

	// アイリスサイズを反映する
	iris_->SetSize({ irisScale_, irisScale_ });

	// 入場用のアイリスオープンTweenを設定する
	irisOpenTween_.Reset(
		irisMaxScale_,
		0.0f,
		kIrisDuration_,
		Ease::Type::OutBack
	);

	//=========================================================
	// GAME OVER スプライト初期化
	//=========================================================

	// GAME OVER表示用スプライトを生成する
	overSprite_ = std::make_unique<Sprite>();

	// over.pngでスプライトを初期化する
	overSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/over.png");

	// 中心基準で配置・拡縮できるようにする
	overSprite_->SetAnchorPoint({ 0.5f, 0.5f });

	// 画面中央へ配置する
	overSprite_->SetPosition({ WindowsAPI::GetClientWidth() * 0.5f, WindowsAPI::GetClientHeight() * 0.5f });

	// 初期状態は透明にする
	overSprite_->SetColor({ 1, 1, 1, 0 });

	// GAME OVER演出を有効にする
	overActive_ = true;

	// 透明度を初期化する
	overAlpha_ = 1.0f;

	// スケールを初期化する
	overScale_ = 1.0f;

	// グロー用タイマーを初期化する
	overGlowTimer_ = 0.0f;

	// Tweenを一度初期値で作る
	overAlphaTween_.Reset(1.0f, 1.0f, 0.01f, Ease::Type::Linear);
	overScaleTween_.Reset(1.0f, 1.0f, 0.01f, Ease::Type::Linear);

	// GAME OVER文字のフェードインTweenを開始する
	overAlphaTween_.Reset(0.0f, 1.0f, 0.7f, Ease::Type::OutQuad);

	// GAME OVER文字のスケールアップTweenを開始する
	overScaleTween_.Reset(0.8f, 1.0f, 0.7f, Ease::Type::OutBack);

	// アニメーション進行フラグをONにする
	overActive_ = true;

	//=========================================================
	// GameOverメニュー初期化
	//=========================================================

	// リスタート/タイトル用メニューを生成する
	overMenu_ = std::make_unique<GameResultMenuController>();

	// GameOverメニューを初期化する
	overMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		static_cast<float>(WindowsAPI::GetClientWidth()),
		static_cast<float>(WindowsAPI::GetClientHeight())
	);

	//=========================================================
	// ノイズエフェクト初期化
	//=========================================================

	// ノイズエフェクトを生成する
	noiseEffect_ = std::make_unique<TKM::NoiseEffect>();

	// ノイズエフェクトを初期化する
	noiseEffect_->Initialize(dxCommon_);

	// 初期状態でノイズを有効にする
	noiseEffect_->SetActive(true);

	// 初期ノイズ強度を設定する
	noiseEffect_->SetIntensity(0.20f);

	// 初期フラッシュ量を設定する
	noiseEffect_->SetFlash(0.04f);

	// 次のノイズ発生までの時間をランダムに決める
	noiseNextInterval_ = RandomRange(0.02f, 0.8f);

	//=========================================================
	// 水面波紋エフェクト初期化
	//=========================================================

	// 決定時に出す水面波紋エフェクトを生成する
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();

	// 水面波紋エフェクトを初期化する
	rippleEffect_->Initialize(dxCommon_);

	// DirectXCommonへ水面波紋エフェクトを登録する
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());
}

void GameOverScene::Finalize() {
	// このシーンで作ったパーティクルグループをすべて消す
	TKM::ParticleManager::GetInstance()->ClearAllGroups();

	// DirectXCommonから水面波紋エフェクトの参照を外す
	dxCommon_->SetWaterRippleEffect(nullptr);

	// DirectXCommonからノイズエフェクトの参照を外す
	dxCommon_->SetNoiseEffect(nullptr);

	// オーディオマネージャーを終了する
	AudioManager::GetInstance()->Finalize();
}

void GameOverScene::Update() {
	// 入力状態を更新する
	Input::GetInstance()->Update();

	// ボス常時アニメ用タイマーを進める
	bossAnimTimer_ += dt_;

	// ボスジャンプ間隔用タイマーを進める
	bossJumpTimer_ += dt_;

	//=========================================================
	// ボス常時アニメ更新
	//=========================================================

	// ボスが喜んで浮いているような上下揺れを作る
	float idleFloatY =
		std::sinf(bossAnimTimer_ * 2.6f) * 0.55f +
		std::sinf(bossAnimTimer_ * 5.4f + 0.7f) * 0.18f;

	// ボスの左右揺れを作る
	float idleSwingX =
		std::sinf(bossAnimTimer_ * 1.8f) * 0.9f;

	// ボスのZ回転揺れを作る
	float idleRotZ =
		std::sinf(bossAnimTimer_ * 2.1f) * 0.08f +
		std::sinf(bossAnimTimer_ * 4.8f + 0.4f) * 0.03f;

	// ボスのY回転揺れを作る
	float idleRotY =
		3.14159265f + std::sinf(bossAnimTimer_ * 1.1f) * 0.08f;

	// ボス位置を基準位置に戻す
	bossPos_ = bossBasePos_;

	// 左右揺れを反映する
	bossPos_.x += idleSwingX;

	// 上下揺れを反映する
	bossPos_.y += idleFloatY;

	// ボス回転を作る
	bossRot_ = { 0.0f, idleRotY, idleRotZ };

	//=========================================================
	// 定期ジャンプ開始判定
	//=========================================================

	// ジャンプ中でなく、ジャンプ間隔を超えたらジャンプを開始する
	if (!bossJumping_ && bossJumpTimer_ >= bossJumpInterval_) {
		// ジャンプ中フラグを立てる
		bossJumping_ = true;

		// ジャンプ間隔タイマーをリセットする
		bossJumpTimer_ = 0.0f;

		// ジャンプ経過時間をリセットする
		bossJumpElapsed_ = 0.0f;

		// 着地シェイク未発生に戻す
		bossLandingShakeTriggered_ = false;
	}

	//=========================================================
	// ジャンプ中処理
	//=========================================================

	// ジャンプ中ならジャンプ演出を更新する
	if (bossJumping_) {
		// ジャンプ経過時間を進める
		bossJumpElapsed_ += dt_;

		// ジャンプ進行率を計算する
		float t = bossJumpElapsed_ / bossJumpDuration_;

		// 進行率を1.0で止める
		if (t > 1.0f) {
			t = 1.0f;
		}

		// 放物線状のジャンプ量を作る
		float jumpArc = 4.0f * t * (1.0f - t);

		// ジャンプ高さをY座標へ加算する
		bossPos_.y += jumpArc * bossJumpHeight_;

		// ジャンプ中は少し大きめに傾ける
		bossRot_.z += std::sinf(t * 3.14159265f) * 0.10f;

		// 着地直前はパニック表情を強める
		float panic01 = (t < 0.55f) ? 0.35f : 0.85f;
		boss_->SetIntroPanic(true, panic01);

		// 着地直前に一度だけカメラシェイクを開始する
		if (!bossLandingShakeTriggered_ && t >= 0.92f) {
			bossLandingShakeTriggered_ = true;
			cameraShakeTimer_ = cameraShakeDuration_;
		}

		// ジャンプ時間が終わったらジャンプ状態を解除する
		if (bossJumpElapsed_ >= bossJumpDuration_) {
			bossJumping_ = false;
			bossJumpElapsed_ = 0.0f;
			boss_->SetIntroPanic(false, 0.0f);
		}
	} else {
		// ジャンプしていない間はパニック表情を解除する
		boss_->SetIntroPanic(false, 0.0f);
	}

	//=========================================================
	// ボス見た目更新
	//=========================================================

	// 喜んでいるように触手チャージを常時強めにする
	boss_->SetTentacleCharge(true, 0.78f);

	// ボス位置を反映する
	boss_->SetPosition(bossPos_);

	// ボス回転を反映する
	boss_->SetRotate(bossRot_);

	// ボスTransformを同期する
	boss_->SyncTransform();

	// ボスを更新する
	boss_->Update(dt_);

	//=========================================================
	// ボスジャンプ連動ストリーク
	//=========================================================

	// ボスジャンプ中だけ画面上から落ちるストリークを発生させる
	if (bossJumping_) {
		// 発生タイマーを進める
		fallEmitTimer_ += dt_;

		// 1フレームおきに発生させるためのトグルを切り替える
		fallFrameToggle_ ^= 1;

		// トグルが0のフレームだけ発生処理を行う
		if (fallFrameToggle_ == 0) {
			// 発生間隔ぶん溜まっている間、複数回発生させる
			while (fallEmitTimer_ >= fallEmitInterval_) {
				// 発生間隔ぶんタイマーを減らす
				fallEmitTimer_ -= fallEmitInterval_;

				// カメラのワールド行列を取得する
				const Matrix4x4 camW = camera_->GetWorldMatrix();

				// カメラ位置を取得する
				Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };

				// カメラ右方向を取得する
				Vector3 camRight = MyMath::Normalize({ camW.m[0][0], camW.m[0][1], camW.m[0][2] });

				// カメラ上方向を取得する
				Vector3 camUp = MyMath::Normalize({ camW.m[1][0], camW.m[1][1], camW.m[1][2] });

				// カメラ前方向を取得する
				Vector3 camFwd = MyMath::Normalize({ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

				// 1回の発生処理で出す数
				const int kSpawnPerStep = 8;

				// カメラ前方の広い範囲にストリークを出す
				for (int i = 0; i < kSpawnPerStep; ++i) {
					// 横方向の散らばりを作る
					float xSpread = ((rand() % 5600) - 2800) / 100.0f;

					// カメラ前方への奥行きを作る
					float zDepth = 18.0f + (rand() % 1800) / 40.0f;

					// 画面上側から降るように高さを作る
					float yHigh = 10.0f + (rand() % 500) / 20.0f;

					// カメラ基準で発生位置を作る
					Vector3 spawn =
						camPos +
						camRight * xSpread +
						camFwd * zDepth +
						camUp * yHigh;

					// 落下ストリークを発生させる
					ParticleManager::GetInstance()->Emit("fallStreak", spawn, 1);
				}
			}
		}
	} else {
		// ジャンプしていない間は発生タイマーをリセットする
		fallEmitTimer_ = 0.0f;
	}

	//=========================================================
	// カメラシェイク更新
	//=========================================================

	// カメラ位置を基準位置から始める
	Vector3 camPos = cameraBaseTranslate_;

	// シェイク時間が残っていれば揺らす
	if (cameraShakeTimer_ > 0.0f) {
		// シェイク残り時間を減らす
		cameraShakeTimer_ -= dt_;

		// 残り時間から減衰率を作る
		float ratio = cameraShakeTimer_ / cameraShakeDuration_;

		// 0未満にならないようにする
		if (ratio < 0.0f) {
			ratio = 0.0f;
		}

		// 減衰込みの揺れ幅を計算する
		float amp = cameraShakeAmp_ * ratio;

		// X方向に細かく揺らす
		camPos.x += std::sinf(bossAnimTimer_ * 95.0f) * amp;

		// Y方向に少し弱めに揺らす
		camPos.y += std::cosf(bossAnimTimer_ * 121.0f) * amp * 0.65f;

		// Z方向にも少しだけ揺らす
		camPos.z += std::sinf(bossAnimTimer_ * 83.0f + 0.6f) * amp * 0.35f;
	}

	// カメラ位置を反映する
	camera_->SetTranslate(camPos);

	// カメラ行列を更新する
	camera_->Update();

	// ライトを更新する
	dirLight_->Update();

	// GAME OVERスプライトを更新する
	overSprite_->Update();

	// パーティクルを更新する
	ParticleManager::GetInstance()->Update(dt_);

	//=========================================================
	// アイリス開き更新
	//=========================================================

	// 入場アイリス中なら開き演出を進める
	if (irisOpening_) {
		// アイリスサイズをTweenに合わせて更新する
		irisScale_ = UpdateIrisScale(iris_.get(), irisOpenTween_, dt_);

		// 開き演出が終わったら入場完了にする
		if (irisOpenTween_.Finished()) {
			irisOpening_ = false;
		}
	}

	//=========================================================
	// GameOverメニュー更新
	//=========================================================

	// アイリス中でなければメニュー入力を受け付ける
	if (!irisClosing_ && !irisOpening_) {
		// メニューの更新結果を取得する
		const auto cmd = overMenu_->Update(dt_);

		// リスタートが選ばれた場合
		if (cmd == GameResultMenuController::Command::Restart) {
			// 次の動作をリスタートにする
			nextAction_ = NextAction::Restart;

			// アイリス閉じを開始する
			irisClosing_ = true;

			// アイリス閉じTweenを開始する
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDuration_, Ease::Type::InBack);
		} else if (cmd == GameResultMenuController::Command::ReturnToTitle) {
			// 次の動作をタイトルへ戻るにする
			nextAction_ = NextAction::ReturnToTitle;

			// アイリス閉じを開始する
			irisClosing_ = true;

			// アイリス閉じTweenを開始する
			irisCloseTween_.Reset(0.0f, irisMaxScale_, kIrisDuration_, Ease::Type::InBack);
		}
	}

	//=========================================================
	// アイリス閉じ更新
	//=========================================================

	// 退場アイリス中なら閉じ演出を進める
	if (irisClosing_) {
		// アイリスサイズをTweenに合わせて更新する
		UpdateIrisScale(iris_.get(), irisCloseTween_, dt_);

		// 閉じ演出が終わったら次のシーンへ遷移する
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
	// スカイボックス回転
	//=========================================================

	// 2π定数
	constexpr float kTwoPi = 6.2831853f;

	// X軸回転を進める
	skyPitch_ -= skyRotSpeedX_;

	// 上限を超えたら一周戻す
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;

	// 下限を超えたら一周進める
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	// スカイボックスをX軸だけ回す
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	//=========================================================
	// GAME OVER スプライト更新
	//=========================================================

	// GAME OVERスプライトがある場合だけ更新する
	if (overSprite_) {
		// フェードインTweenを更新して透明度を取得する
		overAlpha_ = overAlphaTween_.Update(dt_);

		// スケールTweenを更新して拡大率を取得する
		overScale_ = overScaleTween_.Update(dt_);

		// グロー用タイマーを進める
		overGlowTimer_ += dt_;

		// 0〜1で明滅する値を作る
		float s = 0.5f + 0.5f * std::sin(overGlowTimer_ * 3.8f);

		// 明滅に強弱をつける
		float t01 = std::pow(s, 2.3f);

		// 暗い色を作る
		const Vector4 dark = { 0.35f, 0.00f, 0.00f, overAlpha_ };

		// 明るい色を作る
		const Vector4 bright = { 1.60f, 0.08f, 0.02f, overAlpha_ };

		// 暗い色と明るい色を補間する
		Vector4 color = {
			dark.x + (bright.x - dark.x) * t01,
			dark.y + (bright.y - dark.y) * t01,
			dark.z + (bright.z - dark.z) * t01,
			overAlpha_
		};

		// 緑成分を抑える
		color.y *= 0.6f;

		// 青成分をさらに抑えて赤寄りにする
		color.z *= 0.3f;

		// GAME OVERスプライト色を反映する
		overSprite_->SetColor(color);

		// GAME OVERスプライトの基準幅
		const float baseW = 800.0f;

		// GAME OVERスプライトの基準高さ
		const float baseH = 800.0f;

		// Tweenのスケールをサイズに反映する
		overSprite_->SetSize({ baseW * overScale_, baseH * overScale_ });

		// GAME OVERスプライトを更新する
		overSprite_->Update();

		// 水面波紋を更新する
		rippleEffect_->Update(dt_);
	}

	//=========================================================
	// 不定期ノイズ更新
	//=========================================================

	// ノイズが再生中でない場合
	if (!isNoisePlaying_) {
		// 次のノイズ発生までのタイマーを進める
		noiseIntervalTimer_ += dt_;

		// 次のノイズ発生時間に到達したらノイズを開始する
		if (noiseIntervalTimer_ >= noiseNextInterval_) {
			// インターバルタイマーをリセットする
			noiseIntervalTimer_ = 0.0f;

			// ノイズ継続タイマーをリセットする
			noiseDurationTimer_ = 0.0f;

			// ノイズ再生中にする
			isNoisePlaying_ = true;

			// 今回のノイズ継続時間をランダムに決める
			noiseCurrentDuration_ = RandomRange(0.1f, 0.4f);

			// 今回のノイズ強度をランダムに決める
			const float intensity = RandomRange(0.72f, 1.02f);

			// 今回のフラッシュ量をランダムに決める
			const float flash = RandomRange(0.02f, 0.08f);

			// ノイズを有効化する
			noiseEffect_->SetActive(true);

			// ノイズ強度を反映する
			noiseEffect_->SetIntensity(intensity);

			// ノイズフラッシュを反映する
			noiseEffect_->SetFlash(flash);
		}
	} else {
		// ノイズ再生中の経過時間を進める
		noiseDurationTimer_ += dt_;

		// ノイズ継続時間を超えたらノイズを止める
		if (noiseDurationTimer_ >= noiseCurrentDuration_) {
			// 継続タイマーをリセットする
			noiseDurationTimer_ = 0.0f;

			// ノイズ再生中フラグを下ろす
			isNoisePlaying_ = false;

			// ノイズを無効化する
			noiseEffect_->SetActive(false);

			// ノイズ強度を0に戻す
			noiseEffect_->SetIntensity(0.0f);

			// フラッシュ量を0に戻す
			noiseEffect_->SetFlash(0.0f);

			// 次にノイズが来るまでの時間をランダムに決める
			float nextInterval = RandomRange(0.05f, 1.2f);

			// 次回インターバルを保存する
			noiseNextInterval_ = nextInterval;
		}
	}

	// ノイズエフェクトを更新する
	noiseEffect_->Update(dt_);
}

void GameOverScene::Draw() {
	//=========================================================
	// 3D描画
	//=========================================================

	// 3D描画共通設定を行う
	Object3dCommon::GetInstance()->DrawSetCommon();

	// ボスを描画する
	boss_->Draw(dxCommon_);

	// スカイボックスを描画する
	skybox_->Draw();

	//=========================================================
	// パーティクル描画
	//=========================================================

	// パーティクルを描画する
	ParticleManager::GetInstance()->Draw();

	//=========================================================
	// 2D描画
	//=========================================================

	// 2D描画共通設定を行う
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	// アイリス中なら最前面に描画する
	if ((irisOpening_ || irisClosing_)) {
		iris_->Draw();
	}

	// GAME OVERスプライトを描画する
	overSprite_->Draw();

	// メニューを描画する
	overMenu_->Draw();
}