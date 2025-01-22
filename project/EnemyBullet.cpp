#include "EnemyBullet.h"

void EnemyBullet::Initialize(const Vector3& position, const Vector3& velocity, Object3dCommon* object3dCommon, DirectXCommon* dxCommon) {
	position_ = position;
	velocity_ = velocity;
	obj3dCo_ = object3dCommon;
	dxCommon_ = dxCommon;
	active_ = true;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("enemyBullet.obj");
	object3d_->SetTranslate(position_);

	// 初期位置の設定
	transform_.scale = { 0.6f, 0.6f, 0.6f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };
}

void EnemyBullet::Update() {
	if (!active_) return;

	position_ += velocity_;

	// オブジェクトの更新
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(position_);
	object3d_->Update();

	// Z座標の範囲で寿命判定
	if (position_.z < -500.0f || position_.z > 100.0f) {
		active_ = false;
	}
}

void EnemyBullet::Draw() {
	if (active_ && object3d_) {
		object3d_->Draw(dxCommon_);
	}
}

void EnemyBullet::SetCamera(Camera* camera) {
	if (object3d_) {
		object3d_->SetCamera(camera);
	}
}
