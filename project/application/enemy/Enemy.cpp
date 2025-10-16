#include "Enemy.h"
#include "ModelManager.h"

void Enemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("enemy.obj"); // モデル名は適宜変更
	ModelManager::GetInstance()->LoadModel("enemy.obj", dxCommon);

	if (camera) {
		object_->SetCamera(camera);
	}

	baseScale_ = object_->GetScale(); // 元のスケールを保持
	colliderScale_ = baseScale_; // 当たり判定用スケールも初期化
	startX_ = object_->GetTranslate().x; // サイン波の基準用
}

void Enemy::Update() {
	// ---- 位置更新（挙動別）----
	Vector3 pos = object_->GetTranslate();

	switch (behavior_) {
	case EnemyBehavior::StraightStop: {
		if (!stopMove_) {
			pos += velocity_;
			if (pos.z <= stopZ_) { pos.z = stopZ_; stopMove_ = true; }
		}
		break;
	}
	case EnemyBehavior::SineX: {
		t_ += 0.05f;
		pos.z += velocity_.z; // 手前へ
		pos.x = startX_ + std::sinf(sinePhase_ + t_ * sineFreq_) * sineAmpX_;
		if (pos.z <= stopZ_) { pos.z = stopZ_; }
		break;
	}
	case EnemyBehavior::StrafeLtoR: {
		pos.z += velocity_.z;
		// 簡易左右往復
		strafePosX_ += strafeSpeed_ * strafeDir_;
		if (strafePosX_ > strafeRight_) { strafePosX_ = strafeRight_; strafeDir_ = -1; }
		if (strafePosX_ < strafeLeft_) { strafePosX_ = strafeLeft_;  strafeDir_ = +1; }
		pos.x = strafePosX_;
		if (pos.z <= stopZ_) { pos.z = stopZ_; }
		break;
	}
	case EnemyBehavior::ChasePlayer: {
		pos.z += velocity_.z;
		if (playerGetter_) {
			Vector3 toP = playerGetter_() - pos;
			Vector3 desire = { toP.x, toP.y, 0.0f };
			float len = MyMath::Length(desire);
			if (len > 0.001f) {
				Vector3 dir = MyMath::Normalize(desire);
				pos.x += dir.x * chaseSpeed_;
				pos.y += dir.y * chaseSpeed_;
			}
		}
		if (pos.z <= stopZ_) { pos.z = stopZ_; }
		break;
	}

	}

	object_->SetTranslate(pos);

	// ---- ロック中のパルス（既存）----
	if (isLocked_) {
		pulseT_ += 0.12f;
		float s = 1.0f + 0.15f * sinf(pulseT_);
		object_->SetScale({ baseScale_.x * s, baseScale_.y * s, baseScale_.z * s });
	} else {
		object_->SetScale(baseScale_);
	}

	// ---- 将来の射撃フック（必要になったら実装）----
	if (canShoot_) {
		shootTimer_++;
		if (shootTimer_ >= shootInterval_) {
			shootTimer_ = 0.0f;
			// TODO: ここで parentScene_ 経由などで EnemyBullet を生成
			// 例）親に「(pos, 進行方向)」を通知して生成してもらう
		}
	}

	object_->Update();
}


void Enemy::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon);
}

void Enemy::SetCamera(Camera* camera) {
	this->camera = camera;
	if (object_) {
		object_->SetCamera(camera);
	}
}

void Enemy::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos);
}

void Enemy::SetParentScene(BaseScene* scene) {
	parentScene_ = scene;
}

Vector3 Enemy::GetWorldPosition() const {
	return object_->GetTranslate();
}

void Enemy::ImGuiDebug() {
	if (!object_) return;

	ImGui::Begin("Enemy");

	Vector3 pos = object_->GetTranslate();
	Vector3 rot = object_->GetRotate();
	Vector3 scale = object_->GetScale();

	if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) {
		object_->SetTranslate(pos);
	}
	if (ImGui::DragFloat3("Rotation", &rot.x, 0.01f)) {
		object_->SetRotate(rot);
	}
	if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
		object_->SetScale(scale);
	}

	ImGui::Text("N_EnemyHP: %d", hp_);
	ImGui::Text("Dead: %s", isDead_ ? "true" : "false");

	ImGui::End();
}

void Enemy::OnHit()
{
	hp_--;
	if (hp_ <= 0) {
		isDead_ = true;
	}
}

void Enemy::OnHitWithDamage(int damage)
{
	hp_ -= damage;
	if (hp_ <= 0) {
		isDead_ = true;
	}
}
