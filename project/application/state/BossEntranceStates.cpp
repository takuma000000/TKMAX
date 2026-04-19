#include "BossEntranceStates.h"
#include "BossEntranceSequence.h"
#include "manager/BossManager.h"
#include "ParticleManager.h"
#include <algorithm>

namespace {
	BossEntranceSequence& AsBossEntrance_(TKM::IStateContext& ctx) {
		return static_cast<BossEntranceSequence&>(ctx);
	}
}

//=====================================================
// Wait
//=====================================================
void BossEntranceWaitState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ = 0.0f;
	s.currentSkyColor_ = s.kBaseSkyColor_;
}

void BossEntranceWaitState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ += dt;
	s.currentSkyColor_ = s.kBaseSkyColor_;

	if (s.phaseTimer_ >= s.kWaitSec_) {
		s.sm_.Change(std::make_unique<BossEntranceSkyFadeInState>());
	}
}

//=====================================================
// SkyFadeIn
//=====================================================
void BossEntranceSkyFadeInState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ = 0.0f;
}

void BossEntranceSkyFadeInState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ += dt;

	const float t = std::clamp(s.phaseTimer_ / s.kSkyFadeInSec_, 0.0f, 1.0f);
	s.currentSkyColor_ = BossEntranceSequence::LerpColor_(s.kBaseSkyColor_, s.kRedSkyColor_, t);

	if (s.phaseTimer_ >= s.kSkyFadeInSec_) {
		s.convergeEmitTimer_ = 0.0f;
		s.glowEmitTimer_ = 0.0f;
		s.currentSkyColor_ = s.kRedSkyColor_;
		s.sm_.Change(std::make_unique<BossEntranceGatherState>());
	}
}

//=====================================================
// Gather
//=====================================================
void BossEntranceGatherState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ = 0.0f;
	s.convergeEmitTimer_ = 0.0f;
	s.glowEmitTimer_ = 0.0f;
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceGatherState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ += dt;
	s.currentSkyColor_ = s.kRedSkyColor_;

	s.convergeEmitTimer_ += dt;
	s.glowEmitTimer_ += dt;

	while (s.convergeEmitTimer_ >= s.kGatherEmitInterval_) {
		s.convergeEmitTimer_ -= s.kGatherEmitInterval_;
		s.EmitGather_();
	}

	while (s.glowEmitTimer_ >= s.kRingEmitInterval_) {
		s.glowEmitTimer_ -= s.kRingEmitInterval_;

		auto* pm = TKM::ParticleManager::GetInstance();
		if (pm) {
			pm->Emit("bossEntrance_ringThin", s.spawnPos_, 1);
		}
	}

	if (s.phaseTimer_ >= s.kGatherSec_) {
		s.sm_.Change(std::make_unique<BossEntranceCoverState>());
	}
}

//=====================================================
// Cover
//=====================================================
void BossEntranceCoverState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ = 0.0f;
	s.coverEmitTimer_ = 0.0f;
	s.coverEmitCount_ = 0;
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceCoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ += dt;
	s.currentSkyColor_ = s.kRedSkyColor_;

	s.coverEmitTimer_ += dt;
	while (s.coverEmitTimer_ >= s.kCoverEmitInterval_) {
		s.coverEmitTimer_ -= s.kCoverEmitInterval_;

		s.EmitCover_();
		++s.coverEmitCount_;

		if (!s.bossSpawned_ && s.coverEmitCount_ >= s.kCoverSpawnEmitCount_) {
			if (s.bossManager_) {
				s.bossManager_->SpawnForEntrance();

				if (BossEnemy* boss = s.bossManager_->GetBoss()) {
					s.burstStartPos_ = s.spawnPos_ + Vector3{ 0.0f, 2.2f, -10.0f };
					s.burstEndPos_ = s.spawnPos_;
					s.burstStartScale_ = { 1.6f, 1.6f, 1.6f };
					s.burstEndScale_ = { 5.0f, 5.0f, 5.0f };

					boss->SetPosition(s.burstStartPos_);
					boss->SetScale(s.burstStartScale_);
					boss->SyncTransform();
				}
			}
			s.bossSpawned_ = true;
		}
	}

	if (s.phaseTimer_ >= s.kCoverSec_) {
		s.burstFxEmitted_ = false;
		s.sm_.Change(std::make_unique<BossEntranceBurstState>());
	}
}

//=====================================================
// Burst
//=====================================================
void BossEntranceBurstState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ = 0.0f;
	s.burstFxEmitted_ = false;
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceBurstState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ += dt;
	s.currentSkyColor_ = s.kRedSkyColor_;

	if (!s.burstFxEmitted_) {
		s.EmitBurst_();
		s.burstFxEmitted_ = true;
	}

	if (s.bossManager_) {
		if (BossEnemy* boss = s.bossManager_->GetBoss()) {
			float t = std::clamp(s.phaseTimer_ / s.kBurstSec_, 0.0f, 1.0f);
			float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

			Vector3 pos{};
			pos.x = MyMath::Lerp(s.burstStartPos_.x, s.burstEndPos_.x, e);
			pos.y = MyMath::Lerp(s.burstStartPos_.y, s.burstEndPos_.y, e);
			pos.z = MyMath::Lerp(s.burstStartPos_.z, s.burstEndPos_.z, e);

			Vector3 scale{};
			scale.x = MyMath::Lerp(s.burstStartScale_.x, s.burstEndScale_.x, e);
			scale.y = MyMath::Lerp(s.burstStartScale_.y, s.burstEndScale_.y, e);
			scale.z = MyMath::Lerp(s.burstStartScale_.z, s.burstEndScale_.z, e);

			boss->SetPosition(pos);
			boss->SetScale(scale);
			boss->SyncTransform();
		}
	}

	if (s.phaseTimer_ >= s.kBurstSec_) {
		if (s.bossManager_) {
			if (BossEnemy* boss = s.bossManager_->GetBoss()) {
				boss->SetPosition(s.burstEndPos_);
				boss->SetScale(s.burstEndScale_);
				boss->SyncTransform();
			}
			s.bossManager_->BeginBattle();
		}

		s.holdEmitTimer_ = 0.0f;
		s.sm_.Change(std::make_unique<BossEntrancePushState>());
	}
}

//=====================================================
// Push
//=====================================================
void BossEntrancePushState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ = 0.0f;
	s.holdEmitTimer_ = 0.0f;
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntrancePushState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ += dt;
	s.currentSkyColor_ = s.kRedSkyColor_;

	s.holdEmitTimer_ += dt;
	while (s.holdEmitTimer_ >= s.kPushEmitInterval_) {
		s.holdEmitTimer_ -= s.kPushEmitInterval_;
		s.EmitPushWave_();
	}

	if (s.phaseTimer_ >= s.kPushSec_) {
		s.sm_.Change(std::make_unique<BossEntranceDoneState>());
	}
}

//=====================================================
// Done
//=====================================================
void BossEntranceDoneState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);
	s.phaseTimer_ = 0.0f;
	s.isActive_ = false;
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceDoneState::Update(TKM::IStateContext& ctx, float dt) {
	(void)ctx;
	(void)dt;
}