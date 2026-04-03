#include "BossEntranceSequence.h"
#include "manager/BossManager.h"
#include "ParticleManager.h"
#include "AudioManager.h"
#include <algorithm>

Vector4 BossEntranceSequence::LerpColor_(const Vector4& a, const Vector4& b, float t) {
	t = std::clamp(t, 0.0f, 1.0f);

	Vector4 out{};
	out.x = MyMath::Lerp(a.x, b.x, t);
	out.y = MyMath::Lerp(a.y, b.y, t);
	out.z = MyMath::Lerp(a.z, b.z, t);
	out.w = MyMath::Lerp(a.w, b.w, t);
	return out;
}

void BossEntranceSequence::Reset_() {
	isActive_ = false;
	bossSpawned_ = false;
	phase_ = Phase::Idle;
	phaseTimer_ = 0.0f;
	convergeEmitTimer_ = 0.0f;
	glowEmitTimer_ = 0.0f;
	holdEmitTimer_ = 0.0f;
	coverEmitTimer_ = 0.0f;
	coverEmitCount_ = 0;
	currentSkyColor_ = kBaseSkyColor_;
	spawnPos_ = { 0.0f, 0.0f, 0.0f };
	burstFxEmitted_ = false;
	burstStartPos_ = { 0.0f, 0.0f, 0.0f };
	burstEndPos_ = { 0.0f, 0.0f, 0.0f };
	burstStartScale_ = { 1.0f, 1.0f, 1.0f };
	burstEndScale_ = { 1.0f, 1.0f, 1.0f };
}

void BossEntranceSequence::Start(const Vector3& spawnPos) {
	// すでに演出中なら何もしない
	if (isActive_) {
		return;
	}

	// 念のためリセットしてから開始
	Reset_();

	// 演出開始
	isActive_ = true;
	phase_ = Phase::Wait;
	spawnPos_ = spawnPos;
	currentSkyColor_ = kBaseSkyColor_;

	TKM::AudioManager::GetInstance()->PlaySound("bossPhaseBGM", 0.2f, true);
}

void BossEntranceSequence::EmitGather_() {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) {
		return;
	}

	pm->Emit("bossEntrance_gather", spawnPos_, 18);
	pm->Emit("bossEntrance_ringThin", spawnPos_, 2);
	pm->Emit("bossEntrance_streak", spawnPos_, 6);
}

void BossEntranceSequence::EmitBurst_() {

	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) return;

	pm->Emit("bossEntrance_ringShock", spawnPos_, 8);
	pm->Emit("bossEntrance_spark", spawnPos_, 60);
	pm->Emit("bossEntrance_streak", spawnPos_, 60);
}

void BossEntranceSequence::EmitPushWave_() {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) {
		return;
	}

	pm->Emit("bossEntrance_smoke", spawnPos_, 8);
	pm->Emit("bossEntrance_streak", spawnPos_, 5);
	pm->Emit("bossEntrance_spark", spawnPos_, 8);
}

void BossEntranceSequence::EmitCover_() {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) {
		return;
	}
	// かなり多めに出して、覆いを作る
	pm->Emit("bossEntrance_smoke", spawnPos_, 100);
	pm->Emit("bossEntrance_streak", spawnPos_, 60);
	pm->Emit("bossEntrance_ringThin", spawnPos_, 3);
}

