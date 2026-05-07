#include "BossEntranceSequence.h"
#include "AudioManager.h"
#include "BossEntranceStates.h"
#include "ParticleManager.h"
#include <algorithm>

//=============================================================
// 色補間
//=============================================================
Vector4 BossEntranceSequence::LerpColor_(const Vector4& a, const Vector4& b, float t) {
	// 補間率を 0.0 ～ 1.0 に収める
	t = std::clamp(t, 0.0f, 1.0f);

	Vector4 out{};
	out.x = MyMath::Lerp(a.x, b.x, t);
	out.y = MyMath::Lerp(a.y, b.y, t);
	out.z = MyMath::Lerp(a.z, b.z, t);
	out.w = MyMath::Lerp(a.w, b.w, t);
	return out;
}

//=============================================================
// 内部状態リセット
//=============================================================
void BossEntranceSequence::Reset_() {
	isActive_ = false;
	bossSpawned_ = false;

	//=========================================================
	// タイマー類初期化
	//=========================================================
	phaseTimer_ = 0.0f;
	convergeEmitTimer_ = 0.0f;
	glowEmitTimer_ = 0.0f;
	holdEmitTimer_ = 0.0f;
	coverEmitTimer_ = 0.0f;

	//=========================================================
	// カウント・状態初期化
	//=========================================================
	coverEmitCount_ = 0;
	burstFxEmitted_ = false;

	//=========================================================
	// 見た目・座標系初期化
	//=========================================================
	currentSkyColor_ = kBaseSkyColor_;
	spawnPos_ = { 0.0f, 0.0f, 0.0f };

	burstStartPos_ = { 0.0f, 0.0f, 0.0f };
	burstEndPos_ = { 0.0f, 0.0f, 0.0f };

	burstStartScale_ = { 1.0f, 1.0f, 1.0f };
	burstEndScale_ = { 1.0f, 1.0f, 1.0f, };

	//=========================================================
	// 外部参照初期化
	//=========================================================
	bossManager_ = nullptr;
}

//=============================================================
// 登場演出開始
//=============================================================
void BossEntranceSequence::Start(const Vector3& spawnPos) {
	// 既に開始中なら何もしない
	if (isActive_) {
		return;
	}

	//=========================================================
	// 演出状態初期化
	//=========================================================
	Reset_();

	isActive_ = true;
	spawnPos_ = spawnPos;
	currentSkyColor_ = kBaseSkyColor_;

	//=========================================================
	// ステート開始
	//=========================================================
	sm_.Initialize(this);
	sm_.Change(std::make_unique<BossEntranceWaitState>());

	//=========================================================
	// BGM開始
	//=========================================================
	TKM::AudioManager::GetInstance()->PlaySound("bossPhaseBGM", 0.1f, true);
}

//=============================================================
// 収束エフェクト発生
//=============================================================
void BossEntranceSequence::EmitGather_() {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) {
		return;
	}

	pm->Emit("bossEntrance_gather", spawnPos_, 18);
	pm->Emit("bossEntrance_ringThin", spawnPos_, 2);
	pm->Emit("bossEntrance_streak", spawnPos_, 6);
}

//=============================================================
// バーストエフェクト発生
//=============================================================
void BossEntranceSequence::EmitBurst_() {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) {
		return;
	}

	pm->Emit("bossEntrance_ringShock", spawnPos_, 8);
	pm->Emit("bossEntrance_spark", spawnPos_, 60);
	pm->Emit("bossEntrance_streak", spawnPos_, 60);
}

//=============================================================
// 押し出し波エフェクト発生
//=============================================================
void BossEntranceSequence::EmitPushWave_() {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) {
		return;
	}

	pm->Emit("bossEntrance_smoke", spawnPos_, 8);
	pm->Emit("bossEntrance_streak", spawnPos_, 5);
	pm->Emit("bossEntrance_spark", spawnPos_, 8);
}

//=============================================================
// 覆いエフェクト発生
//=============================================================
void BossEntranceSequence::EmitCover_() {
	auto* pm = TKM::ParticleManager::GetInstance();
	if (!pm) {
		return;
	}

	// かなり多めに出して、出現位置を覆うような見た目を作る
	pm->Emit("bossEntrance_smoke", spawnPos_, 100);
	pm->Emit("bossEntrance_streak", spawnPos_, 60);
	pm->Emit("bossEntrance_ringThin", spawnPos_, 3);
}

//=============================================================
// 更新
//=============================================================
void BossEntranceSequence::Update(float dt, BossManager* bossManager) {
	// 演出が動いていなければ何もしない
	if (!isActive_) {
		return;
	}

	//=========================================================
	// 外部参照更新
	//=========================================================
	bossManager_ = bossManager;

	//=========================================================
	// ステート更新
	//=========================================================
	sm_.Update(dt);
}