#include "ClearComedyStates.h"
#include "GameClearScene.h"
#include "ParticleManager.h"
#include "AudioManager.h"
#include <algorithm>

namespace {
	GameClearScene& AsClear_(TKM::IStateContext& ctx) {
		return static_cast<GameClearScene&>(ctx);
	}
}

//=====================================================
// WaitAfterClear
//=====================================================
void ClearComedyWaitAfterClearState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyWaitAfterClearState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	s.clearComedyTimeScale_.Update(s.dt_);
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	s.clearComedyTimer_ += comedyDt;

	if (s.clearComedyTimer_ >= 0.85f) {
		s.SpawnClearComedyActors_();
		s.clearComedySM_.Change(std::make_unique<ClearComedySpawnState>());
	}
}

//=====================================================
// Spawn
//=====================================================
void ClearComedySpawnState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedySpawnState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	s.clearComedyTimeScale_.Update(s.dt_);
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	s.clearComedyTimer_ += comedyDt;

	if (s.clearComedyBoss_) {
		s.clearComedyBoss_->SetIntroPanic(true, 0.35f);
		s.clearComedyBoss_->Update(comedyDt);
	}
	if (s.clearComedyMobA_) {
		s.clearComedyMobA_->Update(comedyDt);
	}
	if (s.clearComedyMobB_) {
		s.clearComedyMobB_->Update(comedyDt);
	}

	if (s.clearComedyTimer_ >= 0.65f) {

		if (!s.clearComedyNoticeMarkPlayed_) {
			auto* pm = TKM::ParticleManager::GetInstance();

			if (s.clearComedyBoss_) {
				Vector3 p = s.clearComedyBoss_->GetWorldPosition() + Vector3{ 0.0f, 6.0f, 0.0f } + s.clearParticleGlobalOffset_;
				pm->Emit("bossNoticeMark", p, 1);
			}

			if (s.clearComedyMobA_) {
				Vector3 p = s.clearComedyMobA_->GetWorldPosition() + Vector3{ 0.0f, 3.0f, 0.0f };
				pm->Emit("bossNoticeMark", p, 1);
			}

			if (s.clearComedyMobB_) {
				Vector3 p = s.clearComedyMobB_->GetWorldPosition() + Vector3{ 0.0f, 3.0f, 0.0f };
				pm->Emit("bossNoticeMark", p, 1);
			}

			s.clearComedyNoticeMarkPlayed_ = true;
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedySlowNoticeState>());
	}
}

//=====================================================
// SlowNotice
//=====================================================
void ClearComedySlowNoticeState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedySlowNoticeState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	s.clearComedyTimeScale_.Update(s.dt_);
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	s.clearComedyTimer_ += comedyDt;

	if (s.clearComedyBoss_) {
		s.clearComedyBoss_->SetIntroPanic(true, 1.0f);
		s.clearComedyBoss_->Update(comedyDt);
	}
	if (s.clearComedyMobA_) {
		s.clearComedyMobA_->Update(comedyDt);
	}
	if (s.clearComedyMobB_) {
		s.clearComedyMobB_->Update(comedyDt);
	}

	if (s.clearComedyTimer_ >= 0.75f) {
		// RunAway の開始位置を、この瞬間の見た目位置で確定
		if (s.clearComedyBoss_) {
			s.clearComedyBossRunStartPos_ = s.clearComedyBoss_->GetWorldPosition();
		}
		if (s.clearComedyMobA_) {
			s.clearComedyMobARunStartPos_ = s.clearComedyMobA_->GetWorldPosition();
		}
		if (s.clearComedyMobB_) {
			s.clearComedyMobBRunStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		s.clearComedyFallSlowRequested_ = false;
		s.clearComedySM_.Change(std::make_unique<ClearComedyRunAwayState>());
	}
}

//=====================================================
// RunAway
//=====================================================
void ClearComedyRunAwayState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyRunAwayState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	s.clearComedyTimeScale_.Update(s.dt_);
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	s.clearComedyTimer_ += comedyDt;

	float t = std::clamp(s.clearComedyTimer_ / 1.35f, 0.0f, 1.0f);
	float bossMoveT = Ease::Eval(Ease::Type::InQuad, std::clamp(s.clearComedyTimer_ / 1.80f, 0.0f, 1.0f));
	float mobMoveT = Ease::Eval(Ease::Type::InQuad, t);

	if (s.clearComedyBoss_) {
		Vector3 pos = MyMath::Vector3Lerp(s.clearComedyBossRunStartPos_, s.clearComedyBossEscapePos_, bossMoveT);
		s.clearComedyBoss_->SetPosition(pos);
		s.clearComedyBoss_->SetRotate({ 0.0f, -0.9f, 0.0f });
		s.clearComedyBoss_->SetIntroPanic(true, 0.75f);
		s.clearComedyBoss_->SyncTransform();
		s.clearComedyBoss_->Update(comedyDt);
	}

	if (s.clearComedyMobA_) {
		Vector3 pos = MyMath::Vector3Lerp(s.clearComedyMobARunStartPos_, s.clearComedyMobAEscapePos_, mobMoveT);
		s.clearComedyMobA_->SetPosition(pos);
		s.clearComedyMobA_->SetRotate({ 0.0f, -0.9f, 0.0f });
		s.clearComedyMobA_->SyncTransform();
		s.clearComedyMobA_->Update(comedyDt);
	}

	if (s.clearComedyMobB_) {
		Vector3 pos = MyMath::Vector3Lerp(s.clearComedyMobBRunStartPos_, s.clearComedyMobBFallPos_, mobMoveT);
		s.clearComedyMobB_->SetPosition(pos);
		s.clearComedyMobB_->SetRotate({ 0.0f, -0.9f, 0.0f });
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);
	}

	if (s.clearComedyTimer_ >= 1.35f) {
		if (s.clearComedyBoss_) {
			s.clearComedyBossRecoverStartPos_ = s.clearComedyBoss_->GetWorldPosition();
		}
		if (s.clearComedyMobA_) {
			s.clearComedyMobARecoverStartPos_ = s.clearComedyMobA_->GetWorldPosition();
		}
		if (s.clearComedyMobB_) {
			s.clearComedyMobBRecoverStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedyFallDownState>());
	}
}

