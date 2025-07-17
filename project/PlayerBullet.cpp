#include "PlayerBullet.h"
#include <engine/effect/ParticleManager.h>

void PlayerBullet::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetScale({ 0.2f, 0.2f, 0.2f });
}

void PlayerBullet::Update() {
	Vector3 pos = object_->GetTranslate();
	pos = pos + velocity_;
	object_->SetTranslate(pos);

	if (enemy_) {
		Vector3 enemyPos = enemy_->GetWorldPosition(); // 要：後述の関数追加
		float distance = MyMath::Length(pos - enemyPos);
		if (distance < 1.0f) {
			isHit_ = true;
			// パーティクル出す
			ParticleManager::GetInstance()->Emit("uv", pos, 30); // 名前と数を指定
			isDead_ = true;
			return;
		}
	}

	// 画面外 or 距離で死亡判定
	if (pos.z > 100.0f) {
		isDead_ = true;
	}
	object_->Update();
}

void PlayerBullet::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon);
}

void PlayerBullet::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos);
}

void PlayerBullet::SetVelocity(const Vector3& vel) {
	velocity_ = vel;
}
