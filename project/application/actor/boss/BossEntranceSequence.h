#pragma once
#include "MyMath.h"

class BossManager;

//=============================================================
// BossEntranceSequenceクラス
// ・WAVE3終了後のボス登場演出を管理するクラス。
// ・演出中はGameScene側でゲームプレイをロックし、
//   演出の途中でBossManager::StartBattle()を呼ぶ。
//=============================================================
class BossEntranceSequence {
public:
	BossEntranceSequence() = default;
	~BossEntranceSequence() = default;

	/// <summary>
	/// ボス登場演出を開始します。
	/// </summary>
	/// <param name="spawnPos">ボスの出現位置</param>
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
	bool IsActive() const { return isActive_; }
	/// <summary>
	/// ボスを生成済みかどうかを返します。
	/// </summary>
	bool HasSpawnedBoss() const { return bossSpawned_; }

	// Getter=================================================
	/// <summary>
	/// 現在の空色を返します。
	/// </summary>
	const Vector4& GetSkyColor() const { return currentSkyColor_; }
	// ========================================================

private:
	enum class Phase {
		Idle,       // 待機状態（非アクティブ）
		Wait,       // 全滅後の一拍
		SkyFadeIn,  // 空を赤へ染める
		Gather,     // 吸い込み予兆
		Cover,      // 赤空で全体を覆う
		Burst,      // 巨大出現バースト + ボス生成
		Push,       // 赤空の中で押し出し余韻
		Done,       // 演出完了
	};

	/// <summary>
	/// 2色のカラーを t (0.0f～1.0f) で線形補間します。
	/// </summary>
	/// <param name="a">開始色</param>
	/// <param name="b">終了色</param>
	/// <param name="t">補間パラメータ（0.0f～1.0f）</param>
	/// <returns>補間された色</returns>
	static Vector4 LerpColor_(const Vector4& a, const Vector4& b, float t);
	/// <summary>
	/// 吸い込み予兆のエフェクトを発生させます。
	/// </summary>
	void EmitGather_();
	/// <summary>
	/// ボス出現のバーストエフェクトを発生させます。
	/// </summary>
	void EmitBurst_();
	/// <summary>
	/// 押し出し余韻のエフェクトを発生させます。
	/// </summary>
	void EmitPushWave_();
	/// <summary>
	/// 覆いのエフェクトを発生させます。
	/// </summary>
	void EmitCover_();
	/// <summary>
	/// 演出をリセットして、最初の状態に戻します。
	/// </summary>
	void Reset_();

	static constexpr float kWaitSec_ = 0.30f;
	static constexpr float kSkyFadeInSec_ = 0.65f;
	static constexpr float kGatherSec_ = 0.75f;
	static constexpr float kCoverSec_ = 0.48f;
	static constexpr float kPushSec_ = 0.55f;

	static constexpr float kGatherEmitInterval_ = 0.03f;
	static constexpr float kRingEmitInterval_ = 0.08f;
	static constexpr float kCoverEmitInterval_ = 0.05f;
	static constexpr float kPushEmitInterval_ = 0.06f;

	static constexpr float kBurstSec_ = 0.35f;
	static constexpr int kCoverSpawnEmitCount_ = 4;

	const Vector4 kBaseSkyColor_{ 1.0f, 1.0f, 1.0f, 1.0f }; // 最初の空色（白）
	const Vector4 kRedSkyColor_{ 10.0f, 0.0f, 0.0f, 1.0f }; // 赤く強烈に染まった空色（演出のピークで使用）

	bool isActive_ = false;
	bool bossSpawned_ = false;

	Phase phase_ = Phase::Idle;
	float phaseTimer_ = 0.0f; // 現在のフェーズの経過時間を計測するタイマー
	float convergeEmitTimer_ = 0.0f;
	float glowEmitTimer_ = 0.0f;
	float holdEmitTimer_ = 0.0f;
	float coverEmitTimer_ = 0.0f;

	int coverEmitCount_ = 0; // Cover中に覆いパーティクルを何回放出したか

	Vector3 spawnPos_{ 0.0f, 0.0f, 0.0f }; // ボスの出現位置（ワールド座標）
	Vector4 currentSkyColor_{ 1.0f, 1.0f, 1.0f, 1.0f }; // 現在の空色（フェーズに応じて変化させる）

	bool burstFxEmitted_ = false;
	Vector3 burstStartPos_{ 0.0f, 0.0f, 0.0f };
	Vector3 burstEndPos_{ 0.0f, 0.0f, 0.0f };
	Vector3 burstStartScale_{ 1.0f, 1.0f, 1.0f };
	Vector3 burstEndScale_{ 1.0f, 1.0f, 1.0f };
};