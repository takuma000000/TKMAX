#include "Enemy.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, const Vector3& position) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	camera_ = camera;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("enemy.obj");

	// 初期位置の設定
	transform_.scale = { 3.0f, 3.0f, 3.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = position;

	initialPosition_ = position; // 初期位置を保存

	// ランダムな初速度を設定
	ChangeDirection();
}

void Enemy::Update() {
	moveChangeTimer_--;

	// 一定時間ごとに方向変更
	if (moveChangeTimer_ <= 0) {
		ChangeDirection();
		moveChangeTimer_ = rand() % 60 + 30; // 30～90フレームで方向を変える
	}

	// 移動
	transform_.translate.x += velocity_.x;
	transform_.translate.y += velocity_.y;

	// 画面外や移動範囲外に出たら方向を変える
	if (transform_.translate.x > initialPosition_.x + moveRadius_ || transform_.translate.x < initialPosition_.x - moveRadius_) {
		velocity_.x = -velocity_.x;
	}
	if (transform_.translate.y > initialPosition_.y + moveRadius_ || transform_.translate.y < initialPosition_.y - moveRadius_) {
		velocity_.y = -velocity_.y;
	}

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

void Enemy::ChangeDirection()
{
	float angle = (rand() % 360) * (3.141592f / 180.0f); // 0〜360度のランダムな角度
	velocity_.x = cos(angle) * moveSpeed_;
	velocity_.y = sin(angle) * moveSpeed_;
}
