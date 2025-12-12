#pragma once
#include "application/enemy/Enemy.h"
#include "engine/3d/camera/Camera.h"
#include <array>
#include <random>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// Bossの各種定数
namespace BossParam {
	// --- フェーズ切り替え HP ---
	constexpr int Phase2HP = 60;
	constexpr int Phase3HP = 30;
	// --- P1 移動 ---
	constexpr float P1FrontZ = 20.0f;
	constexpr float P1MaxSpeed = 0.8f;
	constexpr float P1StopRadius = 1.5f;
	// --- 共通 ---
	constexpr float ArriveRadius = 3.0f;
	// --- P3 移動 ---
	constexpr float P3CenterOffsetZ = 20.0f;
	constexpr float P3RangeX = 18.0f;
	constexpr float P3RangeY = 3.0f;
	constexpr float P3OmegaX = 0.05f;
	constexpr float P3OmegaY = 0.035f;
	constexpr float P3MaxSpeed = 1.2f;
	// --- 先読み ---
	constexpr float LookAheadFrames = 6.0f;
	// --- テレグラフ / 発射 / クールダウン ---
	constexpr float TelegraphP1 = 60.0f;
	constexpr float TelegraphP2 = 45.0f;
	constexpr float TelegraphP3 = 30.0f;
	constexpr float FireP1 = 40.0f;
	constexpr float FireP2 = 60.0f;
	constexpr float FireP3 = 90.0f;
	constexpr float CooldownP1 = 120.0f;
	constexpr float CooldownP2 = 60.0f;
	constexpr float CooldownP3 = 45.0f;
	// --- ビーム / 扇 / 連射の距離帯 ---
	constexpr float BeamDistCenter = 45.0f;
	constexpr float BeamDistWidth = 30.0f;
	constexpr float FanDistCenter = 32.0f;
	constexpr float FanDistWidth = 18.0f;
	constexpr float RapidDistCenter = 18.0f;
	constexpr float RapidDistWidth = 16.0f;
	// --- ペナルティ ---
	constexpr float CooldownPenaltyValue = 0.6f;
	constexpr float ChainPenaltyValue = 0.7f;
	// --- FireTick: 発射間隔 & 性能 ---
	// P1やさしいビーム
	constexpr int   P1BeamInterval = 24;
	constexpr float P1BeamSpeed = 0.55f;
	constexpr int   P1BeamDamage = 1;
	constexpr int   P1BeamLife = 150;
	// P2/P3 ビーム
	constexpr int   BeamInterval = 3;
	constexpr float BeamSpeed = 0.7f;
	constexpr int   BeamDamage = 2;
	constexpr int   BeamLife = 240;
	// 扇
	constexpr int   FanInterval = 10;
	constexpr float FanSpeed = 0.9f;
	constexpr int   FanDamage = 1;
	constexpr int   FanLife = 180;
	// 連射
	constexpr int   RapidInterval = 6;   // 「6フレームごと」にしておく
	constexpr float RapidSpeed = 1.4f;
	constexpr int   RapidDamage = 1;
	constexpr int   RapidLife = 120;
	// --- ロック時スケール・点滅 ---
	constexpr float LockBlinkSpeed = 0.2f;
	constexpr float LockBlinkAmount = 0.2f;
	constexpr float NormalScale = 5.0f;
	constexpr float NormalCollider = 5.5f;
	constexpr float LockedCollider = 5.0f;
	// --- 数学系 ---
	constexpr float EpsilonLength = 1e-4f;
	// === Telegraph(予備動作) ===
	constexpr float TeleP1 = 60.0f;
	constexpr float TeleP2 = 45.0f;
	constexpr float TeleP3 = 30.0f;
	// === Cooldown(休憩時間) ===
	constexpr float CD_P1 = 120.0f;
	constexpr float CD_P2 = 60.0f;
	constexpr float CD_P3 = 45.0f;
	// --- フェーズ別バイアス（固定値） ---
	constexpr float P1_FanWeak = 0.4f;
	constexpr float P1_RapidWeak = 0.2f;
	constexpr float P2_BeamMid = 0.6f;
	constexpr float P2_RapidMid = 0.5f;
	constexpr float P3_FanMid = 0.7f;
	constexpr float P3_BeamWeak = 0.4f;
	// --- 初期設定 --- 
	constexpr int   InitHP = 80;
	constexpr float InitScale = 5.0f;
	constexpr Vector3 InitColliderScale = { 12.180f,18.210f,11.560f }; // モデル基準
}

