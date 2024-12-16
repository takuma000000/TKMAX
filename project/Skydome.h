#pragma once
#include <memory>
#include "Object3d.h"
#include "Vector3.h"

#include "DirectXCommon.h"
#include "Object3dCommon.h"

#include "Camera.h"

class Sprite;

class Skydome {
public:
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon);
	void Update();
	void Draw();
	void SetCamera(Camera* camera);

	// スケール、回転、位置の設定
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

private:
	Transform transform_;

	std::unique_ptr<Object3d> object3d_ = nullptr;

	DirectXCommon* dxCommon_ = nullptr;
	Object3dCommon* obj3dCo_ = nullptr;

};
