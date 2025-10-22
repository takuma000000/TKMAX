#pragma once

#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "application/enemy/Enemy.h"
#include <engine/effect/particle/ParticlerEmitter.h>
#include <string>

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
	void SetPosition(const Vector3& pos);
	/// <summary>弾の速度を設定します。</summary>
	void SetVelocity(const Vector3& vel);
	/// <summary>弾が当たったときの処理。</summary>
	bool IsDead() const { return isDead_; }
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
	Enemy* GetEnemy() const { return enemy_; }
	/// <summary>弾が当たったときの処理。</summary>
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	/// <summary>弾が当たったときの処理。</summary>
	void SetPlayer(Player* player) { player_ = player; }
	/// <summary>弾が当たったときの処理。</summary>
	void SetSpecialAttack(bool flag) { isSpecialAttack_ = flag; }
	/// <summary>ホーミング設定。</summary>
	void SetHoming(bool enable, float speed) { isHoming_ = enable; homingSpeed_ = speed; }

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

	ParticleEmitter trailEmitter_; // 弾の軌跡パーティクル
	std::string trailGroup_ = "bulletTrail"; // 既定
};
