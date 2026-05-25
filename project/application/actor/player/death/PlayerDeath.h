#pragma once
#include "Object3d.h"

//=============================================================
// PlayerDeathクラス
// プレイヤーの撃墜開始、落下、吹き飛び、回転を管理するクラス。
//=============================================================
class PlayerDeath {
public:

	/// <summary>
	/// 撃墜状態を初期化します。
	/// </summary>
	void Initialize();
	/// <summary>
	/// 撃墜演出を更新します。
	/// </summary>
	/// <param name="ownerObject">プレイヤー本体</param>
	void Update(TKM::Object3d* ownerObject);
	/// <summary>
	/// 撃墜開始処理を行います。
	/// </summary>
	void Start();

	/// <summary>
	/// 撃墜中かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 撃墜開始時の一度きり処理が必要かどうか。
	/// </summary>
	/// <returns></returns>
	bool IsStartRequested() const { return startRequested_; }

	/// <summary>
	/// 撃墜開始時の一度きり処理要求を消費します。
	/// </summary>
	void ConsumeStartRequest();

private:

	//=============================================================
	// 撃墜状態
	//=============================================================
	bool isDead_ = false;          // 撃墜中か
	bool startRequested_ = false;  // 撃墜開始時の一度きり処理要求

	Vector3 deathVelocity_ = { 0.0f, 0.0f, 0.0f };        // 故障落下中の速度
	Vector3 deathAngularVelocity_ = { 0.0f, 0.0f, 0.0f }; // 故障落下中の角速度

	//=============================================================
	// 撃墜調整値
	//=============================================================
	static constexpr float kDeathBackwardSpeed_ = 0.55f;    // 後方へ吹き飛ぶ強さ
	static constexpr float kDeathFallStartSpeed_ = 0.01f;   // 落下開始速度
	static constexpr float kDeathGravity_ = 0.006f;         // 落下重力
	static constexpr float kDeathFallMaxSpeed_ = 0.25f;     // 落下速度の最大値
	static constexpr float kDeathBackwardDamping_ = 0.992f; // 後方速度の減衰
	static constexpr float kDeathRotateDamping_ = 0.992f;   // 回転速度の減衰
	static constexpr float kDeathMaxPitch_ = 1.20f;         // ピッチ最大値
	static constexpr float kDeathMaxRoll_ = 0.80f;          // ロール最大値
};