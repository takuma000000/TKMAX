#include "IntroSequence.h"
#include <algorithm>
#include "ParticleManager.h"

namespace TKM {
	void IntroSequence::Initialize(DirectXCommon* dxCommon) {
		// Iris（開始は画面を覆った状態→開く）
		iris_ = CreateCenteredIrisSprite(dxCommon, irisMaxScale_);
		irisScale_ = irisMaxScale_; // 開始は画面全体を覆うサイズにしておく
		irisTween_.Reset(irisMaxScale_, 0.0f, kIrisDurationSec_, Ease::Type::OutBack); // 開始から終わりにかけて、画面を覆った状態から完全に開いた状態へ（Ease::OutBackで、少し戻しながら勢いよく開く感じにする）

		// start.png（最初は非表示）
		startSprite_ = std::make_unique<Sprite>();
		startSprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, "./resources/texture/start.png"); // テクスチャは適宜用意してください
		startSprite_->SetAnchorPoint({ 0.5f, 0.5f }); // 画像の中心が位置座標になるようにアンカーポイントを設定
		startSprite_->SetPosition({ startStartPos_.x, startStartPos_.y });
		startSprite_->SetSize({ 100, 100 }); // 適宜サイズを調整してください
		startSprite_->SetColor({ 1,1,1,1 }); // 最初は完全に不透明にしておく（スライドインと同時にフェードアウトも始める想定なので、スライドイン開始前からアルファを0にしておくと、スライドインとフェードアウトが両方始まったときにアルファが0のままになってしまうため）