//=====================================================
// FallDown
//=====================================================
void ClearComedyFallDownState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyFallDownState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	s.clearComedyTimeScale_.Update(s.dt_);
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	s.clearComedyTimer_ += comedyDt;

	float t = std::clamp(s.clearComedyTimer_ / 0.85f, 0.0f, 1.0f);

	// ボスは待たずにそのまま退場方向へ進む
	if (s.clearComedyBoss_) {
		float bossMoveT = Ease::Eval(Ease::Type::InQuad, t);
		Vector3 bossPos = MyMath::Vector3Lerp(
			s.clearComedyBossRecoverStartPos_,
			s.clearComedyBossExitPos_,
			bossMoveT
		);
		s.clearComedyBoss_->SetPosition(bossPos);
		s.clearComedyBoss_->SetRotate({ 0.0f, -1.00f, 0.0f });
		s.clearComedyBoss_->SetIntroPanic(false, 0.0f);
		s.clearComedyBoss_->SyncTransform();
		s.clearComedyBoss_->Update(comedyDt);
	}

	// 雑魚Aも待たずにそのまま退場方向へ進む
	if (s.clearComedyMobA_) {
		float mobAMoveT = Ease::Eval(Ease::Type::InQuad, t);
		Vector3 mobAPos = MyMath::Vector3Lerp(
			s.clearComedyMobARecoverStartPos_,
			s.clearComedyMobAExitPos_,
			mobAMoveT
		);
		s.clearComedyMobA_->SetPosition(mobAPos);
		s.clearComedyMobA_->SetRotate({ 0.0f, -1.00f, 0.0f });
		s.clearComedyMobA_->SyncTransform();
		s.clearComedyMobA_->Update(comedyDt);
	}

	// 雑魚Bだけ派手に転ぶ
	if (s.clearComedyMobB_) {
		Vector3 startPos = s.clearComedyMobBRecoverStartPos_;

		// まず一瞬浮くターゲット
		Vector3 popPos = startPos + Vector3{ 0.0f, 1.4f, 0.8f };

		// 最終的な転倒位置
		Vector3 slamPos = startPos + Vector3{ 0.0f, -1.8f, 2.8f };

		Vector3 pos{};
		Vector3 rot{};

		if (t < 0.35f) {

			if (!s.clearComedyFallSlowRequested_) {
				s.clearComedyTimeScale_.RequestSlowAdvanced(0.20f, 1.7f, 0.05f, 0.25f);
				s.clearComedyFallSlowRequested_ = true;
			}

			if (!s.clearComedyMobBSlipEffectPlayed_) {
				Vector3 slipPos = startPos + Vector3{ 1.0f, 0.1f, 0.35f } + s.clearParticleGlobalOffset_;

				auto* pm = TKM::ParticleManager::GetInstance();
				pm->Emit("clearComedySlip_streak", slipPos, 8);
				pm->Emit("clearComedySlip_spark", slipPos, 10);
				pm->Emit("clearComedySlip_ring", slipPos, 2);
				pm->Emit("clearComedySlip_chip", slipPos, 8);

				s.clearComedyMobBSlipEffectPlayed_ = true;

				TKM::AudioManager::GetInstance()->PlaySound("slip", 0.3f);
			}

			// 前半：一瞬ふわっと浮く
			float u = t / 0.35f;
			float jumpT = Ease::Eval(Ease::Type::OutQuad, u);

			pos = MyMath::Vector3Lerp(startPos, popPos, jumpT);

			// 少し前のめりになりながら浮く
			rot.x = MyMath::Lerp(0.0f, -0.35f, jumpT);
			rot.y = MyMath::Lerp(-0.9f, -0.75f, jumpT);
			rot.z = MyMath::Lerp(0.0f, 0.35f, jumpT);
		} else {
			// 後半：ズコーーーーっと落ちる
			float u = (t - 0.35f) / 0.65f;
			float slamT = Ease::Eval(Ease::Type::InExpo, u);

			pos = MyMath::Vector3Lerp(popPos, slamPos, slamT);

			// 一気に横倒れ
			rot.x = MyMath::Lerp(-0.35f, 0.15f, slamT);
			rot.y = MyMath::Lerp(-0.75f, s.clearComedyMobBFallRot_.y, slamT);
			rot.z = MyMath::Lerp(0.35f, 1.95f, slamT);
		}

		s.clearComedyMobB_->SetPosition(pos);
		s.clearComedyMobB_->SetRotate(rot);
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);

		if (!s.clearComedyMobBFallEffectPlayed_ && t >= 0.92f) {
			Vector3 fallFxPos = s.clearComedyMobB_->GetWorldPosition() + s.clearParticleGlobalOffset_;

			auto* pm = TKM::ParticleManager::GetInstance();
			// 着地時の転倒エフェクト
			pm->Emit("clearComedyFall_dust", fallFxPos, 10);
			pm->Emit("clearComedyFall_star", fallFxPos, 8);
			pm->Emit("clearComedyFall_line", fallFxPos, 8);
			pm->Emit("clearComedyFall_puff", fallFxPos, 6);


			s.clearComedyMobBFallEffectPlayed_ = true;
			TKM::AudioManager::GetInstance()->PlaySound("comedy", 0.35f);
		}
	}

	if (s.clearComedyTimer_ >= 0.85f) {
		s.clearComedyBoss_.reset();
		s.clearComedyMobA_.reset();

		if (s.clearComedyMobB_) {
			s.clearComedyMobBRecoverStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedyStandUpState>());
	}
}