//=============================================================
// BossEnemyクラス
// ボスの挙動と攻撃を制御するクラス。
//=============================================================
class BossEnemy : public Enemy {
public:

	/// <summary>
	/// ボスの初期化を行います。
	/// </summary>
	/// <param name="common"></param>
	/// <param name="dxCommon"></param>
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	/// <summary>
	/// ボスの更新を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// ImGuiデバッグ表示を行います。
	/// </summary>
	void ImGuiDebug();

	// Getter===================================
	/// <summary>
	/// 現在のフェーズを取得します。
	/// </summary>
	/// <returns></returns>
	int GetPhase() const { return static_cast<int>(phase_); }
	// =========================================
private:
	//======================================================================
	// フェーズ / ステージ管理
	//======================================================================
	// ====== 既存のフェーズ管理 ======
	enum class Phase { P1, P2, P3 };
	Phase phase_ = Phase::P1;

	// ====== 攻撃ステージ管理（テレグラフ/発射/クールダウン） ======
	enum class ActStage { Telegraph, Fire, Cooldown };
	ActStage stage_ = ActStage::Cooldown;
	float    stageT_ = 0.0f;
	//======================================================================
	// 移動（周回＋到達減速）
	//======================================================================
	// ====== 移動（周回＋到達減速） ======
	float theta_ = 0.0f;  // 周回角度
	float dzMin_ = 45.0f; // プレイヤーより奥側に居る最小差
	float arriveRadius_ = 3.0f;

	// P2 用レンジ・速度
	float p2RangeX_ = 12.0f;   // 左右幅（±）
	float p2RangeY_ = 1.5f;    // 上下ゆらぎ
	float p2OmegaX_ = 0.03f;   // 左右往復の角速度（遅め）
	float p2OmegaY_ = 0.02f;   // 上下ゆらぎの角速度
	float p2MaxSpeed_ = 0.8f;    // 追従（目標位置への移動）最大速度
	//======================================================================
	// 攻撃タイプ / 実行中攻撃
	//======================================================================
	// ====== 攻撃の種類 ======
	enum class AttackType { Beam, Fan, Rapid };
	AttackType currentAttack_ = AttackType::Beam;
	//======================================================================
	// AI: ユーティリティ選択 / クールダウン管理
	//======================================================================
	// ====== AI: ユーティリティ選択用 ======
	struct CD {
		float cool = 90.0f; // クールダウン(フレーム基準でOK)
		float t = 0.0f;  // 残り時間
	};

	CD cdBeam_{ 75.0f, 0.0f }, cdFan_{ 90.0f, 0.0f }, cdRapid_{ 45.0f, 0.0f };

	int        sameAttackChain_ = 0;                 // 同一攻撃の連続回数
	AttackType lastAttack_ = AttackType::Beam;

	// 重み（ImGuiで調整可能）
	float wDistBeam_ = 1.0f, wAlignBeam_ = 1.0f, phaseBiasBeam_ = 0.5f;
	float wDistFan_ = 1.2f, wAlignFan_ = 0.6f, phaseBiasFan_ = 0.7f;
	float wDistRapid_ = 0.6f, wAlignRapid_ = 1.2f, phaseBiasRapid_ = 1.0f;
	float jitterAmplitude_ = 0.35f;  // ランダムゆらぎの強さ
	int   maxSameChain_ = 2;      // 同じ技の最大連続回数

