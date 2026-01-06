#pragma once
#include <random>
#include <algorithm>
#include "Enemy.h"
#include "MyMath.h"
#include "AuraVolumeRenderer.h"

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
		// --- Rage専用攻撃 ---
		LaserWindup,
		LaserFire,
		LaserRecover,
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

	/// <summary>
	/// 怒りモードを設定する
	/// </summary>
	/// <returns></returns>
	bool IsAuraActive() const { return auraActive_; }
	/// <summary>
	/// ダッシュ予備動作中かどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsDashWindup() const { return state_ == State::DashWindup; }

	// =========================
	// Laser（怒り中攻撃）情報
	// =========================
	bool IsLaserWindup() const { return state_ == State::LaserWindup; }
	bool IsLaserFiring() const { return state_ == State::LaserFire; }
	bool IsLaserActive() const { return laserActive_; }        // 予告 or 発射中
	bool IsLaserTelegraph() const { return laserTelegraph_; }  // 予告中
	const Vector3& GetLaserStartWS() const { return laserStartWS_; }
	const Vector3& GetLaserEndWS() const { return laserEndWS_; }
	float GetLaserRadius() const { return laserRadius_; }

	// Getter===================================
	/// <summary>
	/// 予備動作に入ってからの経過秒を取得します。
	/// </summary>
	/// <returns></returns>
	float GetAuraT() const { return auraT_; }
	/// <summary>
	/// 基準位置を取得します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetAuraPos() const { return auraPos_; }
	/// <summary>
	/// オーラの色を取得します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetAuraColor() const { return auraColor_; }
	/// <summary>
	/// オーラの強度を取得します。
	/// </summary>
	/// <returns></returns>
	float GetAuraIntensity() const { return auraIntensity_; }
	/// <summary>
	/// オーラのスケール倍率を取得します。
	/// </summary>
	/// <returns></returns>
	float GetAuraScaleMul() const { return auraScaleMul_; }
	/// <summary>
	/// 足元リングの使用有無を取得します。
	/// </summary>
	/// <returns></returns>
	bool GetAuraUseRing() const { return auraUseRing_; }
	// =========================================
private:
	// --- constants ---
	// Dash offsets
	struct DashOffsets {
		float startZOff;
		float endZOff;
	};
	// ダッシュ開始・終了Zオフセット
	static constexpr DashOffsets kDashOffsets_[2] = {
		/* Cross */ {  0.0f,  0.0f },
		/* Hook  */ { 10.0f, -5.0f },
	};

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

	// --- Laser ---
	void UpdateLaserWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	void UpdateLaserFire(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	void UpdateLaserRecover(float dt, Enemy& boss, Vector3& pos);

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

	// =========================
	// Laser（怒り中のみ）
	// =========================
	float laserCooldown_ = 5.0f;      // 連発防止
	float laserCooldownT_ = 0.0f;
	float laserChance_ = 0.40f;       // Orbit終了時にレーザーへ分岐する確率（怒り中）
	float laserWindup_ = 0.70f;       // 予告
	float laserFire_ = 1.10f;         // 発射
	float laserRecover_ = 0.55f;      // 復帰
	float laserRadius_ = 2.2f;        // 当たり判定の太さ
	float laserMuzzleYOffset_ = 10.0f; // 発射位置Yオフセット（ボス中心＋）
	float laserTrackStrength_ = 0.15f; // 発射中の軽い追尾（0で固定）

	bool  laserActive_ = false;        // 予告 or 発射
	bool  laserTelegraph_ = false;     // 予告中
	Vector3 laserStartWS_{ 0.0f,0.0f,0.0f };
	Vector3 laserEndWS_{ 0.0f,0.0f,0.0f };
	Vector3 laserBasePos_{ 0.0f,0.0f,0.0f }; // レーザー中の固定基準
	Vector3 laserAimFixed_{ 0.0f,0.0f,0.0f }; // 予告開始時の狙い（固定）

	// --- Windup Stop & Shake ---
	Vector3 windupBasePos_{ 0.0f, 0.0f, 0.0f }; // 予備動作開始位置（固定）
	float windupFxTimer_ = 0.0f; // エフェクト用タイマー
	float windupShakeAmp_ = 0.3f; // 揺れ振幅
	float windupShakeFreq1_ = 55.0f; // 揺れ周波数1
	float windupShakeFreq2_ = 83.0f; // 揺れ周波数2

	// --- Aura（段階1：情報だけ。描画は次の段階） ---
	bool   auraActive_ = false;
	float  auraT_ = 0.0f;
	Vector3 auraPos_{ 0.0f, 0.0f, 0.0f };
	Vector3 auraColor_{ 0.2f, 0.85f, 1.0f }; // 初期は青寄り（あとでImGuiで変える）
	float   auraIntensity_ = 1.0f;
	float   auraScaleMul_ = 1.6f;            // ボスサイズに対する広がり倍率（仮）
	bool    auraUseRing_ = true;             // 足元リングのON/OFF

	// auraVolume_
	TKM::AuraVolumeRenderer* auraVolume_ = nullptr;
};