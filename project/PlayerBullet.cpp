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
		// 敵のワールド座標を取得
		Vector3 enemyPos = enemy_->GetWorldPosition();
		// 敵のスケールから最大軸を使ってヒット判定半径を算出（+0.5fで調整）
		Vector3 enemyScale = enemy_->GetScale();
		float hitRadius = std::max({ enemyScale.x, enemyScale.y, enemyScale.z }) * 0.5f + 0.5f;
		// 弾と敵の距離が一定未満ならヒットと判定
		if (MyMath::Length(pos - enemyPos) < hitRadius) {
			isHit_ = true;// ヒットフラグを立てる
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
