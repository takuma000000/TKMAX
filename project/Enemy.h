#pragma once
#include <memory>
#include <Object3d.h>
#include <Object3dCommon.h>
#include <ModelManager.h>
#include <DirectXCommon.h>

class Enemy {
public:
	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();

	void SetCamera(Camera* cam);
	Object3d* GetObject3d() const;

private:
	DirectXCommon* dxCommon_ = nullptr;
	std::unique_ptr<Object3d> object3d_ = nullptr;
};
