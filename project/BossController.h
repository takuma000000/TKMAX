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

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="arenaMin"></param>
	/// <param name="arenaMax"></param>
	void Initialize(const Vector3& arenaMin, const Vector3& arenaMax);
	/// <summary>
	/// リセット
	/// </summary>
	void Reset();
	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="boss"></param>
	void Update(float dt, Enemy& boss);

private:
	// --- state updates ---
	/// <summary>
	/// 侵入
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="boss"></param>
	/// <param name="pos"></param>
	void UpdateEnter(float dt, Enemy& boss, Vector3& pos);
	/// <summary>
	/// 軌道回転
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="boss"></param>
	/// <param name="pos"></param>
	/// <param name="playerPos"></param>
	void UpdateOrbit(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	/// <summary>
	/// ダッシュ予備動作
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="boss"></param>
	/// <param name="pos"></param>
	/// <param name="playerPos"></param>
	void UpdateDashWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	/// <summary>
	/// ダッシュ実行
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="boss"></param>
	/// <param name="pos"></param>
	void UpdateDashRun(float dt, Enemy& boss, Vector3& pos);
	/// <summary>
	/// 回復
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="boss"></param>
	/// <param name="pos"></param>
	void UpdateRecover(float dt, Enemy& boss, Vector3& pos);

	// --- helpers ---
	/// <summary>
	/// 状態変更
	/// </summary>
	/// <param name="s"></param>
	void ChangeState(State s);
	/// <summary>
	/// ダッシュ準備
	/// </summary>
	/// <param name="currentPos"></param>
	/// <param name="playerPos"></param>
	void PrepareDash(const Vector3& currentPos, const Vector3& playerPos);
	/// <summary>
	/// アリーナ内に位置をクランプする
	/// </summary>
	/// <param name="p"></param>
	void ClampToArena(Vector3& p);

	/// <summary>
	/// 値を目標に向かって近づける
	/// </summary>
	/// <param name="v"></param>
	/// <param name="target"></param>
	/// <param name="delta"></param>
	/// <returns></returns>
	static float Approach(float v, float target, float delta);
	/// <summary>
	/// スムーズダンプ
	/// </summary>
	/// <param name="from"></param>
	/// <param name="to"></param>
	/// <param name="factor"></param>
	/// <param name="dt"></param>
	/// <returns></returns>
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