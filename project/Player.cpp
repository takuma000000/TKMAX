#include "Player.h"


void Player::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	common_ = common;
	dxCommon_ = dxCommon;

	object_ = std::make_unique<Object3d>();
	object_->Initialize(common_, dxCommon_);
	object_->SetModel("sphere.obj");
}

void Player::Update() {
	HandleGamePadMove();
	//HandleCameraControl();
	HandleFollowCamera();

	HandleShooting(); // 先にプレイヤーの操作より下に置くと自然
	for (auto it = bullets_.begin(); it != bullets_.end(); ) {
		(*it)->Update();
		if ((*it)->IsDead()) {
			it = bullets_.erase(it);
		} else {
			++it;
		}
	}

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

	for (auto& bullet : bullets_) {
		bullet->Draw(dxCommon);
	}
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

	const float moveSpeed = 0.2f;
	const SHORT deadZone = 8000;

	SHORT lx = input->GetLeftStickX();
	SHORT ly = input->GetLeftStickY();

	float stickX = abs(lx) > deadZone ? (lx / 32768.0f) : 0.0f;
	float stickY = abs(ly) > deadZone ? (ly / 32768.0f) : 0.0f;

	if (stickX == 0.0f && stickY == 0.0f) return;

	Vector3 pos = object_->GetTranslate();
	pos.x += stickX * moveSpeed;
	pos.y -= -stickY * moveSpeed;
	object_->SetTranslate(pos);
}

void Player::HandleCameraControl() {
	if (!camera) return;

	Input* input = Input::GetInstance();

	const float sensitivity = 0.02f; // 回転感度（調整してOK）
	const SHORT deadZone = 8000;

	SHORT rx = input->GetRightStickX();
	SHORT ry = input->GetRightStickY();

	// デッドゾーン処理
	float rotX = abs(ry) > deadZone ? -(ry / 32768.0f) * sensitivity : 0.0f;
	float rotY = abs(rx) > deadZone ? (rx / 32768.0f) * sensitivity : 0.0f;

	Vector3 rotation = camera->GetRotate();
	rotation.x += rotX;
	rotation.y += rotY;

	// X軸回転に制限をかける（真上向いたり真下向いたりしないように）
	const float limitX = 1.5f; // ≒ 85度くらい
	rotation.x = std::clamp(rotation.x, -limitX, limitX);

	camera->SetRotate(rotation);
}

void Player::HandleFollowCamera() {
	if (!camera) return;

	// プレイヤーの位置取得
	Vector3 playerPos = object_->GetTranslate();

	// カメラの回転から方向を求める（Y軸回転だけ使う）
	Vector3 camRot = camera->GetRotate();
	float distance = 40.0f;   // プレイヤーからの距離
	float height = 4.0f;      // プレイヤーより高い位置

	// 回転角度からカメラの方向を計算（XZ平面）
	float angleY = camRot.y;
	Vector3 offset = {
		sinf(angleY) * -distance,
		height,
		cosf(angleY) * -distance
	};

	// カメラ位置を設定（プレイヤーに追従）
	Vector3 cameraPos = {
		playerPos.x + offset.x,
		playerPos.y + offset.y,
		playerPos.z + offset.z
	};
	camera->SetTranslate(cameraPos);
}

void Player::HandleShooting() {
	Input* input = Input::GetInstance();
	if (input->TriggerButton(XINPUT_GAMEPAD_RIGHT_SHOULDER)) {
		auto bullet = std::make_unique<PlayerBullet>();
		bullet->Initialize(common_, dxCommon_);

		Vector3 startPos = object_->GetTranslate();
		bullet->SetPosition(startPos);

		if (enemy_) {
			Vector3 enemyPos = enemy_->GetWorldPosition();
			Vector3 dir = enemyPos - startPos;
			float length = MyMath::Length(dir);

			if (length < 0.01f) {
				dir = { 0, 0, 1 }; // fallback
			} else {
				dir = MyMath::Normalize(dir);
			}

			bullet->SetVelocity(dir * 0.5f);
		} else {
			bullet->SetVelocity({ 0, 0, 0.5f });
		}

		bullet->SetCamera(camera);
		bullet->SetEnemy(enemy_);
		bullets_.push_back(std::move(bullet));
	}
}





