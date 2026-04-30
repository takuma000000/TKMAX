#pragma once
#include <random>
#include <algorithm>
#include <array>
#include "Enemy.h"
#include "MyMath.h"
#include "AuraVolumeRenderer.h"
#include "StateMachine.h"
#include "BossConfig.h"

// =============================================================
// BossControllerクラス
// ボス敵の行動制御を行うクラス。
// =============================================================
class BossController : public TKM::IStateContext {
public:
	enum class State {
		Enter, // ボス登場
		Orbit, // プレイヤーを中心に回りながら攻撃
		Recover, // ダメージ受けて回復
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
	/// ミサイル攻撃発射リクエストを取得して消費します。
	/// </summary>
	/// <param name="outPos">ミサイルの発射位置（ワールド座標）</param>
	/// <param name="outTarget">ミサイルのターゲット位置（ワールド座標）</param>
	/// <param name="outSpeed">ミサイルの移動速度</param>
	/// <param name="outControlOffset">ミサイルの制御点オフセット（ワールド座標、ターゲットに対する相対位置）</param>
	/// <param name="outDamage">ミサイルのダメージ量</param>
	/// <param name="outLifeFrame">ミサイルの生存フレーム数</param>
	/// <returns>発射リクエストが存在した場合 true、それ以外は false</returns>
	bool ConsumeMissileFireRequest(
		Vector3& outPos,
		Vector3& outTarget,
		float& outSpeed,
		Vector3& outControlOffset,
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
	/// チャージの進行度(0..1)（触手揺れ強度用）
	/// </summary>
	float GetCharge01() const;
	// =========================================
	// Setter===================================
	/// <summary>
	/// BossControllerConfig を設定します。
	/// </summary>
	/// <param name="config">設定へのポインタ</param>
	void SetConfig(const BossControllerConfig* config);
	// =========================================
private:
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
	// 前フレームプレイヤー位置
	Vector3 prevPlayerPos_{}; // プレイヤー位置前フレームキャッシュ
	bool hasPrevPlayerPos_ = false; // 前フレーム位置キャッシュ有無
	Vector3 playerPos_ = { 0.0f, 0.0f, 0.0f }; // プレイヤー位置キャッシュ
	Vector3 playerVel_ = { 0.0f, 0.0f, 0.0f }; // プレイヤー速度キャッシュ
	//==============================
	// Orbit
	//==============================
	float recoverAngle_ = 0.0f; // 回復時のOrbit角度スタート位置
	//==============================
	// Rage
	//==============================
	// --- 怒りモード（有無のみ） ---
	bool rageActive_ = false; // 怒っているか
	// --- Rage Gauge ---
	float rageGauge_ = 0.0f;          // 0..1
	int   lastHpForRage_ = -1;        // 前回HP（ダメージ検出用）
	float noDamageTime_ = 0.0f;       // 最後に被ダメしてからの経過
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
	// 同時発射数分のリクエストをキューイングして、Update側で消費していく方式。これも予備動作やクールタイムとは独立。
	struct MissileFireRequest {
		Vector3 pos_{ 0.0f, 0.0f, 0.0f };
		Vector3 target_{ 0.0f, 0.0f, 0.0f };
		Vector3 controlOffset_{ 0.0f, 0.0f, 0.0f };
	};
	static constexpr int kMissileSimultaneousCount_ = 6; // 同時発射数（多すぎると見た目がうるさくなるので注意）
	std::array<MissileFireRequest, kMissileSimultaneousCount_> missileRequests_{}; // 発射リクエスト配列
	int missileRequestCount_ = 0; // 発射リクエスト数
	int missileRequestConsumeIndex_ = 0; // 発射リクエスト消費用インデックス
	// --- Missile simultaneous shot ---
	bool missileCharging_ = false; // 溜め中か
	float missileChargeTimer_ = 0.0f; // 溜め残り
	int missileChargeFrame_ = 0; // 間引き用
	Vector3 burstTargetSnap_{ 0.0f, 0.0f, 0.0f }; // 発射瞬間のplayer座標
	bool burstTargetValid_ = false; // スナップ座標が有効か
	bool burstCharged_ = false; // チャージ完了済みか
	//==============================
	// SlashWave（通常時攻撃その2）
	//==============================
	bool slashFireReq_ = false; // 発射要求フラグ
	Vector3 slashPos_{ 0.0f,0.0f,0.0f }; // 発射位置
	Vector3 slashTarget_{ 0.0f,0.0f,0.0f }; // 発射時点のplayer座標（到達点）
	// 予備動作（スラッシュ用：ミサイルとは別）
	bool slashCharging_ = false; // 溜め中か
	float slashChargeTimer_ = 0.0f; // 溜め残り
	int slashChargeFrame_ = 0; // 間引き用
	// クールタイム（連発防止）
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
	// StateMachine
	TKM::StateMachine sm_; // 状態遷移マシン
	Enemy* boss_ = nullptr; // Update中だけ有効
	Vector3 posWork_{}; // State側で動かす座標（最後にbossへ反映）
	//==============================
	// Config
	//==============================
	const BossControllerConfig* config_ = nullptr;
};