//=====================================================
// StandUp
//=====================================================
void ClearComedyStandUpState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyStandUpState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	s.clearComedyTimeScale_.Update(s.dt_);
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	s.clearComedyTimer_ += comedyDt;

	float t = std::clamp(s.clearComedyTimer_ / 1.0f, 0.0f, 1.0f);
	float standT = Ease::Eval(Ease::Type::OutBack, t);

	if (s.clearComedyMobB_) {
		// 位置は動かさない。その場で起き上がる
		s.clearComedyMobB_->SetPosition(s.clearComedyMobBRecoverStartPos_);

		Vector3 rot = {
			0.0f,
			MyMath::Lerp(s.clearComedyMobBFallRot_.y, -1.05f, standT),
			MyMath::Lerp(1.95f, 0.0f, standT)
		};

		s.clearComedyMobB_->SetRotate(rot);
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);
	}

	if (s.clearComedyTimer_ >= 1.0f) {
		// 起き上がり終わった地点を逃走開始位置にする
		if (s.clearComedyMobB_) {
			s.clearComedyMobBRecoverStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedyRecoverRunState>());
	}
}

//=====================================================
// RecoverRun
//=====================================================
void ClearComedyRecoverRunState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyRecoverRunState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	s.clearComedyTimeScale_.Update(s.dt_);
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	s.clearComedyTimer_ += comedyDt;

	float t = std::clamp(s.clearComedyTimer_ / 1.00f, 0.0f, 1.0f);
	float moveT = Ease::Eval(Ease::Type::InCubic, t);

	// 起き上がった後に逃走
	if (s.clearComedyMobB_) {
		Vector3 pos = MyMath::Vector3Lerp(
			s.clearComedyMobBRecoverStartPos_,
			s.clearComedyMobBExitPos_,
			moveT
		);

		s.clearComedyMobB_->SetPosition(pos);
		s.clearComedyMobB_->SetRotate({ 0.0f, -1.05f, 0.0f });
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);
	}

	if (s.clearComedyTimer_ >= 1.00f) {
		s.clearComedyMobB_.reset();
		s.clearComedyActorsSpawned_ = false;

		{
			auto* pm = TKM::ParticleManager::GetInstance();

			Vector3 burstPos = s.playerDisplayPos_ + s.clearBannerBurstOffset_;

			pm->Emit("clearBannerBurst_core", burstPos, 6);
			pm->Emit("clearBannerBurst_confetti", burstPos, 70);
			pm->Emit("clearBannerBurst_ray", burstPos, 30);

			TKM::AudioManager::GetInstance()->PlaySound("clear_display", 0.3f);
		}

		s.isClearSpriteVisible_ = true;
		s.isClearMenuVisible_ = true;
		s.isClearSpritePopPlaying_ = true;
		s.clearSpritePopTime_ = 0.0f;
		s.clearSprite_->SetPosition(s.clearSpriteStartPos_);
		s.clearStageFireActive_ = true;
		s.clearStageFireTimer_ = 0.0f;

		s.clearComedySM_.Change(std::make_unique<ClearComedyDoneState>());
	}
}

//=====================================================
// Done
//=====================================================
void ClearComedyDoneState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyDoneState::Update(TKM::IStateContext& ctx, float dt) {
	(void)ctx;
	(void)dt;
}