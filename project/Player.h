#pragma once

#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "ModelManager.h"
#include "PlayerBullet.h"
#include "Input.h"
#include "Enemy.h"
#include "externals/imgui/imgui.h"
#include <algorithm>
#include <list>

class Player {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void Draw(DirectXCommon* dxCommon);
	void ImGuiDebug();

	void RemoveEnemyIfDead();

	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const {
		return bullets_;
	}

	Vector3 GetPosition() const {
		return object_ ? object_->GetTranslate() : Vector3();
	}

	void SetCamera(Camera* camera) 
	{
		this->camera = camera;
		if (object_) {
			object_->SetCamera(camera);
		}
	}
	void SetPosition(const Vector3& pos);
	void SetParentScene(BaseScene* parentScene);
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }


private:

	void HandleGamePadMove();
	void HandleCameraControl();
	void HandleFollowCamera();
	void HandleShooting();

	Camera* camera = nullptr;
	Object3dCommon* common_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;
	Enemy* enemy_ = nullptr;

	BaseScene* parentScene_ = nullptr;
	std::unique_ptr<Object3d> object_;
	std::list<std::unique_ptr<PlayerBullet>> bullets_;
};
