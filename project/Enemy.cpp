#include "Enemy.h"

void Enemy::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon_);

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(Object3dCommon::GetInstance(), Object3dCommon::GetInstance()->GetDxCommon());
	object3d_->SetModel("sphere.obj"); // 敵のモデル（お好みで）
	object3d_->SetTranslate({ 5, 1, 0 }); // 初期位置（お好みで）
}

void Enemy::Update() {
	if (object3d_) {
		object3d_->Update();
	}
}

void Enemy::Draw() {
	if (object3d_) {
		object3d_->Draw(Object3dCommon::GetInstance()->GetDxCommon());
	}
}

void Enemy::SetCamera(Camera* cam) {
	if (object3d_) {
		object3d_->SetCamera(cam);
	}
}

Object3d* Enemy::GetObject3d() const {
	return object3d_.get();
}
