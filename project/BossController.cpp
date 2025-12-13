#include "BossController.h"
#include <cmath>

void BossController::Initialize(const Vector3& arenaMin, const Vector3& arenaMax) {
	arenaMin_ = arenaMin;
	arenaMax_ = arenaMax;
	state_ = State::Enter;
	timer_ = 0.0f;
	dashCount_ = 0;
	lastDashDir_ = 1;
	rng_.seed(std::random_device{}());
}

void BossController::Reset() {
	state_ = State::Enter;
	timer_ = 0.0f;
	dashCount_ = 0;
	lastDashDir_ = 1;
}

void BossController::Update(float dt, Enemy& boss) {
	timer_ += dt;

	Vector3 playerPos{ 0.0f, 0.0f, 0.0f };
	if (boss.GetPlayer()) {
		playerPos = boss.GetPlayer()();
	}

	Vector3 pos = boss.GetWorldPosition();

	switch (state_) {
	case State::Enter:      UpdateEnter(dt, boss, pos); break;
	case State::Orbit:      UpdateOrbit(dt, boss, pos, playerPos); break;
	case State::DashWindup: UpdateDashWindup(dt, boss, pos, playerPos); break;
	case State::DashRun:    UpdateDashRun(dt, boss, pos); break;
	case State::Recover:    UpdateRecover(dt, boss, pos); break;
	}

	ClampToArena(pos);
	boss.SetPosition(pos);
	boss.SyncTransform();
}

void BossController::UpdateEnter(float dt, Enemy& boss, Vector3& pos) {
	const float targetZ = orbitZ_;
	const float speed = 18.0f;

	pos.z = Approach(pos.z, targetZ, speed * dt);
	pos.x = Approach(pos.x, 0.0f, 10.0f * dt);
	pos.y = Approach(pos.y, orbitY_, 10.0f * dt);

	if (std::abs(pos.z - targetZ) < 0.05f) {
		ChangeState(State::Orbit);
	}
}

void BossController::UpdateOrbit(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	float t = timer_;

	float angle = t * orbitAngularSpeed_;
	float ox = std::cos(angle) * orbitRadiusX_;
	float oy = std::sin(angle * 0.9f) * orbitRadiusY_;

	Vector3 target;
	target.x = playerPos.x * orbitPlayerInfluence_ + ox;
	target.y = orbitY_ + playerPos.y * 0.2f + oy;
	target.z = orbitZ_;

	pos = SmoothDamp(pos, target, orbitFollow_, dt);

	if (timer_ >= orbitDuration_) {
		PrepareDash(pos, playerPos);
		ChangeState(State::DashWindup);
	}
}

void BossController::UpdateDashWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	pos = SmoothDamp(pos, dashStartPos_, 0.18f, dt);

	if (timer_ >= dashWindup_) {
		ChangeState(State::DashRun);
	}
}

void BossController::UpdateDashRun(float dt, Enemy& boss, Vector3& pos) {
	Vector3 to = dashEndPos_ - pos;
	float len = MyMath::Length(to);

	if (len < 0.01f) {
		dashCount_++;

		if (dashCount_ < dashRepeat_) {
			lastDashDir_ *= -1;
			PrepareDash(pos, lastPlayerPos_);
			ChangeState(State::DashWindup);
		} else {
			ChangeState(State::Recover);
		}
		return;
	}

	Vector3 dir = MyMath::Normalize(to);
	pos += dir * (dashSpeed_ * dt);
}

void BossController::UpdateRecover(float dt, Enemy& boss, Vector3& pos) {
	Vector3 target{ 0.0f, orbitY_, orbitZ_ };
	pos = SmoothDamp(pos, target, 0.14f, dt);

	if (timer_ >= recoverDuration_) {
		ChangeState(State::Orbit);
	}
}

void BossController::ChangeState(State s) {
	state_ = s;
	timer_ = 0.0f;

	// Dash系の回数は Dash開始時にリセット
	if (s == State::DashWindup) {
		if (dashCount_ == 0) {
			// 何もしない（PrepareDash側で開始する想定）
		}
	}
	if (s == State::Orbit) {
		dashCount_ = 0;
	}
}

void BossController::PrepareDash(const Vector3& currentPos, const Vector3& playerPos) {
	lastPlayerPos_ = playerPos;

	float side = (playerPos.x >= 0.0f) ? -1.0f : +1.0f;
	side *= static_cast<float>(lastDashDir_);

	dashStartPos_ = {
		side * dashStartX_,
		orbitY_ + dashStartYBias_,
		dashStartZ_
	};

	dashEndPos_ = {
		-side * dashEndX_,
		orbitY_ + dashEndYBias_,
		dashEndZ_
	};
}

void BossController::ClampToArena(Vector3& p) {
	p.x = std::clamp(p.x, arenaMin_.x, arenaMax_.x);
	p.y = std::clamp(p.y, arenaMin_.y, arenaMax_.y);
	p.z = std::clamp(p.z, arenaMin_.z, arenaMax_.z);
}

float BossController::Approach(float v, float target, float delta) {
	if (v < target) { return std::min(v + delta, target); }
	return std::max(v - delta, target);
}

Vector3 BossController::SmoothDamp(const Vector3& from, const Vector3& to, float factor, float dt) {
	// ざっくり指数補間（factor小さいほどヌルい）
	float k = 1.0f - std::pow(1.0f - factor, dt * 60.0f);
	return MyMath::Vector3Lerp(from, to, k);
}
