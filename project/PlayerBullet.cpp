#define NOMINMAX
#include "PlayerBullet.h"
#include <engine/effect/ParticleManager.h>
#include "AABB.h"

void PlayerBullet::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetScale({ 0.2f, 0.2f, 0.2f });
}

void PlayerBullet::Update() {
	// 現在の座標を取得して、速度分だけ進める
	Vector3 pos = object_->GetTranslate();
	pos = pos + velocity_;
	object_->SetTranslate(pos);

	// 敵が存在するなら当たり判定チェック
	if (enemy_) {
		// 弾の座標とスケールを取得
		Vector3 bulletPos = object_->GetTranslate();
		Vector3 bulletScale = object_->GetScale();

		// 敵のワールド座標とスケールを取得
		Vector3 enemyPos = enemy_->GetWorldPosition();
		Vector3 enemyScale = enemy_->GetScale();

		// 弾と敵のAABB（軸に沿ったバウンディングボックス）を作成
		AABB bulletBox(bulletPos, bulletScale);
		AABB enemyBox(enemyPos, enemyScale);

		// AABB同士の当たり判定を行う
		if (bulletBox.IsCollidingWithAABB(enemyBox)) {
			// 当たったらフラグを立てて削除対象にする
			isHit_ = true;
			isDead_ = true;

			// パーティクルを発生させる
			ParticleManager::GetInstance()->Emit("uv", bulletPos, 30);

			return; // 一度当たったらそれ以上の処理はしない
		}
	}

	// 一定距離（Z方向）を超えたら弾を削除する
	if (pos.z > 100.0f) {
		isDead_ = true;
	}

	// Object3d の更新処理（行列など）
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
