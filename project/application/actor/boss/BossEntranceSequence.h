#pragma once
#include "MyMath.h"
#include "StateMachine.h"

class BossManager;

//=============================================================
// BossEntranceSequenceクラス
// WAVE終了後のボス登場演出を管理するクラス。
// 演出中はゲームプレイをロックし、演出の途中で
// BossManager::StartBattle() を呼び出します。
//=============================================================
class BossEntranceSequence : public TKM::IStateContext {
public:
	BossEntranceSequence() = default;
	~BossEntranceSequence() = default;

	/// <summary>
	/// ボス登場演出を開始します。
	/// </summary>
	/// <param name="spawnPos">ボスの出現位置（ワールド座標）</param>
	void Start(const Vector3& spawnPos);

	/// <summary>
	/// ボス登場演出を更新します。
	/// </summary>
	/// <param name="dt">経過時間（秒）</param>
	/// <param name="bossManager">ボスマネージャ</param>
	void Update(float dt, BossManager* bossManager);

	/// <summary>
	/// 演出中かどうかを返します。
	/// </summary>
	/// <returns>trueなら演出中、falseなら未演出または終了済み</returns>
	bool IsActive() const { return isActive_; }

	/// <summary>
	/// ボスを生成済みかどうかを返します。
	/// </summary>
	/// <returns>trueなら生成済み、falseなら未生成</returns>
	bool HasSpawnedBoss() const { return bossSpawned_; }

	// Getter=================================================
	/// <summary>
	/// 現在の空色を返します。
	/// </summary>
	/// <returns>現在の空色</returns>
	const Vector4& GetSkyColor() const { return currentSkyColor_; }
	// ========================================================

private:

	/// <summary>
	/// 2色のカラーを t（0.0f～1.0f）で線形補間します。
	/// </summary>
	/// <param name="a">開始色</param>
	/// <param name="b">終了色</param>
	/// <param name="t">補間係数（0.0f～1.0f）</param>
	/// <returns>補間後の色</returns>
	static Vector4 LerpColor_(const Vector4& a, const Vector4& b, float t);

	/// <summary>
	/// 吸い込み予兆のエフェクトを発生させます。
	/// </summary>
	void EmitGather_();

	/// <summary>
	/// ボス出現時のバーストエフェクトを発生させます。
	/// </summary>
	void EmitBurst_();

	/// <summary>
	/// 押し出し余韻のエフェクトを発生させます。
	/// </summary>
	void EmitPushWave_();

	/// <summary>
	/// 覆いエフェクトを発生させます。
	/// </summary>
	void EmitCover_();

	/// <summary>
	/// 演出状態を初期化して、最初の状態に戻します。
	/// </summary>
	void Reset_();

	//======================================================================
	// 演出時間定数
	//======================================================================
	static constexpr float kWaitSec_ = 0.30f;       // 開始前の待機時間
	static constexpr float kSkyFadeInSec_ = 0.65f;  // 空色を変化させる時間
	static constexpr float kGatherSec_ = 0.75f;     // 吸い込み演出の継続時間
	static constexpr float kCoverSec_ = 0.48f;      // 覆い演出の継続時間
	static constexpr float kPushSec_ = 0.55f;       // 押し出し演出の継続時間
	static constexpr float kBurstSec_ = 0.48f;      // バースト演出の継続時間

	//======================================================================
	// エフェクト放出間隔
	//======================================================================
	static constexpr float kGatherEmitInterval_ = 0.03f; // 吸い込み粒子の放出間隔
	static constexpr float kRingEmitInterval_ = 0.08f;   // リング状エフェクトの放出間隔
	static constexpr float kCoverEmitInterval_ = 0.05f;  // 覆いエフェクトの放出間隔
	static constexpr float kPushEmitInterval_ = 0.06f;   // 押し出し余韻の放出間隔

	//======================================================================
	// 演出回数・色設定
	//======================================================================
	static constexpr int kCoverSpawnEmitCount_ = 4; // Cover中に覆いエフェクトを放出する回数

	const Vector4 kBaseSkyColor_{ 1.0f, 1.0f, 1.0f, 1.0f };  // 通常時の空色
	const Vector4 kRedSkyColor_{ 10.0f, 0.0f, 0.0f, 1.0f };  // 演出ピーク時の空色

	//======================================================================
	// 演出状態
	//======================================================================
	bool isActive_ = false;     // 演出が進行中かどうか
	bool bossSpawned_ = false;  // ボスを生成済みかどうか

	float phaseTimer_ = 0.0f;        // 現在フェーズの経過時間
	float convergeEmitTimer_ = 0.0f; // 吸い込み粒子の放出タイマー
	float glowEmitTimer_ = 0.0f;     // 発光演出の放出タイマー
	float holdEmitTimer_ = 0.0f;     // 保持演出用タイマー
	float coverEmitTimer_ = 0.0f;    // 覆いエフェクトの放出タイマー

	int coverEmitCount_ = 0; // Cover中に覆いエフェクトを何回放出したか

	//======================================================================
	// 位置・色情報
	//======================================================================
	Vector3 spawnPos_{ 0.0f, 0.0f, 0.0f };           // ボスの出現位置
	Vector4 currentSkyColor_{ 1.0f, 1.0f, 1.0f, 1.0f }; // 現在の空色

	//======================================================================
	// バースト演出用パラメータ
	//======================================================================
	bool burstFxEmitted_ = false;                    // バースト演出を発生済みかどうか
	Vector3 burstStartPos_{ 0.0f, 0.0f, 0.0f };     // バースト開始位置
	Vector3 burstEndPos_{ 0.0f, 0.0f, 0.0f };       // バースト終了位置
	Vector3 burstStartScale_{ 1.0f, 1.0f, 1.0f };   // バースト開始スケール
	Vector3 burstEndScale_{ 1.0f, 1.0f, 1.0f };     // バースト終了スケール

	//======================================================================
	// ステートマシン / 外部参照
	//======================================================================
	TKM::StateMachine sm_;           // 演出進行を管理するステートマシン
	BossManager* bossManager_ = nullptr; // ボスマネージャへの参照

	//======================================================================
	// フレンドステート
	//======================================================================
	friend class BossEntranceWaitState;
	friend class BossEntranceSkyFadeInState;
	friend class BossEntranceGatherState;
	friend class BossEntranceCoverState;
	friend class BossEntranceBurstState;
	friend class BossEntrancePushState;
	friend class BossEntranceDoneState;
};