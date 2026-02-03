#pragma once
#include <random>
#include <algorithm>
#include "Enemy.h"
#include "MyMath.h"
#include "AuraVolumeRenderer.h"

// =============================================================
// BossControllerクラス
// ボス敵の行動制御を行うクラス。
// =============================================================
class BossController {
public:
	enum class State {
		Enter,
		Orbit,
		LaserWindup,
		LaserFire,
		LaserRecover,
		Recover,
	};

	/// <summary>
	/// アリーナ範囲の初期化を行います。
	/// </summary>
	/// <param name="arenaMin">アリーナ範囲の最小座標（ワールド座標）</param>
	/// <param name="arenaMax">アリーナ範囲の最大座標（ワールド座標）</param>
	void Initialize(const Vector3& arenaMin, const Vector3& arenaMax);
	/// <summary>
	/// リセット
	/// </summary>
	void Reset();
	/// <summary>
	/// 毎フレームの更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <param name="boss">更新対象となるボス敵</param>
	void Update(float dt, Enemy& boss);
	/// <summary>
	/// ImGui によるデバッグ情報を表示します。
	/// </summary>
	/// <param name="boss">デバッグ表示および調整対象となるボス敵</param>
	void ImGuiDebug(Enemy& boss);

	bool ConsumeMissileFireRequest(
		Vector3& outPos,
		Vector3& outTarget,
		float& outSpeed,
		float& outCurveHeight,
		int& outDamage,
		int& outLifeFrame
	);

	/// <summary>
	/// 怒りモードを設定する
	/// </summary>
	/// <returns></returns>
	bool IsAuraActive() const { return auraActive_; }
	/// <summary>
	/// レーザー予告中かどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsLaserWindup() const { return state_ == State::LaserWindup; }
	/// <summary>
	/// レーザー発射中かどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsLaserFiring() const { return state_ == State::LaserFire; }
	/// <summary>
	/// レーザーがアクティブかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsLaserActive() const { return laserActive_; }        // 予告 or 発射中
	/// <summary>
	/// レーザーが予告中かどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsLaserTelegraph() const { return laserTelegraph_; }  // 予告中

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
	/// <summary>
	/// レーザー開始位置（ワールド座標）を取得します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetLaserStartWS() const { return laserStartWS_; }
	/// <summary>
	/// レーザー終了位置（ワールド座標）を取得します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetLaserEndWS() const { return laserEndWS_; }
	/// <summary>
	/// レーザー半径を取得します。
	/// </summary>
	/// <returns></returns>
	float GetLaserRadius() const { return laserRadius_; }
	// =========================================
private:
	/// <summary>
	/// 侵入状態の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <param name="boss">更新対象となるボス敵</param>
	/// <param name="pos">ボス位置（参照で更新される、ワールド座標）</param>
	void UpdateEnter(float dt, Enemy& boss, Vector3& pos);
	/// <summary>
	/// 軌道回転状態の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <param name="boss">更新対象となるボス敵</param>
	/// <param name="pos">ボス位置（参照で更新される、ワールド座標）</param>
	/// <param name="playerPos">プレイヤー位置（ワールド座標）</param>
	void UpdateOrbit(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	/// <summary>
	/// 回復状態の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <param name="boss">更新対象となるボス敵</param>
	/// <param name="pos">ボス位置（参照で更新される、ワールド座標）</param>
	void UpdateRecover(float dt, Enemy& boss, Vector3& pos);
	/// <summary>
	/// レーザー溜め（予備動作）状態の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <param name="boss">更新対象となるボス敵</param>
	/// <param name="pos">ボス位置（参照で更新される、ワールド座標）</param>
	/// <param name="playerPos">プレイヤー位置（ワールド座標）</param>
	void UpdateLaserWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	/// <summary>
	/// レーザー発射状態の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <param name="boss">更新対象となるボス敵</param>
	/// <param name="pos">ボス位置（参照で更新される、ワールド座標）</param>
	/// <param name="playerPos">プレイヤー位置（ワールド座標）</param>
	void UpdateLaserFire(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos);
	/// <summary>
	/// レーザー後隙（回復）状態の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <param name="boss">更新対象となるボス敵</param>
	/// <param name="pos">ボス位置（参照で更新される、ワールド座標）</param>
	void UpdateLaserRecover(float dt, Enemy& boss, Vector3& pos);
	/// <summary>
	/// 状態を変更します。
	/// </summary>
	/// <param name="s">遷移先の状態</param>
	void ChangeState(State s);
	/// <summary>
	/// 位置をアリーナ範囲内にクランプします。
	/// </summary>
	/// <param name="p">クランプ対象の位置（参照で更新される、ワールド座標）</param>
	void ClampToArena(Vector3& p);
	/// <summary>
	/// 値を目標に向かって一定量だけ近づけます。
	/// </summary>
	/// <param name="v">現在値</param>
	/// <param name="target">目標値</param>
	/// <param name="delta">1回の呼び出しで近づける最大量</param>
	/// <returns>更新後の値</returns>
	static float Approach(float v, float target, float delta);
	/// <summary>
	/// 2点間を滑らかに補間します。
	/// </summary>
	/// <param name="from">開始位置</param>
	/// <param name="to">目標位置</param>
	/// <param name="factor">補間係数（大きいほど追従が速い）</param>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	/// <returns>補間後の位置</returns>
	static Vector3 SmoothDamp(const Vector3& from, const Vector3& to, float factor, float dt);

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

	float recoverDuration_ = 1.0f;

	std::mt19937 rng_;

	// 予測（どれくらい先を狙うか）
	float predictLeadTime_ = 0.35f;
	// 前フレームプレイヤー位置
	Vector3 prevPlayerPos_{};
	bool hasPrevPlayerPos_ = false;

	Vector3 playerPos_ = { 0.0f, 0.0f, 0.0f }; // プレイヤー位置キャッシュ
	Vector3 playerVel_ = { 0.0f, 0.0f, 0.0f }; // プレイヤー速度キャッシュ

	float recoverAngle_ = 0.0f; // 回復時のOrbit角度スタート位置

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

	// =============================================================
	// Missile（通常時攻撃）
	// =============================================================
	bool missileFireReq_ = false; // 発射要求フラグ
	Vector3 missilePos_{ 0.0f, 0.0f, 0.0f }; // 発射位置
	Vector3 missileDir_{ 0.0f, 0.0f, 1.0f }; // 発射方向
	float missileSpeed_ = 70.0f; // 速度
	int missileDamage_ = 1; // ダメージ
	int missileLifeFrame_ = 180; // 寿命フレーム
	float missileMuzzleYOffset_ = 1.0f; // 発射位置Yオフセット（ボス中心＋）

	Vector3 missileTarget_{ 0.0f,0.0f,0.0f }; // 発射時点のplayer座標（到達点）
	float missileCurveHeight_ = 2.5f;         // 曲線の山なり高さ

	// --- Missile burst (3連射) ---
	int burstLeft_ = 0; // 残り連射数
	float burstInterval_ = 0.5f; // 何秒おきに撃つか（0.08〜0.18あたり好み）
	float burstTimer_ = 0.0f; // 連射タイマー
	Vector3 burstTargetSnap_{}; // 発射時点のplayer座標を固定
	bool burstTargetValid_ = false; // 固定座標が有効かどうか
};