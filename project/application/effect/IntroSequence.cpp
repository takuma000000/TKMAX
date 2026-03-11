#include "IntroSequence.h"
#include <algorithm>
#include "ParticleManager.h"

namespace TKM {
	void IntroSequence::Initialize(DirectXCommon* dxCommon, TKM::Object3dCommon* object3dCommon) {
		object3dCommon_ = object3dCommon;
		dxCommon_ = dxCommon;

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
		phase_ = Phase::IrisOpen;
		introBoss_.reset();
		introBossVisible_ = false;
		introBossSpawned_ = false;
		introBossElapsed_ = 0.0f;
		introBossPhaseElapsed_ = 0.0f;
		introBossPos_ = { 0.0f, 6.0f, introBossAppearStartZ_ };
		introBossBasePos_ = introBossPos_;
		introBossRot_ = { 0.0f, 3.14159265f, 0.0f };
		introBossEscapeTargetX_ = 0.0f;
		introBossEscapeTargetTimer_ = 0.0f;
		camBlendToBossActive_ = false;
		camBlendBackActive_ = false;
		camSavedRot_ = { 0.0f, 0.0f, 0.0f };
		camBossStartRot_ = { 0.0f, 0.0f, 0.0f };
		camBossTargetRot_ = { 0.0f, 0.0f, 0.0f };
		camReturnStartRot_ = { 0.0f, 0.0f, 0.0f };
		introBossPreSpawnElapsed_ = 0.0f;
		introBossPreSpawnEmitAccum_ = 0.0f;
		introBossSpawnFxStarted_ = false;
		introBossSpawnFxFinished_ = false;
		introBossNoticeMarkEmitted_ = false;
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
				phase_ = Phase::CameraIntro;
				camYawTween_.Reset(camYawStart_, camYawEnd_, camIntroDuration_, Ease::Type::OutBack);
			}
		}

		// --- Camera Intro ---
		if (camIntroActive_) { // カメラインロ更新
			float yawNow = camYawTween_.Update(kFixedDt_); // ツイーンの進行に合わせて現在のヨー回転を計算
			float denom = std::max(0.0001f, (camYawEnd_ - camYawStart_)); // 開始と終了のヨー回転の差が小さい場合にゼロ割りを防ぐため、denom を小さな値でクランプ
			float t01 = std::clamp((yawNow - camYawStart_) / denom, 0.0f, 1.0f); // 現在のヨー回転が開始と終了の間でどれくらい進んでいるかを0..1の範囲で計算
			float pitchNow = MyMath::Lerp(camPitchStart_, camPitchEnd_, t01); // ツイーンの進行に合わせてピッチも線形補間で計算

			camera->SetRotate({ pitchNow, yawNow, 0.0f }); // 現在の回転をカメラにセット
			// ツイーン完了でカメラインロ終了
			if (camYawTween_.Finished()) {
				camIntroActive_ = false; // カメラインロ終了
				camIntroDone_ = true; // カメラインロが完了したことを記録
				camera->SetRotate({ camPitchEnd_, camYawEnd_, 0.0f }); // 最終的な回転を確実にセット
			}
		}
		// --- Boss Intro ---
		// カメラインロが完了したら、ボス出現前の待機演出を開始（1回だけ）。カメラインロが完了していないときは、ここでボス出現前の待機演出を開始しないようにする（これも安全側の措置。通常はカメラインロが完了してからこのコードに到達するはずなので、ここでチェックする必要はないが、万が一の初期化順の問題などでカメラインロが完了していない状態でここに到達してしまったときに、ボス出現前の待機演出を開始しないようにするため）。
		if (camIntroDone_ && phase_ == Phase::CameraIntro) {
			StartBossPreSpawn_(camera);
		}
		// --- Boss camera blend in ---
		if (camBlendToBossActive_) {
			float t = camBlendToBossTween_.Update(kFixedDt_); // ツイーンの進行に合わせて0..1の範囲で t を計算
			
			// カメラ回転を開始と終了の間で線形補間して計算
			Vector3 rot{};
			rot.x = MyMath::Lerp(camBossStartRot_.x, camBossTargetRot_.x, t);
			rot.y = MyMath::Lerp(camBossStartRot_.y, camBossTargetRot_.y, t);
			rot.z = MyMath::Lerp(camBossStartRot_.z, camBossTargetRot_.z, t);

			camera->SetRotate(rot);

			// ツイーン完了でブレンド終了
			if (camBlendToBossTween_.Finished()) {
				camBlendToBossActive_ = false;
				camera->SetRotate(camBossTargetRot_);
			}
		}
		// --- Boss camera blend back ---
		// ボス登場～逃走の間に、何らかの理由でカメラを元の位置に戻す必要が出たときのブレンド（例：プレイヤーが死んでリトライしたときなど）。ブレンド開始から終了まで、カメラ回転を開始と終了の間で線形補間して計算。
		if (camBlendBackActive_) {
			float t = camBlendBackTween_.Update(kFixedDt_);
			// カメラ回転を開始と終了の間で線形補間して計算
			Vector3 rot{};
			rot.x = MyMath::Lerp(camReturnStartRot_.x, camSavedRot_.x, t);
			rot.y = MyMath::Lerp(camReturnStartRot_.y, camSavedRot_.y, t);
			rot.z = MyMath::Lerp(camReturnStartRot_.z, camSavedRot_.z, t);

			camera->SetRotate(rot);

			// ツイーン完了でブレンド終了
			if (camBlendBackTween_.Finished()) {
				camBlendBackActive_ = false;
				camera->SetRotate(camSavedRot_);
			}
		}
		// ボス登場～逃走更新
		if (phase_ == Phase::BossPreSpawn) {
			UpdateBossPreSpawn_(camera);
		} else if (phase_ == Phase::BossAppear) {
			UpdateBossAppear_(camera);
		} else if (phase_ == Phase::BossPause) {
			UpdateBossPause_(camera);
		} else if (phase_ == Phase::BossNoticeHop) {
			UpdateBossNoticeHop_(camera);
		} else if (phase_ == Phase::BossPanic) {
			UpdateBossPanic_(camera);
		} else if (phase_ == Phase::BossEscape) {
			UpdateBossEscape_(camera);
		}

		// --- Boss phase camera follow ---
		// ボス登場～逃走の間、ブレンド中でないときは、カメラをゆっくりボスの方に向ける（ブレンド中はツイーンで回転を制御するため、ここでは回転を更新しない）
		if (!camBlendToBossActive_ && !camBlendBackActive_) {
			if (phase_ == Phase::BossAppear ||
				phase_ == Phase::BossPause ||
				phase_ == Phase::BossNoticeHop ||
				phase_ == Phase::BossPanic ||
				phase_ == Phase::BossEscape) {

				Vector3 nowRot = camera->GetRotate(); // 現在のカメラ回転を取得
				Vector3 nextRot{};
				float follow = 8.0f * kFixedDt_;

				nextRot.x = MyMath::Lerp(nowRot.x, camBossTargetRot_.x, follow);
				nextRot.y = MyMath::Lerp(nowRot.y, camBossTargetRot_.y, follow);
				nextRot.z = MyMath::Lerp(nowRot.z, camBossTargetRot_.z, follow);

				camera->SetRotate(nextRot);
			}
		}

		// --- start.png を一度だけ出す ---
		if (phase_ == Phase::ShowStart && !startPlayed_ && !camBlendBackActive_) {
			StartGameStart_(enemiesInitialized, outRequestInitEnemies);
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
					phase_ = Phase::Done;

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

	void IntroSequence::Draw(DirectXCommon* dxCommon, bool irisClosing) const {
		// Iris（開いているとき、または閉じる演出中は描画）
		if (irisOpening_ && iris_) {
			iris_->Draw();
		}
		// アイリスが閉じる演出中は描画
		if (irisClosing && iris_) {
			iris_->Draw();
		}
		// start.png（表示中のみ）
		if (startVisible_ && startSprite_) {
			startSprite_->Draw();
		}
	}

	void IntroSequence::DrawIntroBoss3D(DirectXCommon* dxCommon) const {
		if (introBossVisible_ && introBoss_) {
			introBoss_->Draw(dxCommon); // イントロ用ボスの3D描画
		}
	}

	void IntroSequence::StartBossIntro_(Camera* camera) {
		if (!camera || !object3dCommon_ || introBossSpawned_) { return; }

		introBoss_ = std::make_unique<BossEnemy>();
		introBoss_->Initialize(object3dCommon_, dxCommon_);
		introBoss_->SetCamera(camera);
		introBoss_->SetPosition({ 0.0f, 6.0f, introBossAppearStartZ_ });
		introBoss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
		introBoss_->SetLocked(true);

		introBossPos_ = { 0.0f, 6.0f, introBossAppearStartZ_ };
		introBossBasePos_ = introBossPos_;
		introBossRot_ = { 0.0f, 3.14159265f, 0.0f };

		introBossVisible_ = true;
		introBossSpawned_ = true;
		introBossPhaseElapsed_ = 0.0f;
		introBossElapsed_ = 0.0f;
		phase_ = Phase::BossAppear;

		// BossPreSpawn 開始時に保存した通常カメラ回転へ最後に戻すため。
		camBossStartRot_ = camera->GetRotate();
		camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };

		camBlendToBossActive_ = true;
		camBlendBackActive_ = false;
		camBlendToBossTween_.Reset(0.0f, 1.0f, camBlendToBossSec_, Ease::Type::InOutSine);

		ParticleManager::GetInstance()->Emit("bossWarp_core", introBossPos_, 12);
	}

	void IntroSequence::StartBossPreSpawn_(Camera* camera) {
		if (!camera) { return; }

		phase_ = Phase::BossPreSpawn;
		introBossPreSpawnElapsed_ = 0.0f;
		introBossPreSpawnEmitAccum_ = 0.0f;
		introBossSpawnFxStarted_ = true;
		introBossSpawnFxFinished_ = false;

		// まだボス本体は出さない
		introBossVisible_ = false;
		introBossSpawned_ = false;

		// 出現予定位置だけ先に決める
		introBossPos_ = { 0.0f, 6.0f, introBossAppearStartZ_ };
		introBossBasePos_ = introBossPos_;
		introBossRot_ = { 0.0f, 3.14159265f, 0.0f };

		// カメラはもうボス出現位置を見に行く
		camSavedRot_ = camera->GetRotate();
		camBossStartRot_ = camSavedRot_;
		camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };

		camBlendToBossActive_ = true;
		camBlendBackActive_ = false;
		camBlendToBossTween_.Reset(0.0f, 1.0f, camBlendToBossSec_, Ease::Type::InOutSine);

		// 最初の“ゆがみの芯”
		ParticleManager::GetInstance()->Emit("bossWarp_core", introBossPos_, 4);
		ParticleManager::GetInstance()->Emit("bossWarp_swirl", introBossPos_, 18);
		ParticleManager::GetInstance()->Emit("bossWarp_dust", introBossPos_, 8);
	}

	void IntroSequence::UpdateBossPreSpawn_(Camera* camera) {
		if (!camera) { return; }

		introBossPreSpawnElapsed_ += kFixedDt_;
		introBossPreSpawnEmitAccum_ += kFixedDt_;

		float t = introBossPreSpawnElapsed_ / introBossPreSpawnSec_;
		t = std::clamp(t, 0.0f, 1.0f);

		// 時間経過に応じて、徐々にエフェクトの密度を上げていく
		while (introBossPreSpawnEmitAccum_ >= 0.08f) {
			introBossPreSpawnEmitAccum_ -= 0.08f;

			if (t < 0.45f) {
				// 前半：小さい粒子がパラパラ漂って、少しずつ吸われる
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", introBossPos_, 6);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", introBossPos_, 3);
			} else if (t < 0.80f) {
				// 中盤：明らかに吸い込みが始まる
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", introBossPos_, 12);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", introBossPos_, 6);
				ParticleManager::GetInstance()->Emit("bossWarp_core", introBossPos_, 2);
			} else {
				// 終盤：中心密度を上げて、出現直前感を出す
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", introBossPos_, 16);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", introBossPos_, 8);
				ParticleManager::GetInstance()->Emit("bossWarp_core", introBossPos_, 4);
			}
		}

		// 終盤で一回だけ「穴が開く」感じを強める
		if (!introBossSpawnFxFinished_ && t >= 0.82f) {
			introBossSpawnFxFinished_ = true;
			ParticleManager::GetInstance()->Emit("bossWarp_core", introBossPos_, 20);
			ParticleManager::GetInstance()->Emit("bossWarp_swirl", introBossPos_, 40);
		}

		// ゆがみ演出が終わったら、ここで初めてボス本体登場へ
		if (introBossPreSpawnElapsed_ >= introBossPreSpawnSec_) {
			StartBossIntro_(camera);
		}
	}

	void IntroSequence::UpdateBossAppear_(Camera* camera) {
		if (!introBoss_ || !camera) { return; }

		introBossPhaseElapsed_ += kFixedDt_;
		float t = introBossPhaseElapsed_ / introBossAppearSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		// 進行度
		float moveT = t;
		// 直線的すぎないように、少しゆっくり寄ってくる感じにする
		moveT = moveT * moveT * (3.0f - 2.0f * moveT); // smoothstep

		// 前進の基本
		float z = MyMath::Lerp(introBossAppearStartZ_, introBossAppearEndZ_, moveT);

		// ぷかぷか漂うオフセット
		float floatX = std::sinf(introBossPhaseElapsed_ * introBossAppearFloatFreqX_) * introBossAppearFloatAmpX_;

		// 上下は1本だと機械っぽいので、遅い大波 + 速い小波 を重ねる
		float floatYMain =
			std::sinf(introBossPhaseElapsed_ * introBossAppearFloatFreqY_) * introBossAppearFloatAmpY_;

		float floatYSub =
			std::sinf(introBossPhaseElapsed_ * (introBossAppearFloatFreqY_ * 2.15f) + 0.8f) *
			(introBossAppearFloatAmpY_ * 0.38f);

		float floatY = floatYMain + floatYSub;

		// 到着時に暴れすぎないよう、後半は少し弱める
		float damp = MyMath::Lerp(1.0f, 0.45f, moveT);

		introBossPos_.z = z;
		introBossPos_.x = floatX * damp;
		float bodyDrift = std::sinf(introBossPhaseElapsed_ * 0.95f + 1.2f) * 0.9f;
		introBossPos_.y = 6.0f + bodyDrift + floatY * damp;

		// 上下のぷかぷかに合わせて少し傾ける
		float rotZ =
			std::sinf(introBossPhaseElapsed_ * 2.2f) * introBossAppearTiltZ_ * damp +
			std::sinf(introBossPhaseElapsed_ * 4.6f + 0.5f) * (introBossAppearTiltZ_ * 0.35f) * damp;

		introBoss_->SetPosition(introBossPos_);
		introBoss_->SetRotate({ 0.0f, 3.14159265f, rotZ });
		introBoss_->SetIntroPanic(false, 0.0f);
		introBoss_->Update(kFixedDt_);

		camBossTargetRot_ = {
		MyMath::Lerp(0.10f, 0.06f, t),
		MyMath::Lerp(-0.10f, 0.0f, t),
		0.0f
		};

		if (t >= 1.0f) {
			introBossPhaseElapsed_ = 0.0f;
			introBossBasePos_ = introBossPos_;
			phase_ = Phase::BossPause;
		}
	}

	void IntroSequence::UpdateBossPause_(Camera* camera) {
		if (!introBoss_ || !camera) { return; }

		introBossPhaseElapsed_ += kFixedDt_;

		float t = introBossPhaseElapsed_ / introBossPauseSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		// 到達位置で一瞬静止
		introBossPos_ = introBossBasePos_;
		introBoss_->SetPosition(introBossPos_);
		introBoss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

		// 完全静止だと硬いので、ほんの少しだけ上下に呼吸
		float idleY = std::sinf(introBossPhaseElapsed_ * 5.0f) * 0.10f;
		introBoss_->SetPosition({ introBossPos_.x, introBossPos_.y + idleY, introBossPos_.z });

		// この段階ではまだ慌てさせない
		introBoss_->SetIntroPanic(false, 0.0f);
		introBoss_->Update(kFixedDt_);

		// カメラも落ち着かせる
		camBossTargetRot_ = { 0.055f, 0.0f, 0.0f };

		if (t >= 1.0f) {
			introBossPhaseElapsed_ = 0.0f;
			introBossBasePos_ = { introBossPos_.x, introBossPos_.y + idleY, introBossPos_.z };
			phase_ = Phase::BossNoticeHop;
		}
	}

	void IntroSequence::UpdateBossNoticeHop_(Camera* camera) {
		if (!introBoss_ || !camera) { return; }

		introBossPhaseElapsed_ += kFixedDt_;

		float t = introBossPhaseElapsed_ / introBossNoticeHopSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		// 最初に少し溜めてから跳ねる
		float hopT = 0.0f;
		if (t < 0.20f) {
			hopT = 0.0f;
		} else {
			hopT = (t - 0.20f) / 0.80f;
			if (hopT > 1.0f) { hopT = 1.0f; }
		}

		if (!introBossNoticeMarkEmitted_ && t >= 0.20f) {
			introBossNoticeMarkEmitted_ = true;

			// 顔より少し手前
			const Vector3 center = introBossBasePos_ + Vector3{ 0.0f, 3.2f, -3.0f };

			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -7.0f,  2.0f, 0.0f }, 1); // 左上
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 7.0f,  2.0f, 0.0f }, 1); // 右上
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -9.0f,  0.0f, 0.0f }, 1); // 左
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 9.0f,  0.0f, 0.0f }, 1); // 右
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 6.0f, -2.5f, 0.0f }, 1); // 右下
		}

		float hop = std::sinf(hopT * 3.14159265f);
		float hopY = hop * introBossNoticeHopY_;

		// 少しだけ「ビクッ」と横にもズレると気づいた感が出る
		float surpriseX = std::sinf(t * 3.14159265f) * 0.35f;

		// 軽く潰れてから伸びる感じ
		float squash = std::sinf(t * 3.14159265f);

		Vector3 pos = introBossBasePos_;
		pos.x += surpriseX;
		pos.y += hopY;

		// ほんの少し後ろにのけぞる
		float rotZ = std::sinf(t * 3.14159265f) * 0.12f;

		introBossPos_ = pos; // ベース位置に、ホップと横ズレを加算した位置を現在位置とする
		introBoss_->SetPosition(pos);
		introBoss_->SetRotate({ 0.0f, 3.14159265f, rotZ }); // 回転は、ベースの向きに、気づいたときのビクッと感を少し加える感じ

		// 気づいた瞬間の触手バタつき
		introBoss_->SetIntroPanic(true, 0.85f);

		// カメラは少しだけ反応させる
		camBossTargetRot_ = {
			0.045f,
			0.015f,
			0.0f
		};

		introBoss_->Update(kFixedDt_);

		if (t >= 1.0f) {
			introBoss_->SetIntroPanic(true, 1.0f);
			introBossPhaseElapsed_ = 0.0f;
			introBossBasePos_ = introBossPos_;
			introBossNoticeMarkEmitted_ = false;
			phase_ = Phase::BossPanic;
		}
	}

	void IntroSequence::UpdateBossPanic_(Camera* camera) {
		if (!introBoss_ || !camera) { return; }

		introBossPhaseElapsed_ += kFixedDt_;
		float t = introBossPhaseElapsed_ / introBossPanicSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		float shakeX = std::sinf(introBossPhaseElapsed_ * 12.0f) * introBossPanicAmpX_ * (0.30f + t * 0.70f);
		float shakeY = std::fabs(std::sinf(introBossPhaseElapsed_ * 15.0f)) * introBossPanicAmpY_;
		float wobbleRotZ = std::sinf(introBossPhaseElapsed_ * 13.0f) * 0.14f;

		introBossPos_.x = introBossBasePos_.x + shakeX;
		introBossPos_.y = introBossBasePos_.y + shakeY;
		introBossPos_.z = introBossBasePos_.z;

		introBoss_->SetPosition(introBossPos_);
		introBoss_->SetRotate({ 0.0f, 3.14159265f, wobbleRotZ });
		introBoss_->SetIntroPanic(true, 0.55f + t * 0.45f);
		introBoss_->Update(kFixedDt_);

		camBossTargetRot_ = { 0.06f, 0.0f, 0.0f };
		// ボス登場から慌てるフェーズの間は、カメラはボス演出用の回転を維持する

		if (t >= 1.0f) {
			introBossPhaseElapsed_ = 0.0f;
			introBossEscapeTargetX_ = introBossPos_.x;
			introBossEscapeTargetTimer_ = 0.0f;
			phase_ = Phase::BossEscape;
		}
	}

	void IntroSequence::UpdateBossEscape_(Camera* camera) {
		if (!introBoss_ || !camera) { return; }

		introBossPhaseElapsed_ += kFixedDt_;
		introBossEscapeTargetTimer_ += kFixedDt_;

		float t = introBossPhaseElapsed_ / introBossEscapeSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		// 逃走中は、X軸方向の逃げる動きの目標位置を一定時間ごとにランダムに変える
		if (introBossEscapeTargetTimer_ >= introBossEscapeTargetInterval_) {
			introBossEscapeTargetTimer_ = 0.0f;
			float sign = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
			float ampGrow = MyMath::Lerp(0.65f, 1.25f, t);
			float mag = 2.0f + (static_cast<float>(std::rand()) / RAND_MAX) * introBossEscapeAmpX_ * ampGrow;
			introBossEscapeTargetX_ = sign * mag;
		}

		float follow = 16.0f * kFixedDt_;
		introBossPos_.x = MyMath::Lerp(introBossPos_.x, introBossEscapeTargetX_, follow);
		introBossPos_.z += introBossEscapeSpeedZ_ * kFixedDt_ * (1.0f + t * 0.30f);
		introBossPos_.y = 6.0f + std::fabs(std::sinf(introBossPhaseElapsed_ * 14.0f)) * introBossEscapeHopY_;

		float leanZ = std::sinf(introBossPhaseElapsed_ * 16.0f) * 0.22f;

		introBoss_->SetPosition(introBossPos_);
		introBoss_->SetRotate({ 0.0f, 3.14159265f + std::sinf(introBossPhaseElapsed_ * 8.0f) * 0.10f, leanZ });
		introBoss_->SetIntroPanic(true, 1.0f);
		introBoss_->Update(kFixedDt_);

		camBossTargetRot_ = { 0.05f, 0.0f, 0.0f };

		if (introBossPos_.z >= introBossEscapeEndZ_ || t >= 1.0f) {
			introBoss_->SetIntroPanic(false, 0.0f);
			introBossVisible_ = false;
			introBoss_.reset();

			// ボス演出カメラ → 通常カメラへ戻す
			camReturnStartRot_ = camera->GetRotate();
			camBlendBackActive_ = true;
			camBlendToBossActive_ = false;
			camBlendBackTween_.Reset(0.0f, 1.0f, camBlendBackSec_, Ease::Type::InOutSine);

			phase_ = Phase::ShowStart;
		}

		ParticleManager::GetInstance()->Emit("bossEscape_trail", introBossPos_, 6);
	}

	void IntroSequence::StartGameStart_(bool enemiesInitialized, bool& outRequestInitEnemies) {
		// start.png をスライドインさせて表示開始
		startPlayed_ = true;
		startVisible_ = true;
		startSlideIn_ = true;
		// スライドインと同時にフェードアウトも始めるため、開始前からアルファを0にしておく（両方が同時に始まったときに、アルファが0のままになってしまうのを防ぐため）
		startFadeOut_ = false;
		startHoldElapsed_ = 0.0f;
		startAlpha_ = 1.0f;
		// スライドイン開始前からアルファを0にしておくと、スライドインとフェードアウトが両方始まったときにアルファが0のままになってしまうため、スライドイン開始前からアルファを0にしておく
		startSprite_->SetColor({ 1,1,1,startAlpha_ });
		startSprite_->SetPosition({ startStartPos_.x, startEndPos_.y });
		startTween_.Reset(0.0f, 1.0f, startDuration_, Ease::Type::OutBack);

		(void)enemiesInitialized;
		(void)outRequestInitEnemies;
	}
} // namespace TKM