#pragma once
#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "EnemyBullet.h"

class Player;

class Enemy {
public:
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, const Vector3& position);
	void Update(Player* player);
	void Draw();

	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

	const Vector3& GetScale() const { return transform_.scale; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }

	void SetCamera(Camera* camera);

	bool IsDead() const { return isDead_; }
	void OnCollision() { isDead_ = true; }
	const Vector3& GetPosition() const { return transform_.translate; }

	void ChangeDirection();
	const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return bullets_; }

private:
	Transform transform_;
	std::unique_ptr<Object3d> object3d_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;
	Object3dCommon* obj3dCo_ = nullptr;
	Camera* camera_ = nullptr;
	bool isDead_ = false;

	Vector3 velocity_;
	Vector3 initialPosition_;
	float moveRadius_ = 3.0f;
	float moveSpeed_ = 0.25f;
	int moveChangeTimer_ = 0;

	std::vector<std::unique_ptr<EnemyBullet>> bullets_;
	int fireTimer_ = 0;
	const int fireInterval_ = 120;

	void FireBullet(const Vector3& playerPos);
};
