#pragma once

#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "Enemy.h"

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
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }

private:
	std::unique_ptr<Object3d> object_;
	Vector3 velocity_{};
	bool isDead_ = false;
	bool isHit_ = false;

	Enemy* enemy_ = nullptr;
};
