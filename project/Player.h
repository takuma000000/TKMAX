#pragma once
#include <memory>
#include <Object3d.h>
#include <Object3dCommon.h>
#include <ModelCommon.h>
#include <ModelManager.h>
#include <DirectXCommon.h>

class Player
{
public:
	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();

	void SetCamera(Camera* cam);
	Object3d* GetObject3d() const; // Cameraセット用に

private:
	DirectXCommon* dxCommon_ = nullptr;

	std::unique_ptr<Object3d> object3d_ = nullptr;
};

