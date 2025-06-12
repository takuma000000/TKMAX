#include "Player.h"

void Player::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	ModelManager::GetInstance()->LoadModel("cube.obj", dxCommon_);

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(Object3dCommon::GetInstance(), Object3dCommon::GetInstance()->GetDxCommon());
	object3d_->SetModel("cube.obj"); // ここはお好みのモデルに
	object3d_->SetTranslate({ 0, 1, 0 }); // 初期位置もお好みで
}

void Player::SetCamera(Camera* cam) {
	if (object3d_) {
		object3d_->SetCamera(cam);
	}
}

Object3d* Player::GetObject3d() const {
	return object3d_.get();
}

void Player::Update() {
	if (object3d_) {
		object3d_->Update();
	}
}

void Player::Draw() {
	if (object3d_) {
		object3d_->Draw(Object3dCommon::GetInstance()->GetDxCommon());
	}
}

