#pragma once
#include <memory>
#include "Object3d.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "engine/func/math/Vector3.h"

class BossBullet {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dx, Camera* cam,
		const Vector3& pos, const Vector3& dir,
		float speed, int damage, int lifeFrame) {
		obj_ = std::make_unique<Object3d>();
		obj_->Initialize(common, dx);
		obj_->SetModel("sphere.obj");              // 既存モデルを使用
		obj_->SetScale({ 0.6f, 0.6f, 0.6f });        // 見やすいサイズ
		obj_->SetTranslate(pos);
		if (cam) obj_->SetCamera(cam);

		dir_ = dir;
		speed_ = speed;
		damage_ = damage;
		life_ = lifeFrame;
	}

	void Update() {
		if (dead_) return;
		Vector3 p = obj_->GetTranslate();
		p.x += dir_.x * speed_;
		p.y += dir_.y * speed_;
		p.z += dir_.z * speed_;
		obj_->SetTranslate(p);
		obj_->Update();
		if (--life_ <= 0) dead_ = true;
	}

	void Draw(DirectXCommon* dx) {
		if (!dead_) obj_->Draw(dx);
	}

	bool IsDead() const { return dead_; }
	int  Damage()  const { return damage_; }
	const Vector3& GetPos() const { return obj_->GetTranslate(); }
	float Radius() const { return 0.6f; } // 簡易当たり半径

private:
	std::unique_ptr<Object3d> obj_;
	Vector3 dir_{ 0,0,-1 };
	float   speed_ = 0.8f;
	int     damage_ = 1;
	int     life_ = 180;
	bool    dead_ = false;
};
