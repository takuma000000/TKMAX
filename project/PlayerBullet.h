#pragma once
#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "ModelManager.h"

class PlayerBullet {
public:
	void Initialize(DirectXCommon* dxCommon, const Vector3& startPos, const Vector3& targetPos);
	void Update();
	void Draw();

	bool IsDead() const { return isDead_; }
	void SetCamera(Camera* cam);

	// PlayerBullet.h
	Vector3 GetPosition() const { return position_; }

private:
	Vector3 position_;
	Vector3 velocity_;
	float speed_ = 0.15f; // ゆっくり飛ぶくらい（好みに応じて）

	std::unique_ptr<Object3d> object3d_;
	DirectXCommon* dxCommon_ = nullptr;
	bool isDead_ = false;
};
