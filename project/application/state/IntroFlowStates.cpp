#include "IntroFlowStates.h"
#include "IntroSequence.h"
#include "ParticleManager.h"
#include "AudioManager.h"
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
		s.introBossActor_.BeginPreSpawn();

		// カメラはもうボス出現位置を見に行く
		s.camSavedRot_ = camera->GetRotate();
		s.camBossStartRot_ = s.camSavedRot_;
		s.camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };

		s.camBlendToBossActive_ = true;
		s.camBlendBackActive_ = false;
		s.camBlendToBossTween_.Reset(0.0f, 1.0f, s.camBlendToBossSec_, Ease::Type::InOutSine);

		const Vector3& pos = s.introBossActor_.GetPosition();
		ParticleManager::GetInstance()->Emit("bossWarp_core", pos, 4);
		ParticleManager::GetInstance()->Emit("bossWarp_swirl", pos, 18);
		ParticleManager::GetInstance()->Emit("bossWarp_dust", pos, 8);
	}

	void IntroBossPreSpawnState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera) { return; }

		s.introBossActor_.UpdatePreSpawn(IntroSequence::kFixedDt_);

		float t = std::clamp(s.introBossActor_.GetPreSpawnElapsed() / 1.8f, 0.0f, 1.0f);

		float emitAccum = s.introBossActor_.GetPreSpawnEmitAccum();
		while (emitAccum >= 0.08f) {
			emitAccum -= 0.08f;

			if (t < 0.45f) {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 6);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossActor_.GetPosition(), 3);
			} else if (t < 0.80f) {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 12);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossActor_.GetPosition(), 6);
				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 2);
			} else {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 16);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossActor_.GetPosition(), 8);
				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 4);
			}
		}
		s.introBossActor_.SetPreSpawnEmitAccum(emitAccum);

		if (!s.introBossActor_.IsSpawnFxFinished() && t >= 0.82f) {
			s.introBossActor_.SetSpawnFxFinished(true);
			ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 20);
			ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 40);
		}

		// ボス出現
		if (s.introBossActor_.IsPreSpawnFinished()) {
			if (!s.introBossActor_.Exists()) {
				s.introBossActor_.Spawn(camera);
				s.phase_ = IntroSequence::Phase::BossAppear;

				s.camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };
				s.camBlendBackActive_ = false;

				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 12);
				s.flowSM_.Change(std::make_unique<IntroBossAppearState>());
			}
		}
	}

	void IntroBossAppearState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossAppear;
		s.introBossActor_.BeginAppear();
	}

	void IntroBossAppearState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera || !s.introBossActor_.Exists()) { return; }

		float t = s.introBossActor_.GetAppearRatio();
		bool finished = s.introBossActor_.UpdateAppear(IntroSequence::kFixedDt_);

		s.camBossTargetRot_ = {
			MyMath::Lerp(0.10f, 0.06f, t),
			MyMath::Lerp(-0.10f, 0.0f, t),
			0.0f
		};

		if (finished) {
			s.phase_ = IntroSequence::Phase::BossPause;
			s.flowSM_.Change(std::make_unique<IntroBossPauseState>());
		}
	}

	void IntroBossPauseState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossPause;
		s.introBossActor_.BeginPause();
	}

	void IntroBossPauseState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera || !s.introBossActor_.Exists()) { return; }

		bool finished = s.introBossActor_.UpdatePause(IntroSequence::kFixedDt_);
		s.camBossTargetRot_ = { 0.055f, 0.0f, 0.0f };

		if (finished) {
			s.phase_ = IntroSequence::Phase::BossNoticeHop;
			s.flowSM_.Change(std::make_unique<IntroBossNoticeHopState>());
		}
	}

	void IntroBossNoticeHopState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossNoticeHop;
		s.introBossActor_.BeginNoticeHop();
	}

	void IntroBossNoticeHopState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera || !s.introBossActor_.Exists()) { return; }

		float t = s.introBossActor_.GetNoticeHopRatio();

		if (!s.introBossActor_.IsNoticeMarkEmitted() && t >= 0.20f) {
			s.introBossActor_.SetNoticeMarkEmitted(true);

			const Vector3 center = s.introBossActor_.GetBasePosition() + Vector3{ 0.0f, 3.2f, -3.0f };

			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -7.0f,  2.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 7.0f,  2.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -9.0f,  0.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 9.0f,  0.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 6.0f, -2.5f, 0.0f }, 1);

			AudioManager::GetInstance()->PlaySound("surprise", 0.4f);
		}

		bool finished = s.introBossActor_.UpdateNoticeHop(IntroSequence::kFixedDt_);

		s.camBossTargetRot_ = {
			0.045f,
			0.015f,
			0.0f
		};

		if (finished) {
			s.phase_ = IntroSequence::Phase::BossPanic;
			s.flowSM_.Change(std::make_unique<IntroBossPanicState>());
		}
	}

	void IntroBossPanicState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossPanic;
		s.introBossActor_.BeginPanic();
	}

	void IntroBossPanicState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera || !s.introBossActor_.Exists()) { return; }

		bool finished = s.introBossActor_.UpdatePanic(IntroSequence::kFixedDt_);
		s.camBossTargetRot_ = { 0.06f, 0.0f, 0.0f };

		if (finished) {
			s.phase_ = IntroSequence::Phase::BossEscape;
			s.flowSM_.Change(std::make_unique<IntroBossEscapeState>());
		}
	}

	void IntroBossEscapeState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::BossEscape;
		s.introBossActor_.BeginEscape();
	}

	void IntroBossEscapeState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;
		if (!camera || !s.introBossActor_.Exists()) { return; }

		bool finished = s.introBossActor_.UpdateEscape(IntroSequence::kFixedDt_);
		s.camBossTargetRot_ = { 0.05f, 0.0f, 0.0f };

		if (finished) {
			if (!s.introBossActor_.IsEscapeWarpBurstEmitted()) {
				s.introBossActor_.SetEscapeWarpBurstEmitted(true);

				const Vector3 pos = s.introBossActor_.GetPosition();
				ParticleManager::GetInstance()->Emit("bossEscape_warpCore", pos, 10);
				ParticleManager::GetInstance()->Emit("bossEscape_warpSwirl", pos, 36);
				ParticleManager::GetInstance()->Emit("bossEscape_warpShred", pos, 20);
				ParticleManager::GetInstance()->Emit("bossEscape_warpRing", pos, 1);
			}

			s.introBossActor_.Reset();

			s.camReturnStartRot_ = camera->GetRotate();
			s.camBlendBackActive_ = true;
			s.camBlendToBossActive_ = false;
			s.camBlendBackTween_.Reset(0.0f, 1.0f, s.camBlendBackSec_, Ease::Type::InOutSine);

			s.phase_ = IntroSequence::Phase::ShowStart;
			s.flowSM_.Change(std::make_unique<IntroShowStartState>());
			return;
		}

		const Vector3 pos = s.introBossActor_.GetPosition();
		ParticleManager::GetInstance()->Emit("bossEscape_warpSwirl", pos, 5);
		ParticleManager::GetInstance()->Emit("bossEscape_warpShred", pos, 3);
	}

	void IntroShowStartState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		s.phase_ = IntroSequence::Phase::ShowStart;
	}

	void IntroShowStartState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);

		if (!s.camBlendBackActive_) {
			s.startBanner_.Start();
		}

		s.startBanner_.Update(IntroSequence::kFixedDt_);

		if (s.startBanner_.IsFinished()) {
			s.phase_ = IntroSequence::Phase::Done;

			if (!s.currentEnemiesInitialized_ && s.currentOutRequestInitEnemies_) {
				*s.currentOutRequestInitEnemies_ = true;
			}
			s.gameplayLocked_ = false;

			s.flowSM_.Change(std::make_unique<IntroDoneState>());
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