#pragma once

#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "ModelManager.h"
#include "Input.h"
#include "externals/imgui/imgui.h"
#include <algorithm>

class Player {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);
	void ImGuiDebug();

	void SetCamera(Camera* camera) 
	{
		this->camera = camera;
		if (object_) {
			object_->SetCamera(camera);
		}
	}
	void SetPosition(const Vector3& pos);
	void SetParentScene(BaseScene* parentScene);

private:

	void HandleGamePadMove();
	void HandleCameraControl();
	void HandleFollowCamera();

	Camera* camera = nullptr;

	BaseScene* parentScene_ = nullptr;
	std::unique_ptr<Object3d> object_;
};
