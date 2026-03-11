#include "IntroBossActor.h"
#include <algorithm>
#include <cstdlib>

namespace TKM {

	void IntroBossActor::Initialize(TKM::Object3dCommon* object3dCommon, DirectXCommon* dxCommon) {
		object3dCommon_ = object3dCommon;
		dxCommon_ = dxCommon;
		Reset();
	}

	void IntroBossActor::Reset() {
		boss_.reset();

		phaseElapsed_ = 0.0f;
		pos_ = { 0.0f, 6.0f, appearStartZ_ };
		basePos_ = pos_;

		escapeTargetX_ = 0.0f;
		escapeTargetTimer_ = 0.0f;

		preSpawnElapsed_ = 0.0f;
		preSpawnEmitAccum_ = 0.0f;
		spawnFxFinished_ = false;
		noticeMarkEmitted_ = false;
		escapeWarpBurstEmitted_ = false;
	}

	void IntroBossActor::BeginPreSpawn() {
		preSpawnElapsed_ = 0.0f;
		preSpawnEmitAccum_ = 0.0f;
		spawnFxFinished_ = false;
		escapeWarpBurstEmitted_ = false;
		pos_ = { 0.0f, 6.0f, appearStartZ_ };
		basePos_ = pos_;
	}

	void IntroBossActor::UpdatePreSpawn(float dt) {
		preSpawnElapsed_ += dt;
		preSpawnEmitAccum_ += dt;
	}

	bool IntroBossActor::IsPreSpawnFinished() const {
		return preSpawnElapsed_ >= 1.8f;
	}

