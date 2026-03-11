#include "IntroFlowStates.h"
#include "IntroSequence.h"
#include "ParticleManager.h"
#include <algorithm>

namespace {
	TKM::IntroSequence& AsIntro_(TKM::IStateContext& ctx) {
		return static_cast<TKM::IntroSequence&>(ctx);
	}
}

namespace TKM {

	void IntroIrisOpenState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::IrisOpen;
		s.irisOpening_ = true;
	}

	void IntroIrisOpenState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera || !s.iris_) { return; }

		s.emitOpenElapsed_ += IntroSequence::kFixedDt_;

		// リング
		if (s.emitOpenBurst_ && s.emitOpenElapsed_ >= s.emitOpenDelaySec_) {
			s.emitOpenBurst_ = false;

			const Matrix4x4 camW = camera->GetWorldMatrix();
			Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
			Vector3 camFwd = MyMath::Normalize(Vector3{ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

			const float depth = 20.0f;
			Vector3 centerInFront = camPos + camFwd * depth;
			centerInFront.y -= 0.1f;

			ParticleManager::GetInstance()->Emit("irisOpen", centerInFront, 60);

			s.lastEmitPos_ = centerInFront;
			s.emitFireworkPending_ = true;
		}

		// 花火
		if (s.emitFireworkPending_ && s.emitOpenElapsed_ >= s.emitFireworkDelaySec_) {
			s.emitFireworkPending_ = false;
			ParticleManager::GetInstance()->Emit("irisFire", s.lastEmitPos_, 80);
		}

		// Iris更新
		float irisScale = s.irisTween_.Update(IntroSequence::kFixedDt_);
		s.iris_->SetSize({ irisScale, irisScale });
		s.iris_->Update();

		if (s.irisTween_.Finished()) {
			s.irisOpening_ = false;
			s.flowSM_.Change(std::make_unique<IntroCameraIntroState>());
		}
	}

	void IntroCameraIntroState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::CameraIntro;
		s.camYawTween_.Reset(s.camYawStart_, s.camYawEnd_, s.camIntroDuration_, Ease::Type::OutBack);
	}

	void IntroCameraIntroState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera) { return; }

		float yawNow = s.camYawTween_.Update(IntroSequence::kFixedDt_);
		float denom = std::max(0.0001f, (s.camYawEnd_ - s.camYawStart_));
		float t01 = std::clamp((yawNow - s.camYawStart_) / denom, 0.0f, 1.0f);
		float pitchNow = MyMath::Lerp(s.camPitchStart_, s.camPitchEnd_, t01);

		camera->SetRotate({ pitchNow, yawNow, 0.0f });

		if (s.camYawTween_.Finished()) {
			camera->SetRotate({ s.camPitchEnd_, s.camYawEnd_, 0.0f });
			s.flowSM_.Change(std::make_unique<IntroBossPreSpawnState>());
		}
	}

	void IntroBossPreSpawnState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera) { return; }

		s.phase_ = IntroSequence::Phase::BossPreSpawn;
		s.introBossPreSpawnElapsed_ = 0.0f;
		s.introBossPreSpawnEmitAccum_ = 0.0f;
		s.introBossSpawnFxFinished_ = false;

		// まだボス本体は出さない
		s.introBossEscapeWarpBurstEmitted_ = false;

		// 出現予定位置だけ先に決める
		s.introBossPos_ = { 0.0f, 6.0f, s.introBossAppearStartZ_ };
		s.introBossBasePos_ = s.introBossPos_;

		// カメラはもうボス出現位置を見に行く
		s.camSavedRot_ = camera->GetRotate();
		s.camBossStartRot_ = s.camSavedRot_;
		s.camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };

		s.camBlendToBossActive_ = true;
		s.camBlendBackActive_ = false;
		s.camBlendToBossTween_.Reset(0.0f, 1.0f, s.camBlendToBossSec_, Ease::Type::InOutSine);

		ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossPos_, 4);
		ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossPos_, 18);
		ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossPos_, 8);
	}

	void IntroBossPreSpawnState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera) { return; }

		s.introBossPreSpawnElapsed_ += IntroSequence::kFixedDt_;
		s.introBossPreSpawnEmitAccum_ += IntroSequence::kFixedDt_;

		float t = s.introBossPreSpawnElapsed_ / s.introBossPreSpawnSec_;
		t = std::clamp(t, 0.0f, 1.0f);

		while (s.introBossPreSpawnEmitAccum_ >= 0.08f) {
			s.introBossPreSpawnEmitAccum_ -= 0.08f;

			if (t < 0.45f) {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossPos_, 6);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossPos_, 3);
			} else if (t < 0.80f) {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossPos_, 12);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossPos_, 6);
				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossPos_, 2);
			} else {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossPos_, 16);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossPos_, 8);
				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossPos_, 4);
			}
		}

		if (!s.introBossSpawnFxFinished_ && t >= 0.82f) {
			s.introBossSpawnFxFinished_ = true;
			ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossPos_, 20);
			ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossPos_, 40);
		}

		// ゆがみ演出が終わったら、ここで初めてボス本体登場へ
		if (s.introBossPreSpawnElapsed_ >= s.introBossPreSpawnSec_) {
			if (!camera || !s.object3dCommon_ || s.introBoss_) { return; }

			s.introBoss_ = std::make_unique<BossEnemy>();
			s.introBoss_->Initialize(s.object3dCommon_, s.dxCommon_);
			s.introBoss_->SetCamera(camera);
			s.introBoss_->SetPosition({ 0.0f, 6.0f, s.introBossAppearStartZ_ });
			s.introBoss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
			s.introBoss_->SetLocked(true);
			s.introBoss_->SyncTransform();

			s.introBossPos_ = { 0.0f, 6.0f, s.introBossAppearStartZ_ };
			s.introBossBasePos_ = s.introBossPos_;

			s.introBossPhaseElapsed_ = 0.0f;
			s.phase_ = IntroSequence::Phase::BossAppear;

			// BossPreSpawnですでにボス方向へのカメラブレンドは始まっている
			s.camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };
			s.camBlendBackActive_ = false;

			ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossPos_, 12);

			s.flowSM_.Change(std::make_unique<IntroBossAppearState>());
		}
	}

	void IntroBossAppearState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossAppear;
	}

	void IntroBossAppearState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!s.introBoss_ || !camera) { return; }

		s.introBossPhaseElapsed_ += IntroSequence::kFixedDt_;
		float t = s.introBossPhaseElapsed_ / s.introBossAppearSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		float moveT = t;
		moveT = moveT * moveT * (3.0f - 2.0f * moveT);

		float z = MyMath::Lerp(s.introBossAppearStartZ_, s.introBossAppearEndZ_, moveT);

		float floatX = std::sinf(s.introBossPhaseElapsed_ * s.introBossAppearFloatFreqX_) * s.introBossAppearFloatAmpX_;

		float floatYMain =
			std::sinf(s.introBossPhaseElapsed_ * s.introBossAppearFloatFreqY_) * s.introBossAppearFloatAmpY_;

		float floatYSub =
			std::sinf(s.introBossPhaseElapsed_ * (s.introBossAppearFloatFreqY_ * 2.15f) + 0.8f) *
			(s.introBossAppearFloatAmpY_ * 0.38f);

		float floatY = floatYMain + floatYSub;

		float damp = MyMath::Lerp(1.0f, 0.45f, moveT);

		s.introBossPos_.z = z;
		s.introBossPos_.x = floatX * damp;
		float bodyDrift = std::sinf(s.introBossPhaseElapsed_ * 0.95f + 1.2f) * 0.9f;
		s.introBossPos_.y = 6.0f + bodyDrift + floatY * damp;

		float rotZ =
			std::sinf(s.introBossPhaseElapsed_ * 2.2f) * s.introBossAppearTiltZ_ * damp +
			std::sinf(s.introBossPhaseElapsed_ * 4.6f + 0.5f) * (s.introBossAppearTiltZ_ * 0.35f) * damp;

		s.introBoss_->SetPosition(s.introBossPos_);
		s.introBoss_->SetRotate({ 0.0f, 3.14159265f, rotZ });
		s.introBoss_->SetIntroPanic(false, 0.0f);
		s.introBoss_->Update(IntroSequence::kFixedDt_);

		s.camBossTargetRot_ = {
			MyMath::Lerp(0.10f, 0.06f, t),
			MyMath::Lerp(-0.10f, 0.0f, t),
			0.0f
		};

		if (t >= 1.0f) {
			s.introBossPhaseElapsed_ = 0.0f;
			s.introBossBasePos_ = s.introBossPos_;
			s.phase_ = IntroSequence::Phase::BossPause;
			s.flowSM_.Change(std::make_unique<IntroBossPauseState>());
		}
	}

	void IntroBossPauseState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossPause;
	}

	void IntroBossPauseState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!s.introBoss_ || !camera) { return; }

		s.introBossPhaseElapsed_ += IntroSequence::kFixedDt_;

		float t = s.introBossPhaseElapsed_ / s.introBossPauseSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		s.introBossPos_ = s.introBossBasePos_;
		s.introBoss_->SetPosition(s.introBossPos_);
		s.introBoss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

		float idleY = std::sinf(s.introBossPhaseElapsed_ * 5.0f) * 0.10f;
		s.introBoss_->SetPosition({ s.introBossPos_.x, s.introBossPos_.y + idleY, s.introBossPos_.z });

		s.introBoss_->SetIntroPanic(false, 0.0f);
		s.introBoss_->Update(IntroSequence::kFixedDt_);

		s.camBossTargetRot_ = { 0.055f, 0.0f, 0.0f };

		if (t >= 1.0f) {
			s.introBossPhaseElapsed_ = 0.0f;
			s.introBossBasePos_ = { s.introBossPos_.x, s.introBossPos_.y + idleY, s.introBossPos_.z };
			s.phase_ = IntroSequence::Phase::BossNoticeHop;
			s.flowSM_.Change(std::make_unique<IntroBossNoticeHopState>());
		}
	}

	void IntroBossNoticeHopState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossNoticeHop;
	}

	void IntroBossNoticeHopState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!s.introBoss_ || !camera) { return; }

		s.introBossPhaseElapsed_ += IntroSequence::kFixedDt_;

		float t = s.introBossPhaseElapsed_ / s.introBossNoticeHopSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		float hopT = 0.0f;
		if (t < 0.20f) {
			hopT = 0.0f;
		} else {
			hopT = (t - 0.20f) / 0.80f;
			if (hopT > 1.0f) { hopT = 1.0f; }
		}

		if (!s.introBossNoticeMarkEmitted_ && t >= 0.20f) {
			s.introBossNoticeMarkEmitted_ = true;

			const Vector3 center = s.introBossBasePos_ + Vector3{ 0.0f, 3.2f, -3.0f };

			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -7.0f,  2.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 7.0f,  2.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -9.0f,  0.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 9.0f,  0.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 6.0f, -2.5f, 0.0f }, 1);
		}

		float hop = std::sinf(hopT * 3.14159265f);
		float hopY = hop * s.introBossNoticeHopY_;
		float surpriseX = std::sinf(t * 3.14159265f) * 0.35f;

		Vector3 pos = s.introBossBasePos_;
		pos.x += surpriseX;
		pos.y += hopY;

		float rotZ = std::sinf(t * 3.14159265f) * 0.12f;

		s.introBossPos_ = pos;
		s.introBoss_->SetPosition(pos);
		s.introBoss_->SetRotate({ 0.0f, 3.14159265f, rotZ });

		if (t < 0.20f) {
			s.introBoss_->SetIntroPanic(false, 0.0f);
		} else {
			s.introBoss_->SetIntroPanic(true, 0.85f);
		}

		s.camBossTargetRot_ = {
			0.045f,
			0.015f,
			0.0f
		};

		s.introBoss_->Update(IntroSequence::kFixedDt_);

		if (t >= 1.0f) {
			s.introBoss_->SetIntroPanic(true, 1.0f);
			s.introBossPhaseElapsed_ = 0.0f;
			s.introBossBasePos_ = s.introBossPos_;
			s.introBossNoticeMarkEmitted_ = false;
			s.phase_ = IntroSequence::Phase::BossPanic;
			s.flowSM_.Change(std::make_unique<IntroBossPanicState>());
		}
	}

	void IntroBossPanicState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossPanic;
	}

	void IntroBossPanicState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!s.introBoss_ || !camera) { return; }

		s.introBossPhaseElapsed_ += IntroSequence::kFixedDt_;
		float t = s.introBossPhaseElapsed_ / s.introBossPanicSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		float shakeX = std::sinf(s.introBossPhaseElapsed_ * 12.0f) * s.introBossPanicAmpX_ * (0.30f + t * 0.70f);
		float shakeY = std::fabs(std::sinf(s.introBossPhaseElapsed_ * 15.0f)) * s.introBossPanicAmpY_;
		float wobbleRotZ = std::sinf(s.introBossPhaseElapsed_ * 13.0f) * 0.14f;

		s.introBossPos_.x = s.introBossBasePos_.x + shakeX;
		s.introBossPos_.y = s.introBossBasePos_.y + shakeY;
		s.introBossPos_.z = s.introBossBasePos_.z;

		s.introBoss_->SetPosition(s.introBossPos_);
		s.introBoss_->SetRotate({ 0.0f, 3.14159265f, wobbleRotZ });
		s.introBoss_->SetIntroPanic(true, 0.55f + t * 0.45f);
		s.introBoss_->Update(IntroSequence::kFixedDt_);

		s.camBossTargetRot_ = { 0.06f, 0.0f, 0.0f };

		if (t >= 1.0f) {
			s.introBossPhaseElapsed_ = 0.0f;
			s.introBossEscapeTargetX_ = s.introBossPos_.x;
			s.introBossEscapeTargetTimer_ = 0.0f;
			s.phase_ = IntroSequence::Phase::BossEscape;
			s.flowSM_.Change(std::make_unique<IntroBossEscapeState>());
		}
	}

	void IntroBossEscapeState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossEscape;
	}

	void IntroBossEscapeState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!s.introBoss_ || !camera) { return; }

		s.introBossPhaseElapsed_ += IntroSequence::kFixedDt_;
		s.introBossEscapeTargetTimer_ += IntroSequence::kFixedDt_;

		float t = s.introBossPhaseElapsed_ / s.introBossEscapeSec_;
		if (t < 0.0f) t = 0.0f;
		if (t > 1.0f) t = 1.0f;

		if (s.introBossEscapeTargetTimer_ >= s.introBossEscapeTargetInterval_) {
			s.introBossEscapeTargetTimer_ = 0.0f;
			float sign = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
			float ampGrow = MyMath::Lerp(0.65f, 1.25f, t);
			float mag = 2.0f + (static_cast<float>(std::rand()) / RAND_MAX) * s.introBossEscapeAmpX_ * ampGrow;
			s.introBossEscapeTargetX_ = sign * mag;
		}

		float follow = 16.0f * IntroSequence::kFixedDt_;
		s.introBossPos_.x = MyMath::Lerp(s.introBossPos_.x, s.introBossEscapeTargetX_, follow);
		s.introBossPos_.z += s.introBossEscapeSpeedZ_ * IntroSequence::kFixedDt_ * (1.0f + t * 0.30f);
		s.introBossPos_.y = 6.0f + std::fabs(std::sinf(s.introBossPhaseElapsed_ * 14.0f)) * s.introBossEscapeHopY_;

		float leanZ = std::sinf(s.introBossPhaseElapsed_ * 16.0f) * 0.22f;

		s.introBoss_->SetPosition(s.introBossPos_);
		s.introBoss_->SetRotate({ 0.0f, 3.14159265f + std::sinf(s.introBossPhaseElapsed_ * 8.0f) * 0.10f, leanZ });
		s.introBoss_->SetIntroPanic(true, 1.0f);
		s.introBoss_->Update(IntroSequence::kFixedDt_);

		s.camBossTargetRot_ = { 0.05f, 0.0f, 0.0f };

		if (s.introBossPos_.z >= s.introBossEscapeEndZ_ || t >= 1.0f) {
			s.introBoss_->SetIntroPanic(false, 0.0f);

			if (!s.introBossEscapeWarpBurstEmitted_) {
				s.introBossEscapeWarpBurstEmitted_ = true;

				ParticleManager::GetInstance()->Emit("bossEscape_warpCore", s.introBossPos_, 10);
				ParticleManager::GetInstance()->Emit("bossEscape_warpSwirl", s.introBossPos_, 36);
				ParticleManager::GetInstance()->Emit("bossEscape_warpShred", s.introBossPos_, 20);
				ParticleManager::GetInstance()->Emit("bossEscape_warpRing", s.introBossPos_, 1);
			}

			s.introBoss_.reset();

			s.camReturnStartRot_ = camera->GetRotate();
			s.camBlendBackActive_ = true;
			s.camBlendToBossActive_ = false;
			s.camBlendBackTween_.Reset(0.0f, 1.0f, s.camBlendBackSec_, Ease::Type::InOutSine);

			s.phase_ = IntroSequence::Phase::ShowStart;
			s.flowSM_.Change(std::make_unique<IntroShowStartState>());
			return;
		}

		ParticleManager::GetInstance()->Emit("bossEscape_warpSwirl", s.introBossPos_, 5);
		ParticleManager::GetInstance()->Emit("bossEscape_warpShred", s.introBossPos_, 3);
	}

	void IntroShowStartState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::ShowStart;
	}

	void IntroShowStartState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);

		if (s.phase_ == IntroSequence::Phase::ShowStart && !s.startPlayed_ && !s.camBlendBackActive_) {
			s.startPlayed_ = true;
			s.startVisible_ = true;
			s.startSlideIn_ = true;
			s.startFadeOut_ = false;
			s.startHoldElapsed_ = 0.0f;
			s.startAlpha_ = 1.0f;
			s.startSprite_->SetColor({ 1,1,1,s.startAlpha_ });
			s.startSprite_->SetPosition({ s.startStartPos_.x, s.startEndPos_.y });
			s.startTween_.Reset(0.0f, 1.0f, s.startDuration_, Ease::Type::OutBack);
		}

		if (s.startSlideIn_) {
			float startT = s.startTween_.Update(IntroSequence::kFixedDt_);

			if (s.startGlowOn_) {
				float glow = 1.0f + s.startGlowAmp_ * std::sin(startT * MyMath::GetPI());
				s.startSprite_->SetColor({ glow, glow, glow, s.startAlpha_ });
			} else {
				s.startSprite_->SetColor({ 1,1,1,s.startAlpha_ });
			}

			float x = MyMath::Lerp(s.startStartPos_.x, s.startEndPos_.x, startT);
			float y = s.startEndPos_.y;
			s.startSprite_->SetPosition({ x, y });
			s.startSprite_->Update();

			if (s.startTween_.Finished()) {
				s.startSlideIn_ = false;
				s.startHoldElapsed_ = 0.0f;
			}
		} else if (s.startVisible_) {
			if (!s.startFadeOut_ && s.startGlowOn_) {
				float t01 = (s.startHoldSec_ > 0.0f) ? std::min(s.startHoldElapsed_ / s.startHoldSec_, 1.0f) : 1.0f;
				float decay = 1.0f - 0.7f * t01;
				float glow = 1.0f + decay * 0.20f * std::sin(s.startHoldElapsed_ * s.startGlowSpeed_);
				s.startSprite_->SetColor({ glow, glow, glow, s.startAlpha_ });
			}

			if (!s.startFadeOut_) {
				s.startHoldElapsed_ += IntroSequence::kFixedDt_;
				if (s.startHoldElapsed_ >= s.startHoldSec_) {
					s.startFadeOut_ = true;
				}
			}

			if (s.startFadeOut_) {
				s.startAlpha_ -= IntroSequence::kFixedDt_ / s.startFadeSec_;
				if (s.startAlpha_ <= 0.0f) {
					s.startAlpha_ = 0.0f;
					s.startVisible_ = false;
					s.phase_ = IntroSequence::Phase::Done;

					if (!s.currentEnemiesInitialized_ && s.currentOutRequestInitEnemies_) {
						*s.currentOutRequestInitEnemies_ = true;
					}
					s.gameplayLocked_ = false;

					s.flowSM_.Change(std::make_unique<IntroDoneState>());
					return;
				}
				s.startSprite_->SetColor({ 1,1,1,s.startAlpha_ });
			}

			s.startSprite_->Update();
		}
	}

	void IntroDoneState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::Done;
	}

	void IntroDoneState::Update(IStateContext& ctx, float dt) {
		(void)ctx;
		(void)dt;
	}

}