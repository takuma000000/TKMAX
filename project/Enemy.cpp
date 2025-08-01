#include "Enemy.h"
#include "ModelManager.h"

void Enemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj"); // モデル名は適宜変更
	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon);

	if (camera) {
		object_->SetCamera(camera);
	}
}

void Enemy::Update() {
	if (!stopMove_) {
		Vector3 pos = object_->GetTranslate();
		pos += velocity_;

		if (pos.z <= stopZ_) {
			pos.z = stopZ_;     // ぴったり止める
			stopMove_ = true;   // 移動停止
		}

		object_->SetTranslate(pos);
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

	ImGui::Text("HP: %d", hp_);
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