void BossEntranceSequence::Update(float dt, BossManager* bossManager) {
	if (!isActive_) {
		return;
	}

	phaseTimer_ += dt;

	switch (phase_) {
	case Phase::Idle:
		break;

	case Phase::Wait:
		currentSkyColor_ = kBaseSkyColor_;

		if (phaseTimer_ >= kWaitSec_) {
			phase_ = Phase::SkyFadeIn;
			phaseTimer_ = 0.0f;
		}
		break;

	case Phase::SkyFadeIn:
	{
		const float t = std::clamp(phaseTimer_ / kSkyFadeInSec_, 0.0f, 1.0f);
		currentSkyColor_ = LerpColor_(kBaseSkyColor_, kRedSkyColor_, t);

		if (phaseTimer_ >= kSkyFadeInSec_) {
			phase_ = Phase::Gather;
			phaseTimer_ = 0.0f;
			convergeEmitTimer_ = 0.0f;
			glowEmitTimer_ = 0.0f;
			currentSkyColor_ = kRedSkyColor_;
		}
		break;
	}

	case Phase::Gather:
		currentSkyColor_ = kRedSkyColor_;

		convergeEmitTimer_ += dt;
		glowEmitTimer_ += dt;

		while (convergeEmitTimer_ >= kGatherEmitInterval_) {
			convergeEmitTimer_ -= kGatherEmitInterval_;
			EmitGather_();
		}

		while (glowEmitTimer_ >= kRingEmitInterval_) {
			glowEmitTimer_ -= kRingEmitInterval_;

			auto* pm = TKM::ParticleManager::GetInstance();
			if (pm) {
				pm->Emit("bossEntrance_ringThin", spawnPos_, 1);
			}
		}

		if (phaseTimer_ >= kGatherSec_) {
			phase_ = Phase::Cover;
			phaseTimer_ = 0.0f;
		}
		break;
	case Phase::Cover:
		currentSkyColor_ = kRedSkyColor_;

		coverEmitTimer_ += dt;
		while (coverEmitTimer_ >= kCoverEmitInterval_) {
			coverEmitTimer_ -= kCoverEmitInterval_;

			EmitCover_();
			++coverEmitCount_;

			// 覆いが少し重なったら、演出用のボスを先に生成する
			if (!bossSpawned_ && coverEmitCount_ >= kCoverSpawnEmitCount_) {
				if (bossManager) {
					bossManager->SpawnForEntrance();

					if (BossEnemy* boss = bossManager->GetBoss()) {
						burstStartPos_ = spawnPos_ + Vector3{ 0.0f, 2.2f, -10.0f };
						burstEndPos_ = spawnPos_;
						burstStartScale_ = { 1.6f, 1.6f, 1.6f };
						burstEndScale_ = { 5.0f, 5.0f, 5.0f };

						boss->SetPosition(burstStartPos_);
						boss->SetScale(burstStartScale_);
						boss->SyncTransform();
					}
				}
				bossSpawned_ = true;
			}
		}

		if (phaseTimer_ >= kCoverSec_) {
			phase_ = Phase::Burst;
			phaseTimer_ = 0.0f;
			burstFxEmitted_ = false;
		}
		break;
	case Phase::Burst:
		currentSkyColor_ = kRedSkyColor_;

		if (!burstFxEmitted_) {
			EmitBurst_();
			burstFxEmitted_ = true;
		}

		if (bossManager) {
			if (BossEnemy* boss = bossManager->GetBoss()) {
				float t = std::clamp(phaseTimer_ / kBurstSec_, 0.0f, 1.0f);
				float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); // EaseOutCubic

				Vector3 pos{};
				pos.x = MyMath::Lerp(burstStartPos_.x, burstEndPos_.x, e);
				pos.y = MyMath::Lerp(burstStartPos_.y, burstEndPos_.y, e);
				pos.z = MyMath::Lerp(burstStartPos_.z, burstEndPos_.z, e);

				Vector3 scale{};
				scale.x = MyMath::Lerp(burstStartScale_.x, burstEndScale_.x, e);
				scale.y = MyMath::Lerp(burstStartScale_.y, burstEndScale_.y, e);
				scale.z = MyMath::Lerp(burstStartScale_.z, burstEndScale_.z, e);

				boss->SetPosition(pos);
				boss->SetScale(scale);
				boss->SyncTransform();
			}
		}

		if (phaseTimer_ >= kBurstSec_) {
			if (bossManager) {
				if (BossEnemy* boss = bossManager->GetBoss()) {
					boss->SetPosition(burstEndPos_);
					boss->SetScale(burstEndScale_);
					boss->SyncTransform();
				}
				bossManager->BeginBattle();
			}

			phase_ = Phase::Push;
			phaseTimer_ = 0.0f;
			holdEmitTimer_ = 0.0f;
		}
		break;
	case Phase::Push:
		currentSkyColor_ = kRedSkyColor_;

		holdEmitTimer_ += dt;
		while (holdEmitTimer_ >= kPushEmitInterval_) {
			holdEmitTimer_ -= kPushEmitInterval_;
			EmitPushWave_();
		}

		if (phaseTimer_ >= kPushSec_) {
			phase_ = Phase::Done;
			phaseTimer_ = 0.0f;
		}
		break;

	case Phase::Done:
		isActive_ = false;
		currentSkyColor_ = kRedSkyColor_;
		break;
	}
}