	void IntroBossActor::Spawn(Camera* camera) {
		if (boss_ || !object3dCommon_ || !dxCommon_ || !camera) { return; }

		boss_ = std::make_unique<BossEnemy>();
		boss_->Initialize(object3dCommon_, dxCommon_);
		boss_->SetCamera(camera);
		boss_->SetPosition({ 0.0f, 6.0f, appearStartZ_ });
		boss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });
		boss_->SetLocked(true);
		boss_->SyncTransform();

		pos_ = { 0.0f, 6.0f, appearStartZ_ };
		basePos_ = pos_;
		phaseElapsed_ = 0.0f;
	}

	void IntroBossActor::BeginAppear() {
		phaseElapsed_ = 0.0f;
	}

	bool IntroBossActor::UpdateAppear(float dt) {
		if (!boss_) { return false; }

		phaseElapsed_ += dt;
		float t = std::clamp(phaseElapsed_ / appearSec_, 0.0f, 1.0f);

		float moveT = t;
		moveT = moveT * moveT * (3.0f - 2.0f * moveT);

		float z = MyMath::Lerp(appearStartZ_, appearEndZ_, moveT);
		float floatX = std::sinf(phaseElapsed_ * appearFloatFreqX_) * appearFloatAmpX_;

		float floatYMain = std::sinf(phaseElapsed_ * appearFloatFreqY_) * appearFloatAmpY_;
		float floatYSub =
			std::sinf(phaseElapsed_ * (appearFloatFreqY_ * 2.15f) + 0.8f) *
			(appearFloatAmpY_ * 0.38f);

		float floatY = floatYMain + floatYSub;
		float damp = MyMath::Lerp(1.0f, 0.45f, moveT);

		pos_.z = z;
		pos_.x = floatX * damp;

		float bodyDrift = std::sinf(phaseElapsed_ * 0.95f + 1.2f) * 0.9f;
		pos_.y = 6.0f + bodyDrift + floatY * damp;

		float rotZ =
			std::sinf(phaseElapsed_ * 2.2f) * appearTiltZ_ * damp +
			std::sinf(phaseElapsed_ * 4.6f + 0.5f) * (appearTiltZ_ * 0.35f) * damp;

		boss_->SetPosition(pos_);
		boss_->SetRotate({ 0.0f, 3.14159265f, rotZ });
		boss_->SetIntroPanic(false, 0.0f);
		boss_->Update(dt);

		if (t >= 1.0f) {
			phaseElapsed_ = 0.0f;
			basePos_ = pos_;
			return true;
		}
		return false;
	}

	void IntroBossActor::BeginPause() {
		phaseElapsed_ = 0.0f;
	}

	bool IntroBossActor::UpdatePause(float dt) {
		if (!boss_) { return false; }

		phaseElapsed_ += dt;
		float t = std::clamp(phaseElapsed_ / pauseSec_, 0.0f, 1.0f);

		pos_ = basePos_;
		boss_->SetPosition(pos_);
		boss_->SetRotate({ 0.0f, 3.14159265f, 0.0f });

		float idleY = std::sinf(phaseElapsed_ * 5.0f) * 0.10f;
		boss_->SetPosition({ pos_.x, pos_.y + idleY, pos_.z });
		boss_->SetIntroPanic(false, 0.0f);
		boss_->Update(dt);

		if (t >= 1.0f) {
			phaseElapsed_ = 0.0f;
			basePos_ = { pos_.x, pos_.y + idleY, pos_.z };
			return true;
		}
		return false;
	}

	void IntroBossActor::BeginNoticeHop() {
		phaseElapsed_ = 0.0f;
		noticeMarkEmitted_ = false;
	}

	bool IntroBossActor::UpdateNoticeHop(float dt) {
		if (!boss_) { return false; }

		phaseElapsed_ += dt;
		float t = std::clamp(phaseElapsed_ / noticeHopSec_, 0.0f, 1.0f);

		float hopT = 0.0f;
		if (t >= 0.20f) {
			hopT = (t - 0.20f) / 0.80f;
			if (hopT > 1.0f) { hopT = 1.0f; }
		}

		float hop = std::sinf(hopT * 3.14159265f);
		float hopY = hop * noticeHopY_;
		float surpriseX = std::sinf(t * 3.14159265f) * 0.35f;

		Vector3 pos = basePos_;
		pos.x += surpriseX;
		pos.y += hopY;

		float rotZ = std::sinf(t * 3.14159265f) * 0.12f;

		pos_ = pos;
		boss_->SetPosition(pos);
		boss_->SetRotate({ 0.0f, 3.14159265f, rotZ });

		if (t < 0.20f) {
			boss_->SetIntroPanic(false, 0.0f);
		} else {
			boss_->SetIntroPanic(true, 0.85f);
		}

		boss_->Update(dt);

		if (t >= 1.0f) {
			boss_->SetIntroPanic(true, 1.0f);
			phaseElapsed_ = 0.0f;
			basePos_ = pos_;
			noticeMarkEmitted_ = false;
			return true;
		}
		return false;
	}

	void IntroBossActor::BeginPanic() {
		phaseElapsed_ = 0.0f;
	}

	bool IntroBossActor::UpdatePanic(float dt) {
		if (!boss_) { return false; }

		phaseElapsed_ += dt;
		float t = std::clamp(phaseElapsed_ / panicSec_, 0.0f, 1.0f);

		float shakeX = std::sinf(phaseElapsed_ * 12.0f) * panicAmpX_ * (0.30f + t * 0.70f);
		float shakeY = std::fabs(std::sinf(phaseElapsed_ * 15.0f)) * panicAmpY_;
		float wobbleRotZ = std::sinf(phaseElapsed_ * 13.0f) * 0.14f;

		pos_.x = basePos_.x + shakeX;
		pos_.y = basePos_.y + shakeY;
		pos_.z = basePos_.z;

		boss_->SetPosition(pos_);
		boss_->SetRotate({ 0.0f, 3.14159265f, wobbleRotZ });
		boss_->SetIntroPanic(true, 0.55f + t * 0.45f);
		boss_->Update(dt);

		if (t >= 1.0f) {
			phaseElapsed_ = 0.0f;
			escapeTargetX_ = pos_.x;
			escapeTargetTimer_ = 0.0f;
			return true;
		}
		return false;
	}

	void IntroBossActor::BeginEscape() {
		phaseElapsed_ = 0.0f;
		escapeTargetTimer_ = 0.0f;
	}

	bool IntroBossActor::UpdateEscape(float dt) {
		if (!boss_) { return false; }

		phaseElapsed_ += dt;
		escapeTargetTimer_ += dt;
		float t = std::clamp(phaseElapsed_ / escapeSec_, 0.0f, 1.0f);

		if (escapeTargetTimer_ >= escapeTargetInterval_) {
			escapeTargetTimer_ = 0.0f;
			float sign = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
			float ampGrow = MyMath::Lerp(0.65f, 1.25f, t);
			float mag = 2.0f + (static_cast<float>(std::rand()) / RAND_MAX) * escapeAmpX_ * ampGrow;
			escapeTargetX_ = sign * mag;
		}

		float follow = 16.0f * dt;
		pos_.x = MyMath::Lerp(pos_.x, escapeTargetX_, follow);
		pos_.z += escapeSpeedZ_ * dt * (1.0f + t * 0.30f);
		pos_.y = 6.0f + std::fabs(std::sinf(phaseElapsed_ * 14.0f)) * escapeHopY_;

		float leanZ = std::sinf(phaseElapsed_ * 16.0f) * 0.22f;

		boss_->SetPosition(pos_);
		boss_->SetRotate({ 0.0f, 3.14159265f + std::sinf(phaseElapsed_ * 8.0f) * 0.10f, leanZ });
		boss_->SetIntroPanic(true, 1.0f);
		boss_->Update(dt);

		if (pos_.z >= escapeEndZ_ || t >= 1.0f) {
			boss_->SetIntroPanic(false, 0.0f);
			return true;
		}
		return false;
	}

	void IntroBossActor::Draw(DirectXCommon* dxCommon) const {
		if (boss_) {
			boss_->Draw(dxCommon);
		}
	}

	float IntroBossActor::GetAppearRatio() const {
		return std::clamp(phaseElapsed_ / appearSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetPauseRatio() const {
		return std::clamp(phaseElapsed_ / pauseSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetNoticeHopRatio() const {
		return std::clamp(phaseElapsed_ / noticeHopSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetPanicRatio() const {
		return std::clamp(phaseElapsed_ / panicSec_, 0.0f, 1.0f);
	}

	float IntroBossActor::GetEscapeRatio() const {
		return std::clamp(phaseElapsed_ / escapeSec_, 0.0f, 1.0f);
	}

}