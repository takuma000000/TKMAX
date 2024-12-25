#include "Enemy.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, const Vector3& position) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	camera_ = camera;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("enemy.obj");

	// 初期位置の設定
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = position;
}

void Enemy::Update() {
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);

	object3d_->Update();
}

void Enemy::Draw() {
	object3d_->Draw(dxCommon_);
}

void Enemy::SetCamera(Camera* camera) {
	object3d_->SetCamera(camera);
}