		// 初期状態
		gameplayLocked_ = true;
		irisOpening_ = true;
		emitOpenBurst_ = true;
		emitOpenElapsed_ = 0.0f;
		emitFireworkPending_ = false;
		emitFireworkElapsed_ = 0.0f;
		lastEmitPos_ = { 0,0,0 };
		camIntroActive_ = false;
		camIntroDone_ = false;
		startT_ = 0.0f;
		startSlideIn_ = false;
		startVisible_ = false;
		startPlayed_ = false;
		startFadeOut_ = false;
		startHoldElapsed_ = 0.0f;
		startAlpha_ = 1.0f;
	}

	void IntroSequence::Update(float dt, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies) {
		(void)dt;

		// まだカメラが無いなら（初期化順の都合）安全側で何もしない
		if (!camera || !iris_) { return; }

		// --- Iris Opening ---
		if (irisOpening_) {
			emitOpenElapsed_ += kFixedDt_;

			// リング（開始から emitOpenDelaySec_ 秒後に一度だけ）
			if (emitOpenBurst_ && emitOpenElapsed_ >= emitOpenDelaySec_) {
				emitOpenBurst_ = false;

				const Matrix4x4 camW = camera->GetWorldMatrix();
				Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
				Vector3 camFwd = MyMath::Normalize(Vector3{ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

				const float depth = 20.0f;
				Vector3 centerInFront = camPos + camFwd * depth;
				centerInFront.y -= 0.1f;

				ParticleManager::GetInstance()->Emit("irisOpen", centerInFront, 60);

				lastEmitPos_ = centerInFront;
				emitFireworkPending_ = true;
				emitFireworkElapsed_ = 0.0f;
			}

			// 花火（開始から emitFireworkDelaySec_ 秒後に一度だけ）
			if (emitFireworkPending_ && emitOpenElapsed_ >= emitFireworkDelaySec_) {
				emitFireworkPending_ = false;
				ParticleManager::GetInstance()->Emit("irisFire", lastEmitPos_, 80);
			}

			// Iris 見た目更新
			irisScale_ = irisTween_.Update(kFixedDt_);
			iris_->SetSize({ irisScale_, irisScale_ });
			iris_->Update();

			// ツイーン完了でオープニング終了
			if (irisTween_.Finished()) {
				irisOpening_ = false;
			}

			// Iris が終わったら、カメラインロ開始（1回だけ）
			if (!irisOpening_ && !camIntroActive_ && !camIntroDone_) {
				camIntroActive_ = true;
				camYawTween_.Reset(camYawStart_, camYawEnd_, camIntroDuration_, Ease::Type::OutBack);
			}
		}

		// --- Camera Intro ---
		if (camIntroActive_) {
			float yawNow = camYawTween_.Update(kFixedDt_);

			float denom = std::max(0.0001f, (camYawEnd_ - camYawStart_));
			float t01 = std::clamp((yawNow - camYawStart_) / denom, 0.0f, 1.0f);
			float pitchNow = MyMath::Lerp(camPitchStart_, camPitchEnd_, t01);

			camera->SetRotate({ pitchNow, yawNow, 0.0f });

			if (camYawTween_.Finished()) {
				camIntroActive_ = false;
				camIntroDone_ = true;
				camera->SetRotate({ camPitchEnd_, camYawEnd_, 0.0f });
			}
		}

		// --- start.png を一度だけ出す ---
		if (camIntroDone_ && !startPlayed_) {
			startPlayed_ = true;
			startVisible_ = true;
			startSlideIn_ = true;

			startFadeOut_ = false;
			startHoldElapsed_ = 0.0f;
			startAlpha_ = 1.0f;

			startSprite_->SetColor({ 1,1,1,startAlpha_ });
			startSprite_->SetPosition({ startStartPos_.x, startEndPos_.y });
			startTween_.Reset(0.0f, 1.0f, startDuration_, Ease::Type::OutBack);
		}

		// --- スライドイン更新 ---
		if (startSlideIn_) {
			startT_ = startTween_.Update(kFixedDt_);

			if (startGlowOn_) {
				float glow = 1.0f + startGlowAmp_ * std::sin(startT_ * MyMath::GetPI());
				startSprite_->SetColor({ glow, glow, glow, startAlpha_ });
			} else {
				startSprite_->SetColor({ 1,1,1,startAlpha_ });
			}

			float x = MyMath::Lerp(startStartPos_.x, startEndPos_.x, startT_);
			float y = startEndPos_.y;
			startSprite_->SetPosition({ x, y });
			startSprite_->Update();

			if (startTween_.Finished()) {
				startSlideIn_ = false;
				startHoldElapsed_ = 0.0f;
			}
		} else if (startVisible_) {
			// 中央での呼吸発光（だんだん弱くなる）
			if (!startFadeOut_ && startGlowOn_) {
				float t01 = (startHoldSec_ > 0.0f) ? std::min(startHoldElapsed_ / startHoldSec_, 1.0f) : 1.0f;
				float decay = 1.0f - 0.7f * t01;
				float glow = 1.0f + decay * 0.20f * std::sin(startHoldElapsed_ * startGlowSpeed_);
				startSprite_->SetColor({ glow, glow, glow, startAlpha_ });
			}

			// 静止→フェードアウト
			if (!startFadeOut_) {
				startHoldElapsed_ += kFixedDt_;
				if (startHoldElapsed_ >= startHoldSec_) {
					startFadeOut_ = true;
				}
			}

			if (startFadeOut_) {
				startAlpha_ -= kFixedDt_ / startFadeSec_;
				if (startAlpha_ <= 0.0f) {
					startAlpha_ = 0.0f;
					startVisible_ = false;

					// 演出終了：敵初期化要求 + ゲーム解放
					if (!enemiesInitialized) {
						outRequestInitEnemies = true;
					}
					gameplayLocked_ = false;
				}
				startSprite_->SetColor({ 1,1,1,startAlpha_ });
			}

			startSprite_->Update();
		}
	}

	void IntroSequence::Draw(bool irisClosing) const {
		// Iris（開く）
		if (irisOpening_ && iris_) {
			iris_->Draw();
		}
		// Iris（閉じ）…GameScene側のフラグを受ける
		if (irisClosing && iris_) {
			iris_->Draw();
		}
		// start.png
		if (startVisible_ && startSprite_) {
			startSprite_->Draw();
		}
	}
} // namespace TKM