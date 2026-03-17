#include "EnemyBullet.h"
#include <algorithm>

void EnemyBullet::Initialize(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera,
	const Vector3& position,
	const Vector3& velocity
) {
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetTranslate(position);
	object_->SetScale({ 0.6f, 0.6f, 0.6f });
	object_->SetColor({ 1.0f, 0.35f, 0.35f, 1.0f });

	camera_ = camera;
	if (camera_) {
		object_->SetCamera(camera_);
	}

	velocity_ = velocity;
	lifeTimer_ = 0.0f;
	isDead_ = false;
}

void EnemyBullet::Update(float dt) {
	if (!object_ || isDead_) {
		return;
	}

	Vector3 pos_ = object_->GetTranslate();
	pos_ += velocity_ * (dt * 60.0f);
	object_->SetTranslate(pos_);
	object_->Update();

	lifeTimer_ += dt;
	if (lifeTimer_ >= lifeTime_) {
		isDead_ = true;
	}
}

void EnemyBullet::Draw(TKM::DirectXCommon* dx) {
	if (!object_ || isDead_) {
		return;
	}
	object_->Draw(dx);
}

Vector3 EnemyBullet::GetWorldPosition() const {
	if (!object_) {
		return { 0.0f, 0.0f, 0.0f };
	}
	return object_->GetTranslate();
}