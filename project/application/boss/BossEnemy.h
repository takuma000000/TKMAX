#pragma once
#include "application/enemy/Enemy.h"
#include "Camera.h"
#include <array>
#include <random>
#include "externals/imgui/imgui.h"

class BossEnemy : public Enemy {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void ImGuiDebug();

private:
	Camera* camera_ = nullptr;

	// ====== 既存のフェーズ管理 ======
	enum class Phase { P1, P2, P3 };
	Phase phase_ = Phase::P1;

	// ====== 攻撃ステージ管理（テレグラフ/発射/クールダウン） ======
	enum class ActStage { Telegraph, Fire, Cooldown };
	ActStage stage_ = ActStage::Cooldown;
	float stageT_ = 0.0f;

	// ====== 移動（周回＋到達減速） ======
	float theta_ = 0.0f;           // 周回角度
	float orbitR_ = 35.0f;         // 周回半径（動的に微ゆらぎ）
	float orbitOmega_ = 0.7f;      // 周回角速度（動的に微ゆらぎ）
	float dzMin_ = 45.0f;          // プレイヤーより奥側に居る最小差
	float maxSpeed_ = 0.6f;
	float arriveRadius_ = 3.0f;

	// ====== 攻撃の種類 ======
	enum class AttackType { Beam, Fan, Rapid };
	AttackType currentAttack_ = AttackType::Beam;

	// ====== AI: ユーティリティ選択用 ======
	struct CD {
		float cool = 90.0f;   // クールダウン(フレーム基準でOK)
		float t = 0.0f;    // 残り時間
	};
	CD cdBeam_{ 75.0f, 0.0f }, cdFan_{ 90.0f, 0.0f }, cdRapid_{ 45.0f, 0.0f };
	int sameAttackChain_ = 0;         // 同一攻撃の連続回数
	AttackType lastAttack_ = AttackType::Beam;

	// 重み（ImGuiで調整可能）
	float wDistBeam_ = 1.0f, wAlignBeam_ = 1.0f, phaseBiasBeam_ = 0.5f;
	float wDistFan_ = 1.2f, wAlignFan_ = 0.6f, phaseBiasFan_ = 0.7f;
	float wDistRapid_ = 0.6f, wAlignRapid_ = 1.2f, phaseBiasRapid_ = 1.0f;
	float jitterAmplitude_ = 0.35f;   // ランダムゆらぎの強さ
	int   maxSameChain_ = 2;          // 同じ技の最大連続回数

	// 扇/連射の可変パラメータ
	int   fanCount_ = 5;
	float fanSpread_ = 0.35f;         // 基本散開角（フレーム毎に微ゆらぎ）
	float rapidJitterX_ = 0.2f;
	float rapidJitterZ_ = 0.2f;

	// フェーズ別の軌道ゆらぎ
	float orbitR_Jitter_ = 2.0f;
	float orbitOmega_Jitter_ = 0.15f;

	// 先読み照準用（直近プレイヤー速度の指数平滑）
	Vector3 prevPlayerPos_{ 0,0,0 };
	Vector3 playerVelFiltered_{ 0,0,0 };
	float   velFilter_ = 0.2f;        // 0..1（大きいほど最新寄り）

	// 乱数
	std::mt19937 rng_{ 123456u };

	// 内部処理
	void UpdatePhase(); // HPでフェーズ切替
	void UpdateMovement(const Vector3& playerPos, const Vector3& playerVel);
	void UpdateAttack(float dt, const Vector3& playerPos);

	// 攻撃フロー
	void FireBegin();
	void FireTick(float dt, const Vector3& playerPos);
	void FireEnd();

	// ★ユーティリティで次の攻撃を選ぶ（ここがAIの核）
	void SelectNextAttackUtility(const Vector3& playerPos);

	// 時間制御（frame基準の既存値を活かす）
	float TelegraphTime() const;
	float FireTime() const;
	float CooldownTime() const;

	// 補助
	float Rand01();                   // 0..1
	void  TickCooldowns();            // CDを進める
	float DotXZ(const Vector3& a, const Vector3& b) const;
	Vector3 PredictPlayer(const Vector3& playerPos) const; // 先読み
	float PhaseBiasFor(AttackType at) const;

	// 演出
	float blinkT_ = 0.0f;

	// === 可視化用デバッグ ===
	struct DebugAI {
		bool  show = true;
		// 入力
		float dist = 0.f;     // プレイヤーまでの距離
		float align = 0.f;    // 0..1（正対度）
		float jitter = 0.f;   // ±jitterAmplitude_ から今回使われた値
		// 距離適性
		float distBeam = 0.f, distFan = 0.f, distRapid = 0.f;
		// バイアス
		float biasBeam = 0.f, biasFan = 0.f, biasRapid = 0.f;
		// ペナルティ
		float cdBeam = 0.f, cdFan = 0.f, cdRapid = 0.f;          // 0 or 0.6 など
		float chainBeam = 0.f, chainFan = 0.f, chainRapid = 0.f; // 0 or 0.7 など
		// 最終スコア
		float sBeam = 0.f, sFan = 0.f, sRapid = 0.f;
		// 選択候補
		int   chosen = 0; // 0:Beam,1:Fan,2:Rapid
	} dbg_;

	float p2RangeX_ = 12.0f;   // 左右幅（±）
	float p2RangeY_ = 1.5f;    // 上下ゆらぎ
	float p2OmegaX_ = 0.03f;   // 左右往復の角速度（遅め）
	float p2OmegaY_ = 0.02f;   // 上下ゆらぎの角速度
	float p2MaxSpeed_ = 0.8f;  // 追従（目標位置への移動）最大速度

	// 直近履歴（視覚化）
	static constexpr int kHist = 16;
	std::array<int, kHist> history_{};
	int histIndex_ = 0;

	const char* AttackName(AttackType at) const;
	void PushHistory(AttackType at);
};