	// 扇/連射の可変パラメータ
	int   fanCount_ = 5;
	float fanSpread_ = 0.35f;   // 基本散開角（フレーム毎に微ゆらぎ）
	float rapidJitterX_ = 0.2f;
	float rapidJitterZ_ = 0.2f;
	//======================================================================
	// 先読み照準 / プレイヤー履歴
	//======================================================================
	// 先読み照準用（直近プレイヤー速度の指数平滑）
	Vector3 prevPlayerPos_{ 0,0,0 };
	Vector3 playerVelFiltered_{ 0,0,0 };
	float   velFilter_ = 0.2f;   // 0..1（大きいほど最新寄り）
	//======================================================================
	// 内部処理メソッド
	//======================================================================
	/// <summary>
	/// フェーズの更新を行います。
	/// </summary>
	void UpdatePhase(); // HPでフェーズ切替
	/// <summary>
	/// 移動の更新を行います。
	/// </summary>
	/// <param name="playerPos"></param>
	/// <param name="playerVel"></param>
	void UpdateMovement(const Vector3& playerPos, const Vector3& playerVel);
	/// <summary>
	/// 攻撃の更新を行います。
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="playerPos"></param>
	void UpdateAttack(float dt, const Vector3& playerPos);
	/// <summary>
	/// 攻撃開始処理。
	/// </summary>
	void FireBegin();
	/// <summary>
	/// 攻撃継続処理。
	/// </summary>
	/// <param name="dt"></param>
	/// <param name="playerPos"></param>
	void FireTick(float dt, const Vector3& playerPos);
	/// <summary>
	/// 攻撃終了処理。
	/// </summary>
	void FireEnd();
	/// <summary>
	/// 次の攻撃をユーティリティ選択で決定します。
	/// </summary>
	/// <param name="playerPos"></param>
	void SelectNextAttackUtility(const Vector3& playerPos);
	/// <summary>
	/// テレグラフ時間（Telegraph）の秒数を返します。
	/// </summary>
	/// <returns></returns>
	float TelegraphTime() const;
	/// <summary>
	/// 発射時間（Fire）の秒数を返します。
	/// </summary>
	/// <returns></returns>
	float FireTime() const;
	/// <summary>
	/// クールダウン時間（Cooldown）の秒数を返します。
	/// </summary>
	/// <returns></returns>
	float CooldownTime() const;
	/// <summary>
	/// 各攻撃のクールダウンを進めます。
	/// </summary>
	void  TickCooldowns();            // CDを進める
	/// <summary>
	/// プレイヤーの先読み位置を返します。
	/// </summary>
	/// <param name="playerPos"></param>
	/// <returns></returns>
	Vector3 PredictPlayer(const Vector3& playerPos) const; // 先読み
	/// <summary>
	/// 指定した攻撃タイプのフェーズバイアスを返します。
	/// </summary>
	/// <param name="at"></param>
	/// <returns></returns>
	float PhaseBiasFor(AttackType at) const;
	/// <summary>
	/// 到達減速付きシークベクトルを計算します。
	/// </summary>
	/// <param name="current"></param>
	/// <param name="target"></param>
	/// <param name="maxSpeed"></param>
	/// <param name="arriveRadius"></param>
	/// <returns></returns>
	Vector3 SeekArrive(const Vector3& current, const Vector3& target, float maxSpeed, float arriveRadius) const;
	//======================================================================
	// 演出
	//======================================================================
	float blinkT_ = 0.0f;
	//======================================================================
	// デバッグ可視化（ImGui）
	//======================================================================
	// === 可視化用デバッグ ===
	struct DebugAI {
		bool  show = true;
		// 入力
		float dist = 0.f;  // プレイヤーまでの距離
		float align = 0.f;  // 0..1（正対度）
		float jitter = 0.f;  // ±jitterAmplitude_ から今回使われた値
		// 距離適性
		float distBeam = 0.f, distFan = 0.f, distRapid = 0.f;
		// バイアス
		float biasBeam = 0.f, biasFan = 0.f, biasRapid = 0.f;
		// ペナルティ
		float cdBeam = 0.f, cdFan = 0.f, cdRapid = 0.f;          // 0 or 0.6 など
		float chainBeam = 0.f, chainFan = 0.f, chainRapid = 0.f;          // 0 or 0.7 など
		// 最終スコア
		float sBeam = 0.f, sFan = 0.f, sRapid = 0.f;
		// 選択候補
		int   chosen = 0; // 0:Beam,1:Fan,2:Rapid
	} dbg_;
	
	// 直近履歴（視覚化 
	static constexpr int kHist = 16;
	std::array<int, kHist> history_{};
	int histIndex_ = 0;

	bool deathSequence_ = false; // 最終死亡リアクション中かどうか
	float deathTimer_ = 0.0f;   // 最終死亡リアクション用タイマー
	float deathShakePower_ = 0.15f; // 最終死亡リアクション用カメラ揺れ強度

	/// <summary>
	/// 攻撃タイプ名を取得します。
	/// </summary>
	/// <param name="at"></param>
	/// <returns></returns>
	const char* AttackName(AttackType at) const;
	/// <summary>
	/// 攻撃履歴に記録します。
	/// </summary>
	/// <param name="at"></param>
	void PushHistory(AttackType at);
};