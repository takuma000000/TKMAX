#include "Enemy.h"
#include "Player.h"
#include <cmath>
#include <cstdlib>
#include "MyMath.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, const Vector3& position) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	camera_ = camera;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("enemy.obj");

	transform_.scale = { 3.0f, 3.0f, 3.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = position;
	initialPosition_ = position;

	ChangeDirection();
	fireTimer_ = rand() % fireInterval_;
}

void Enemy::Update(Player* player) {
	moveChangeTimer_--;
	if (moveChangeTimer_ <= 0) {
		ChangeDirection();
		moveChangeTimer_ = rand() % 90 + 30;
	}

	transform_.translate.x += velocity_.x;
	transform_.translate.y += velocity_.y;

	if (transform_.translate.x > initialPosition_.x + moveRadius_ || transform_.translate.x < initialPosition_.x - moveRadius_) {
		velocity_.x = -velocity_.x;
	}
	if (transform_.translate.y > initialPosition_.y + moveRadius_ || transform_.translate.y < initialPosition_.y - moveRadius_) {
		velocity_.y = -velocity_.y;
	}

	fireTimer_--;
	if (fireTimer_ <= 0) {
		FireBullet(player->GetTranslate());
		fireTimer_ = fireInterval_;
	}

	for (auto& bullet : bullets_) {
		bullet->Update();
	}
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<EnemyBullet>& bullet) {
				return !bullet->IsActive();
			}),
		bullets_.end());

	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
}

void Enemy::Draw() {
	object3d_->Draw(dxCommon_);
	for (const auto& bullet : bullets_) {
		bullet->Draw();
	}
}

void Enemy::SetCamera(Camera* camera) {
	object3d_->SetCamera(camera);
}

void Enemy::ChangeDirection() {
	float angle = (rand() % 360) * (3.141592f / 180.0f);
	Vector3 newVelocity = { cosf(angle) * moveSpeed_, sinf(angle) * moveSpeed_, 0.0f };
	velocity_.x = velocity_.x * 0.8f + newVelocity.x * 0.2f;
	velocity_.y = velocity_.y * 0.8f + newVelocity.y * 0.2f;
}

void Enemy::FireBullet(const Vector3& playerPos) {
	Vector3 direction = MyMath::Normalize(playerPos - transform_.translate);
	float bulletSpeed = 0.8f;

	auto bullet = std::make_unique<EnemyBullet>();
	bullet->Initialize(transform_.translate, direction * bulletSpeed, obj3dCo_, dxCommon_);
	bullet->SetCamera(camera_);
	bullets_.push_back(std::move(bullet));
}
