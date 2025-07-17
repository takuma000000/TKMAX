#include "Player.h"
#include "externals/imgui/imgui.h"

void Player::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	object_ = std::make_unique<Object3d>();
	object_->Initialize(common, dxCommon);
	object_->SetModel("sphere.obj");
}

void Player::Update() {
	HandleGamePadMove();
	object_->Update();
}

void Player::ImGuiDebug() {
	if (!object_) return;

	ImGui::Begin("Player");

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

	ImGui::End();
}


void Player::Draw(DirectXCommon* dxCommon) {
	object_->Draw(dxCommon);
}

void Player::SetPosition(const Vector3& pos) {
	object_->SetTranslate(pos);
}

void Player::SetParentScene(BaseScene* scene) {
	// 必要な処理を書く、例：
	parentScene_ = scene;
}

void Player::HandleGamePadMove() {
	Input* input = Input::GetInstance();

	const float moveSpeed = 0.1f;
	const SHORT deadZone = 8000;

	SHORT lx = input->GetLeftStickX();
	SHORT ly = input->GetLeftStickY();

	float moveX = abs(lx) > deadZone ? (lx / 32768.0f) * moveSpeed : 0.0f;
	float moveZ = abs(ly) > deadZone ? (ly / 32768.0f) * moveSpeed : 0.0f;

	Vector3 pos = object_->GetTranslate();
	pos.x += moveX;
	pos.z += moveZ;
	object_->SetTranslate(pos);
}

