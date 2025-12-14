#pragma once
#include <random>
#include <algorithm>
#include "Enemy.h"
#include "MyMath.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

class BossController {
public:
	enum class State { // ボスの状態
		Enter,
		Orbit,
		DashWindup,
		DashRun,
		Recover,
	};

	enum class DashType { // ダッシュの種類
		Cross,
		Hook,
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
	/// <summary>
	/// ImGuiデバッグ表示
	/// </summary>
	/// <param name="boss"></param>
	void ImGuiDebug(Enemy& boss);
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

	DashType dashType_ = DashType::Cross; // ダッシュの種類

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

	float dashWindup_ = 2.0f; // 予備動作時間
	float recoverDuration_ = 1.0f;

	float dashSpeed_ = 28.0f;

	// 予測の外し量（どれくらいズラすか）
	float aimJitterX_ = 2.0f;   // 左右ズレ（ワールド座標）
	float aimJitterY_ = 0.0f;   // 基本0（水平勝負なら）
	float aimJitterZ_ = 1.0f;   // 奥行ズレ

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

	// 予測（どれくらい先を狙うか）
	float predictLeadTime_ = 0.35f;
	// 前フレームプレイヤー位置
	Vector3 prevPlayerPos_{};
	bool hasPrevPlayerPos_ = false;

	Vector3 playerPos = { 0.0f, 0.0f, 0.0f }; // プレイヤー位置キャッシュ
	Vector3 playerVel = { 0.0f, 0.0f, 0.0f }; // プレイヤー速度キャッシュ

	float recoverAngle_ = 0.0f; // 回復時のOrbit角度スタート位置

	// --- Recover中の「殴れたら凶悪化」判定 ---
	int  recoverStartHP_ = 0;
	int  recoverDamageThreshold_ = 6;   // とりあえず6（分かりやすく）
	bool nextDashFixed_ = false;        // 次ダッシュを固定するか
	DashType nextDashType_ = DashType::Cross;

	// 次ダッシュだけ速度補正（凶悪化の体感用）
	float nextDashSpeedMul_ = 1.0f;
	float dashSpeedNow_ = 28.0f;        // 実際にDashRunで使う速度

	// --- 怒りモード（有無のみ） ---
	bool rageActive_ = false; // 怒っているか
	// --- Rage Gauge ---
	float rageGauge_ = 0.0f;          // 0..1
	float rageGainPerHp_ = 0.08f;     // HP1減ったら+0.08（12〜13ダメで満タン）
	float rageDecayPerSec_ = 0.25f;   // 何も無いと毎秒-0.25（4秒で空）
	float rageOnThreshold_ = 1.0f;    // 満タンで怒りON
	float rageOffThreshold_ = 0.20f;  // ここまで落ちたら怒りOFF
	int   lastHpForRage_ = -1;        // 前回HP（ダメージ検出用）
	float noDamageTime_ = 0.0f;   // 最後に被ダメしてからの経過
	float rageDecayDelay_ = 2.0f; // 秒間ノーダメなら減衰開始

	// --- Windup Stop & Shake ---
	Vector3 windupBasePos_{ 0.0f, 0.0f, 0.0f }; // 予備動作開始位置（固定）
	float windupFxTimer_ = 0.0f; // エフェクト用タイマー
	float windupShakeAmp_ = 0.3f; // 揺れ振幅
	float windupShakeFreq1_ = 55.0f; // 揺れ周波数1
	float windupShakeFreq2_ = 83.0f; // 揺れ周波数2
};