#pragma once
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "BaseScene.h"
#include "externals/imgui/imgui.h"

class Enemy {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);
	void ImGuiDebug();

	void SetCamera(Camera* camera);
	void SetPosition(const Vector3& pos);
	void SetParentScene(BaseScene* scene);
	Vector3 GetWorldPosition() const;
	Vector3 GetScale() const {
		return object_ ? object_->GetScale() : Vector3{ 1.0f, 1.0f, 1.0f };
	}

private:
	std::unique_ptr<Object3d> object_;
	Camera* camera = nullptr;
	BaseScene* parentScene_ = nullptr;
};
