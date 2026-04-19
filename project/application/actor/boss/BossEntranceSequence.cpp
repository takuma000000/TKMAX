#include "BossEntranceSequence.h"
#include "AudioManager.h"
#include "BossEntranceStates.h"
#include "ParticleManager.h"
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
	burstEndScale_ = { 1.0f, 1.0f, 1.0f, };
	bossManager_ = nullptr;
}

void BossEntranceSequence::Start(const Vector3& spawnPos) {
	if (isActive_) {
		return;
	}

	Reset_();

	isActive_ = true;
	spawnPos_ = spawnPos;
	currentSkyColor_ = kBaseSkyColor_;

	sm_.Initialize(this);
	sm_.Change(std::make_unique<BossEntranceWaitState>());

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

	bossManager_ = bossManager;
	sm_.Update(dt);
}