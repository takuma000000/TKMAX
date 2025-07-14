#include "Player.h"

void Player::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
}

void Player::Update() {
	object_->Update();
}

void Player::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon);
}

void Player::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos);
}

Vector3 Player::GetPosition() const {
	return object_->GetTranslate();
}
