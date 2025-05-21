#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "Camera.h"

class PlayerBullet {
public:
	void Initialize(const Vector3& position, const Vector3& velocity, Object3dCommon* object3dCommon, DirectXCommon* dxCommon);
	void Update();
	void Draw();

	bool IsActive() const { return active_; }
	void Deactivate() { active_ = false; }

	void SetCamera(Camera* camera);
	const Vector3& GetPosition() const;
	void SetPosition(const Vector3& position);

private:
	Transform transform_;
	Vector3 position_;
	Vector3 velocity_;
	bool active_ = false;

	DirectXCommon* dxCommon_ = nullptr;
	Object3dCommon* ob3dCo_ = nullptr;

	std::unique_ptr<Object3d> object3d_;
};
