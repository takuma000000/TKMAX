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

	camera_ = camera;
	if (camera_) {
		object_->SetCamera(camera_);
	}

	velocity_ = velocity;
	lifeTimer_ = 0.0f;
	isDead_ = false;
	type_ = Type::Normal;
	scaleNow_ = 0.6f;
	scaleEnd_ = 0.6f;
	chargeTimer_ = 0.0f;
	chargeDuration_ = 0.0f;
	damage_ = 1;
	visible_ = true;
}

void EnemyBullet::Update(float dt) {
	if (!object_ || isDead_) {
		return;
	}

	Vector3 pos_ = object_->GetTranslate();

	if (type_ == Type::SpecialCoreCharging) {
		chargeTimer_ += dt;

		float t_ = 1.0f;
		if (chargeDuration_ > 0.0001f) {
			t_ = chargeTimer_ / chargeDuration_;
		}
		t_ = std::clamp(t_, 0.0f, 1.0f);

		const float sc_ = scaleNow_ + (scaleEnd_ - scaleNow_) * t_;
		object_->SetScale({ sc_, sc_, sc_ });
		radius_ = sc_ * 0.75f;

		object_->Update();
		return;
	}

	// 通常弾 / 発射済みSP玉
	pos_ += velocity_ * (dt * 60.0f);
	object_->SetTranslate(pos_);
	object_->Update();

	lifeTimer_ += dt;
	if (lifeTimer_ >= lifeTime_) {
		isDead_ = true;
	}
}

void EnemyBullet::Draw(TKM::DirectXCommon* dx) {
	if (!object_ || isDead_ || !visible_) {
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

void EnemyBullet::InitializeSpecialCore(
	TKM::Object3dCommon* common,
	TKM::DirectXCommon* dxCommon,
	TKM::Camera* camera,
	const Vector3& position,
	float startScale,
	float endScale,
	float radius,
	float chargeDuration,
	int damage
) {
	object_ = std::make_unique<TKM::Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetTranslate(position);
	object_->SetScale({ startScale, startScale, startScale });

	camera_ = camera;
	if (camera_) {
		object_->SetCamera(camera_);
	}

	velocity_ = { 0.0f, 0.0f, 0.0f };
	radius_ = radius;
	lifeTimer_ = 0.0f;
	lifeTime_ = 8.0f;
	isDead_ = false;

	type_ = Type::SpecialCoreCharging;
	scaleNow_ = startScale;
	scaleEnd_ = endScale;
	chargeTimer_ = 0.0f;
	chargeDuration_ = chargeDuration;
	damage_ = damage;
	visible_ = false;
}

void EnemyBullet::LaunchSpecialCore(const Vector3& velocity) {
	velocity_ = velocity;
	type_ = Type::SpecialCoreLaunched;
}

void EnemyBullet::SetVisible(bool visible) {
	visible_ = visible;
}

void EnemyBullet::SetPosition(const Vector3& position) {
	if (!object_) { return; }
	object_->SetTranslate(position);
}

void EnemyBullet::SetScale(float uniformScale) {
	scaleNow_ = uniformScale;
	if (!object_) { return; }
	object_->SetScale({ uniformScale, uniformScale, uniformScale });
}

void EnemyBullet::SetColor(const Vector4& color) {
	if (!object_) { return; }
	object_->SetColor(color);
}