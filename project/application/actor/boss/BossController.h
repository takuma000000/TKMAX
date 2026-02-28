#pragma once
#include <random>
#include <algorithm>
#include "Enemy.h"
#include "MyMath.h"
#include "AuraVolumeRenderer.h"
#include "StateMachine.h"

// =============================================================
// BossControllerクラス
// ボス敵の行動制御を行うクラス。
// =============================================================
class BossController : public TKM::IStateContext {
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

	/// <summary>
	/// ミサイル発射リクエストを取得して消費します。
	/// </summary>
	/// <param name="outPos">ミサイルの発射位置（ワールド座標）</param>
	/// <param name="outTarget">ミサイルのターゲット位置（ワールド座標）</param>
	/// <param name="outSpeed">ミサイルの移動速度</param>
	/// <param name="outCurveHeight">曲線移動時の高さオフセット</param>
	/// <param name="outDamage">ミサイルのダメージ量</param>
	/// <param name="outLifeFrame">ミサイルの生存フレーム数</param>
	/// <returns>発射リクエストが存在した場合 true、それ以外は false</returns>
	bool ConsumeMissileFireRequest(
		Vector3& outPos,
		Vector3& outTarget,
		float& outSpeed,
		float& outCurveHeight,
		int& outDamage,
		int& outLifeFrame
	);
	/// <summary>
	/// スラッシュ攻撃発射リクエストを取得して消費します。
	/// </summary>
	/// <param name="outPos">スラッシュの発射位置（ワールド座標）</param>
	/// <param name="outTarget">スラッシュのターゲット位置（ワールド座標）</param>
	/// <param name="outSpeed">スラッシュの移動速度</param>
	/// <param name="outDamage">スラッシュのダメージ量</param>
	/// <param name="outLifeFrame">スラッシュの生存フレーム数</param>
	/// <returns>発射リクエストが存在した場合 true、それ以外は false</returns>
	bool ConsumeSlashFireRequest(
		Vector3& outPos,
		Vector3& outTarget,
		float& outSpeed,
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
	/// <summary>
	/// いずれかの攻撃をチャージ中か？（触手演出用）
	/// </summary>
	bool IsAnyCharging() const;

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
	/// <summary>
	/// チャージの進行度(0..1)（触手揺れ強度用）
	/// </summary>
	float GetCharge01() const {
		float v = 0.0f;
		if (missileCharging_ && missileChargeTime_ > 0.0001f) {
			float t = 1.0f - (missileChargeTimer_ / missileChargeTime_);
			v = std::max(v, std::clamp(t, 0.0f, 1.0f));
		}
		if (slashCharging_ && slashChargeTime_ > 0.0001f) {
			float t = 1.0f - (slashChargeTimer_ / slashChargeTime_);
			v = std::max(v, std::clamp(t, 0.0f, 1.0f));
		}
		// レーザー予告は強めに
		if (laserTelegraph_ || state_ == State::LaserWindup) {
			v = std::max(v, 1.0f);
		}
		return v;
	}
	// =========================================
private:
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

	//==============================
	// State / Time
	//==============================
	State state_ = State::Enter; // 現在状態
	float timer_ = 0.0f; // 状態遷移タイマー
	//==============================
	// Arena
	//==============================
	Vector3 arenaMin_{ -18.0f, 3.0f, 35.0f }; // アリーナ範囲最小座標
	Vector3 arenaMax_{ 18.0f, 12.0f, 70.0f }; // アリーナ範囲最大座標
	//==============================
	// Random
	//==============================
	std::mt19937 rng_; // 乱数生成器
	//==============================
	// Player cache / Prediction
	//==============================
	// 予測（どれくらい先を狙うか）
	float predictLeadTime_ = 0.35f;
	// 前フレームプレイヤー位置
	Vector3 prevPlayerPos_{}; // プレイヤー位置前フレームキャッシュ
	bool hasPrevPlayerPos_ = false; // 前フレーム位置キャッシュ有無
	Vector3 playerPos_ = { 0.0f, 0.0f, 0.0f }; // プレイヤー位置キャッシュ
	Vector3 playerVel_ = { 0.0f, 0.0f, 0.0f }; // プレイヤー速度キャッシュ
	//==============================
	// Orbit
	//==============================
	float orbitZ_ = 55.0f; // 旋回Z座標
	float orbitY_ = 8.0f; // 旋回Y座標
	float orbitRadiusX_ = 10.0f; // 旋回半径X
	float orbitRadiusY_ = 2.5f; // 旋回半径Y
	float orbitAngularSpeed_ = 1.3f; // 旋回角速度（ラジアン/秒）
	float orbitPlayerInfluence_ = 0.35f; // プレイヤー位置影響度合い（0〜1）
	float orbitFollow_ = 0.16f; // 追従速度（大きいほど速い、0〜1）
	float orbitDuration_ = 3.2f; // 旋回継続時間
	float recoverDuration_ = 1.0f; // 回復時間
	float recoverAngle_ = 0.0f; // 回復時のOrbit角度スタート位置
	//==============================
	// Rage
	//==============================
	// --- 怒りモード（有無のみ） ---
	bool rageActive_ = false; // 怒っているか
	// --- Rage Gauge ---
	float rageGauge_ = 0.0f;          // 0..1
	float rageGainPerHp_ = 0.08f;     // HP1減ったら+0.08（12〜13ダメで満タン）
	float rageDecayPerSec_ = 0.25f;   // 何も無いと毎秒-0.25（4秒で空）
	float rageOnThreshold_ = 1.0f;    // 満タンで怒りON
	float rageOffThreshold_ = 0.20f;  // ここまで落ちたら怒りOFF
	int   lastHpForRage_ = -1;        // 前回HP（ダメージ検出用）
	float noDamageTime_ = 0.0f;       // 最後に被ダメしてからの経過
	float rageDecayDelay_ = 2.0f;     // 秒間ノーダメなら減衰開始
	//==============================
	// Laser（怒り中のみ）
	//==============================
	float laserCooldown_ = 5.0f;      // 連発防止
	float laserCooldownT_ = 0.0f; // クールタイム残り
	float laserChance_ = 0.40f;       // Orbit終了時にレーザーへ分岐する確率（怒り中）
	float laserWindup_ = 0.70f;       // 予告
	float laserFire_ = 1.10f;         // 発射
	float laserRecover_ = 0.55f;      // 復帰
	float laserRadius_ = 2.2f;        // 当たり判定の太さ
	float laserMuzzleYOffset_ = 10.0f; // 発射位置Yオフセット（ボス中心＋）
	float laserTrackStrength_ = 0.15f; // 発射中の軽い追尾（0で固定）
	bool  laserActive_ = false;        // 予告 or 発射
	bool  laserTelegraph_ = false;     // 予告中
	Vector3 laserStartWS_{ 0.0f,0.0f,0.0f }; // レーザー開始位置（ワールド座標）
	Vector3 laserEndWS_{ 0.0f,0.0f,0.0f }; // レーザー終了位置（ワールド座標）
	Vector3 laserBasePos_{ 0.0f,0.0f,0.0f }; // レーザー中の固定基準
	Vector3 laserAimFixed_{ 0.0f,0.0f,0.0f }; // 予告開始時の狙い（固定）
	// --- Windup Stop & Shake ---
	Vector3 windupBasePos_{ 0.0f, 0.0f, 0.0f }; // 予備動作開始位置（固定）
	float windupFxTimer_ = 0.0f; // エフェクト用タイマー
	float windupShakeAmp_ = 0.3f; // 揺れ振幅
	float windupShakeFreq1_ = 55.0f; // 揺れ周波数1
	float windupShakeFreq2_ = 83.0f; // 揺れ周波数2
	//==============================
	// Aura
	//==============================
	// --- Aura（段階1：情報だけ。描画は次の段階） ---
	bool   auraActive_ = false; // オーラ有効化フラグ
	float  auraT_ = 0.0f; // 予備動作に入ってからの経過秒
	Vector3 auraPos_{ 0.0f, 0.0f, 0.0f }; // 基準位置
	Vector3 auraColor_{ 0.2f, 0.85f, 1.0f }; // 初期は青寄り（あとでImGuiで変える）
	float   auraIntensity_ = 1.0f; // 強さ
	float   auraScaleMul_ = 1.6f;            // ボスサイズに対する広がり倍率（仮）
	bool    auraUseRing_ = true;             // 足元リングのON/OFF
	// auraVolume_
	TKM::AuraVolumeRenderer* auraVolume_ = nullptr; // オーラボリュームレンダラー
	//==============================
	// Missile（通常時攻撃その1）
	//==============================
	bool missileFireReq_ = false; // 発射要求フラグ
	Vector3 missilePos_{ 0.0f, 0.0f, 0.0f }; // 発射位置
	Vector3 missileDir_{ 0.0f, 0.0f, 1.0f }; // 発射方向
	Vector3 missileTarget_{ 0.0f,0.0f,0.0f }; // 発射時点のplayer座標（到達点）
	float missileSpeed_ = 70.0f; // 速度
	int missileDamage_ = 1; // ダメージ
	int missileLifeFrame_ = 180; // 寿命フレーム
	float missileMuzzleYOffset_ = 1.0f; // 発射位置Yオフセット（ボス中心＋）
	float missileCurveHeight_ = 2.5f;         // 曲線の山なり高さ
	// --- Missile burst (3連射) ---
	int burstLeft_ = 0; // 残り連射数
	float burstInterval_ = 0.5f; // 何秒おきに撃つか（0.08〜0.18あたり好み）
	float burstTimer_ = 0.0f; // 連射タイマー
	Vector3 burstTargetSnap_{}; // 発射時点のplayer座標を固定
	bool burstTargetValid_ = false; // 固定座標が有効かどうか
	bool burstCharged_ = false; // このバーストはチャージ完了済み？
	// --- Missile charge（予備動作）---
	bool missileCharging_ = false;     // 溜め中か
	float missileChargeTime_ = 1.0f;  // 溜め時間（秒）
	float missileChargeTimer_ = 0.0f;  // 溜め残り
	int missileChargeFrame_ = 0;       // 間引き用
	//==============================
	// SlashWave（通常時攻撃その2）
	//==============================
	bool slashFireReq_ = false; // 発射要求フラグ
	Vector3 slashPos_{ 0.0f,0.0f,0.0f }; // 発射位置
	Vector3 slashTarget_{ 0.0f,0.0f,0.0f }; // 発射時点のplayer座標（到達点）
	float slashSpeed_ = 95.0f;     // ミサイルより速め推奨
	int slashDamage_ = 2; // ダメージ
	int slashLifeFrame_ = 90; // 寿命フレーム
	// 予備動作（スラッシュ用：ミサイルとは別）
	bool slashCharging_ = false; // 溜め中か
	float slashChargeTime_ = 0.55f; // 溜め時間（秒）
	float slashChargeTimer_ = 0.0f; // 溜め残り
	int slashChargeFrame_ = 0; // 間引き用
	// クールタイム（連発防止）
	float slashCooldown_ = 2.0f; // 何秒間隔で撃てるか
	float slashCooldownT_ = 0.0f; // クールタイム残り
	// スラッシュ：発射時点のターゲット固定
	Vector3 slashTargetSnap_{ 0.0f, 0.0f, 0.0f }; // 発射時点のplayer座標を固定
	bool slashTargetValid_ = false; // 固定座標が有効かどうか
	//==============================
	// ステートパターン
	//==============================
	friend class BossEnterState;
	friend class BossOrbitState;
	friend class BossRecoverState;
	friend class BossLaserWindupState;
	friend class BossLaserFireState;
	friend class BossLaserRecoverState;
	// StateMachine
	TKM::StateMachine sm_; // 状態遷移マシン
	Enemy* boss_ = nullptr; // Update中だけ有効
	Vector3 posWork_{}; // State側で動かす座標（最後にbossへ反映）
};