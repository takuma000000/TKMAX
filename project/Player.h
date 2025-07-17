#pragma once

#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "ModelManager.h"

class Player {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);

	void SetCamera(Camera* camera) { this->camera = camera; }
	void SetPosition(const Vector3& pos);
	Vector3 GetPosition() const;

private:
	Camera* camera = nullptr;

	std::unique_ptr<Object3d> object_;
};
