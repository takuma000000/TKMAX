#pragma once

#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "application/enemy/Enemy.h"
#include <engine/effect/particle/ParticlerEmitter.h>

class Player;

class PlayerBullet {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);

	bool IsHit() const { return isHit_; }

	void SetPosition(const Vector3& pos);
	void SetVelocity(const Vector3& vel);
	bool IsDead() const { return isDead_; }
	void SetCamera(Camera* camera) {
		if (object_) {
			object_->SetCamera(camera);
		}
	}

	Enemy* GetEnemy() const { return enemy_; }
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	void SetPlayer(Player* player) { player_ = player; }
	void SetSpecialAttack(bool flag) { isSpecialAttack_ = flag; }
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
};
