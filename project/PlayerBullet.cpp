#define NOMINMAX
#include "PlayerBullet.h"
#include <engine/effect/ParticleManager.h>

void PlayerBullet::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
	object_->SetScale({ 0.2f, 0.2f, 0.2f });
}

void PlayerBullet::Update() {
	// 現在位置を取得し、速度分だけ進める
	Vector3 pos = object_->GetTranslate();
	pos = pos + velocity_;
	object_->SetTranslate(pos);

	// 敵が存在する場合、当たり判定チェック
	if (enemy_) {
		// 敵のワールド座標（モデルの中心）を取得
		Vector3 enemyPos = enemy_->GetWorldPosition();

		// 固定の当たり判定半径（スケールに依存しない）
		const float hitRadius = 1.8f; // 見た目に合わせて調整

		// 距離を計算して当たり判定
		float dist = MyMath::Length(pos - enemyPos);
		if (dist < hitRadius) {
			isHit_ = true; // ヒットフラグを立てる

			// ヒット時にパーティクルを発生させる
			ParticleManager::GetInstance()->Emit("uv", pos, 30);

			// 弾を消す
			isDead_ = true;
			return;
		}
	}

	// 画面外に出たら弾を消す（Z軸方向100.0fを超えたら死亡）
	if (pos.z > 100.0f) {
		isDead_ = true;
	}

	// Object3dの更新処理
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
