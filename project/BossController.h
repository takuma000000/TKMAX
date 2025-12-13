#pragma once
#include <random>
#include <algorithm>
#include "application/enemy/Enemy.h"
#include "MyMath.h"

class BossController {
public:
	enum class State {
		Enter,
		Orbit,
		DashWindup,
		DashRun,
		Recover,
	};

	void Initialize(const Vector3& arenaMin, const Vector3& arenaMax);
	void Reset();
	void Update(float dt, Enemy& boss);

private:
	// --- state updates ---
	void UpdateEnter(float dt, Enemy& boss, Vector3& pos);
	void UpdateOrbit(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	void UpdateDashWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	void UpdateDashRun(float dt, Enemy& boss, Vector3& pos);
	void UpdateRecover(float dt, Enemy& boss, Vector3& pos);

	// --- helpers ---
	void ChangeState(State s);
	void PrepareDash(const Vector3& currentPos, const Vector3& playerPos);
	void ClampToArena(Vector3& p);

	static float Approach(float v, float target, float delta);
	static Vector3 SmoothDamp(const Vector3& from, const Vector3& to, float factor, float dt);

private:
	State state_ = State::Enter;
	float timer_ = 0.0f;

	Vector3 arenaMin_{ -18.0f, 3.0f, 35.0f };
	Vector3 arenaMax_{ 18.0f, 12.0f, 70.0f };

	// Orbit
	float orbitZ_ = 55.0f;
	float orbitY_ = 8.0f;
	float orbitRadiusX_ = 10.0f;
	float orbitRadiusY_ = 2.5f;
	float orbitAngularSpeed_ = 1.3f;
	float orbitPlayerInfluence_ = 0.35f;
	float orbitFollow_ = 0.16f;
	float orbitDuration_ = 3.2f;

	// Dash
	Vector3 dashStartPos_{};
	Vector3 dashEndPos_{};
	Vector3 lastPlayerPos_{};

	float dashWindup_ = 0.6f;
	float recoverDuration_ = 1.0f;

	float dashSpeed_ = 28.0f;

	float dashStartX_ = 15.0f;
	float dashEndX_ = 15.0f;
	float dashStartZ_ = 62.0f;
	float dashEndZ_ = 42.0f;
	float dashStartYBias_ = 0.0f;
	float dashEndYBias_ = 0.0f;

	int dashRepeat_ = 2;
	int dashCount_ = 0;
	int lastDashDir_ = 1;

	std::mt19937 rng_;
};