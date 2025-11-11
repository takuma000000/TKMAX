#pragma once

#undef max
#undef min

#define NOMINMAX
#include <algorithm>
#include <string>
#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "application/enemy/Enemy.h"
#include <engine/effect/particle/ParticlerEmitter.h>

class Player;

//=============================================================
// PlayerBulletクラス
// プレイヤーの弾を管理するクラス。
//=============================================================
class PlayerBullet {
public:

	/// <summary>プレイヤーの弾を初期化します。</summary>
	/// <param name="common">Object3d共通。</param>
	/// <param name="dxCommon">DirectX共通。</param>
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	/// <summary>プレイヤーの弾を更新します。</summary>
	void Update();
	/// <summary>プレイヤーの弾を描画します。</summary>
	void Draw(DirectXCommon* dxCommon);

	/// <summary>デバッグ用ImGui表示。</summary>
	bool IsHit() const { return isHit_; }

	/// <summary>弾が当たったときの処理。</summary>
	bool IsDead() const { return isDead_; }

	// setter
	/// <summary>発射の「出方」曲線を開始します。</summary>
	void StartSpawnBezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float duration, const Vector3& velocityAfter);
	/// <summary>弾が当たったときの処理。</summary>
	void SetPosition(const Vector3& pos);
	/// <summary>弾の速度を設定します。</summary>
	void SetVelocity(const Vector3& vel);
	/// <summary>弾が当たったときの処理。</summary>
	void SetCamera(Camera* camera) {
		if (object_) {
			object_->SetCamera(camera);
		}
	}
	/// <summary>弾の軌跡パーティクルのグループを設定します。</summary>
	void SetTrailGroup(const std::string& group) {
		trailGroup_ = group;
		// 位置は現在地で再初期化（生成直後や途中でもOK）
		Vector3 pos = object_ ? object_->GetTranslate() : Vector3{};
		trailEmitter_.Initialize(trailGroup_, pos);
	}
	/// <summary>弾が当たったときの処理。</summary>
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	/// <summary>弾が当たったときの処理。</summary>
	void SetPlayer(Player* player) { player_ = player; }
	/// <summary>弾が当たったときの処理。</summary>
	void SetSpecialAttack(bool flag) { isSpecialAttack_ = flag; }
	/// <summary>ホーミング設定。</summary>
	void SetHoming(bool enable, float speed) { isHoming_ = enable; homingSpeed_ = speed; }
	/// <summary>ホーミング遅延時間設定。</summary>
	void  SetHomingDelay(float sec) { homingDelay_ = std::max(0.0f, sec); }

	// getter
	/// <summary>弾が当たったときの処理。</summary>
	Enemy* GetEnemy() const { return enemy_; }

private:
	Player* player_ = nullptr;

	std::unique_ptr<Object3d> object_;
	Vector3 velocity_{};
	bool isDead_ = false;
	bool isHit_ = false;

	Enemy* enemy_ = nullptr;

	bool isSpecialAttack_ = false; // 一撃必殺フラグ

	bool  isHoming_ = false;
	float homingSpeed_ = 0.6f; // 追従弾の速度（調整可）
	float homingDelay_ = 0.0f;     // 追尾開始までの遅延秒
	bool isSpawningCurve_ = false;    // 発射の「出方」曲線フェーズ中か
	float spawnT_ = 0.0f;            // 0..1 の補間量
	float spawnDuration_ = 0.25f;    // 出方にかける秒数（調整可）
	Vector3 bezP0_, bezP1_, bezP2_, bezP3_; // ベジェ制御点
	Vector3 postSpawnVelocity_ = { 0,0,0 };   // 曲線フェーズ終了後に引き継ぐ速度

	ParticleEmitter trailEmitter_; // 弾の軌跡パーティクル
	std::string trailGroup_ = "bulletTrail"; // デフォルトのパーティクルグループ名

	/// <summary>発射の「出方」曲線を更新します。</summary>
	void UpdateSpawnBezier();
};
