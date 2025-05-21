#include "PlayerBullet.h"
#include <iostream>

void PlayerBullet::Initialize(const Vector3& position, const Vector3& velocity, Object3dCommon* object3dCommon, DirectXCommon* dxCommon) {
	position_ = position;
	velocity_ = velocity;
	dxCommon_ = dxCommon;
	ob3dCo_ = object3dCommon;
	active_ = true;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(ob3dCo_, dxCommon_);
	object3d_->SetModel("bullet.obj");
	object3d_->SetTranslate(position_);

	transform_.scale = { 0.6f, 0.6f, 0.6f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };
}

void PlayerBullet::Update() {
	if (!active_) return;

	position_ += velocity_;
	object3d_->SetTranslate(position_);

	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->Update();

	if (position_.z > 100.0f || position_.z < -100.0f) {
		active_ = false;
		std::cout << "Bullet deactivated due to lifetime." << std::endl;
	}
}

void PlayerBullet::Draw() {
	if (active_) {
		std::cout << "Drawing bullet at: ("
			<< position_.x << ", "
			<< position_.y << ", "
			<< position_.z << ")" << std::endl;

		if (object3d_) {
			object3d_->Draw(dxCommon_);
		} else {
			std::cerr << "Object3d is null!" << std::endl;
		}
	} else {
		std::cerr << "Bullet is inactive, skipping draw." << std::endl;
	}
}

void PlayerBullet::SetCamera(Camera* camera) {
	if (object3d_) {
		object3d_->SetCamera(camera);
	}
}

const Vector3& PlayerBullet::GetPosition() const {
	return position_;
}

void PlayerBullet::SetPosition(const Vector3& position) {
	position_ = position;
	object3d_->SetTranslate(position);
}
