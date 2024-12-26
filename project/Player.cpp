#include "Player.h"
#include <iostream>
#include <externals/imgui/imgui.h>

void Player::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, Input* input) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	camera_ = camera;
	input_ = input;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("player.obj");

	// 初期位置の設定
	transform_.scale = { 0.6f, 0.6f, 0.6f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };


}

void Player::Update() {
	// 入力による移動
	if (input_->PushKey(DIK_W)) {
		transform_.translate.y += 0.1f;
	}
	if (input_->PushKey(DIK_S)) {
		transform_.translate.y -= 0.1f;
	}
	if (input_->PushKey(DIK_A)) {
		transform_.translate.x -= 0.1f;
	}
	if (input_->PushKey(DIK_D)) {
		transform_.translate.x += 0.1f;
	}


	if (input_->PushKey(DIK_SPACE)) {
		FireBullet();
	}


	// 弾の更新
	for (auto& bullet : bullets_) {
		bullet->Update();
	}

	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<PlayerBullet>& bullet) {
				return !bullet->IsActive();
			}),
		bullets_.end());

	// オブジェクトの更新
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();
}

void Player::Draw() {

	// 弾の描画
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}

	object3d_->Draw(dxCommon_);
}

void Player::FireBullet()
{
	auto bullet = std::make_unique<PlayerBullet>();

	Vector3 bulletVelocity = GetTargetDirection() * 1.0f;
	bullet->Initialize(transform_.translate, bulletVelocity, obj3dCo_, dxCommon_);
	// カメラを設定
	bullet->SetCamera(camera_);


	if (!bullet) {
		std::cerr << "Bullet initialization failed!" << std::endl;
		return;
	}

	std::cout << "Bullet created at: ("
		<< transform_.translate.x << ", "
		<< transform_.translate.y << ", "
		<< transform_.translate.z << ")" << std::endl;

	bullets_.push_back(std::move(bullet));
}

Vector3 Player::GetTargetDirection() const
{
	// 仮のターゲットとして、Z方向（画面奥）を基準に計算
	Vector3 targetPosition = { transform_.translate.x, transform_.translate.y, transform_.translate.z + 50.0f };
	// プレイヤーからターゲットへの方向ベクトルを計算
	Vector3 direction = targetPosition - transform_.translate;
	return MyMath::Normalize(direction); // 正規化して方向ベクトルを返す
}

void Player::SetCamera(Camera* camera)
{
	object3d_->SetCamera(camera);
}

void Player::DrawImGui()
{
	if (ImGui::Begin("Player Controls")) {
		// プレイヤーの弾を表示
		int bulletIndex = 0;
		for (auto& bullet : bullets_) {
			// 弾がアクティブなら位置を操作
			if (bullet->IsActive()) {
				Vector3 position = bullet->GetPosition();
				std::string label = "Bullet " + std::to_string(bulletIndex);
				if (ImGui::DragFloat3(label.c_str(), reinterpret_cast<float*>(&position), 0.1f)) {
					bullet->SetPosition(position); // 位置を更新
				}
			}
			bulletIndex++;
		}
	}
	ImGui::End();
}
