#pragma once
#include "Vector3.h"
#include "Object3d.h"

class EnemyBullet {
public:
	void Initialize(const Vector3& position, const Vector3& velocity, Object3dCommon* object3dCommon, DirectXCommon* dxCommon);
	void Update();
	void Draw();

	void SetCamera(Camera* camera);
	bool IsActive() const { return active_; }
	const Vector3& GetPosition() const { return position_; }
	void Deactivate() { active_ = false; }

private:
	Vector3 position_;
	Vector3 velocity_;
	bool active_ = false;

	Transform transform_;

	std::unique_ptr<Object3d> object3d_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;
	Object3dCommon* obj3dCo_ = nullptr;
};
