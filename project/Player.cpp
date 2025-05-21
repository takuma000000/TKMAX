#include "Player.h"
#include <iostream>
#include <externals/imgui/imgui.h>
#include <TextureManager.h>

void Player::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, Input* input, SpriteCommon* spriteCommon) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	camera_ = camera;
	input_ = input;
	spriteCommon_ = spriteCommon;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("player.obj");

	transform_.scale = { 0.6f, 0.6f, 0.6f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };

	TextureManager::GetInstance()->LoadTexture("./resources/point.png");
	TextureManager::GetInstance()->LoadTexture("./resources/rockon.png");

	lockOnMarker_ = std::make_unique<Sprite>();
	lockOnMarker_->Initialize(spriteCommon_, dxCommon_, "./resources/rockon.png");
	lockOnMarker_->SetAnchorPoint({ 0.5f, 0.5f });
	lockOnMarker_->SetSize({ 0.5f, 0.5f });
	lockOnMarker_->SetVisible(false);
}

void Player::Update(const std::vector<Enemy*>& enemies) {
	if (hp_ > 0) {
		if (isInvincible_) {
			invincibleTime_ -= 1.0f / 60.0f;
			blinkTimer_ -= 1.0f / 60.0f;
			if (blinkTimer_ <= 0.0f) {
				blinkTimer_ = blinkInterval_;
				isVisible_ = !isVisible_;
			}
			if (invincibleTime_ <= 0.0f) {
				isInvincible_ = false;
				isVisible_ = true;
			}
		}

		LockOnTarget(enemies);
		if (targetEnemy_) {
			Vector3 enemyPos = targetEnemy_->GetPosition();
			enemyPos.y += 2.0f;
			Vector3 screenPos = WorldToScreen(enemyPos);
			lockOnMarker_->SetPosition({ screenPos.x, screenPos.y });
			lockOnMarker_->SetVisible(true);
		} else {
			lockOnMarker_->SetVisible(false);
		}
		lockOnMarker_->Update();

		if (input_->PushKey(DIK_W)) transform_.translate.y += 0.1f;
		if (input_->PushKey(DIK_S)) transform_.translate.y -= 0.1f;
		if (input_->PushKey(DIK_A)) transform_.translate.x -= 0.1f;
		if (input_->PushKey(DIK_D)) transform_.translate.x += 0.1f;

		if (input_->TriggerKey(DIK_SPACE)) FireBullet();

		for (auto& bullet : bullets_) bullet->Update();
		bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](const std::unique_ptr<PlayerBullet>& bullet) { return !bullet->IsActive(); }), bullets_.end());
	}

	if (overTimer_ > 0.0f) {
		overTimer_ -= 1.0f / 60.0f;
		if (overTimer_ <= 0.0f) overTimer_ = 0.0f;
	}

	if (hp_ <= 0 && hpOverTimer_ > 0.0f) {
		hpOverTimer_ -= 1.0f / 60.0f;
		if (hpOverTimer_ <= 0.0f) hpOverTimer_ = 0.0f;
	}

	if (hp_ > 0 && isVisible_) {
		object3d_->SetScale(transform_.scale);
		object3d_->SetRotate(transform_.rotate);
		object3d_->SetTranslate(transform_.translate);
		object3d_->Update();
	}
}

void Player::Draw() {
	lockOnMarker_->Draw();
	for (auto& bullet : bullets_) bullet->Draw();
	if (!isVisible_ || hp_ <= 0) return;
	object3d_->Draw(dxCommon_);
}

void Player::FireBullet() {
	if (hp_ <= 0 || bulletCount_ <= 0 || targetEnemy_ == nullptr) return;

	Vector3 targetPos = targetEnemy_->GetPosition();
	Vector3 direction = MyMath::Normalize(targetPos - transform_.translate);
	const float bulletSpeed = 3.0f;
	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(transform_.translate, direction * bulletSpeed, obj3dCo_, dxCommon_);
	bullet->SetCamera(camera_);
	bullets_.push_back(std::move(bullet));

	bulletCount_--;
	if (bulletCount_ == 0) overTimer_ = 2.0f;
}

void Player::OnCollision() {
	if (!isInvincible_) {
		isInvincible_ = true;
		invincibleTime_ = 3.0f;
		blinkTimer_ = blinkInterval_;
		isVisible_ = false;
		DecreaseHP();
		if (hp_ == 0) hpOverTimer_ = 2.0f;
	}
}

void Player::SetCamera(Camera* camera) {
	object3d_->SetCamera(camera);
}

void Player::DrawImGui() {
	if (ImGui::Begin("Player Controls")) {
		int bulletIndex = 0;
		for (auto& bullet : bullets_) {
			if (bullet->IsActive()) {
				Vector3 position = bullet->GetPosition();
				std::string label = "Bullet " + std::to_string(bulletIndex);
				if (ImGui::DragFloat3(label.c_str(), reinterpret_cast<float*>(&position), 0.1f)) {
					bullet->SetPosition(position);
				}
			}
			bulletIndex++;
		}
	}
	ImGui::End();
}

void Player::SetSpriteCommon(SpriteCommon* spriteCommon) {
	spriteCommon_ = spriteCommon;
}

void Player::LockOnTarget(const std::vector<Enemy*>& enemies) {
	if (enemies.empty()) {
		targetEnemy_ = nullptr;
		lockOnMarker_->SetVisible(false);
		return;
	}

	float minDistance = FLT_MAX;
	Enemy* closestEnemy = nullptr;
	for (Enemy* enemy : enemies) {
		if (enemy->IsDead()) continue;
		float distance = MyMath::Distance(transform_.translate, enemy->GetPosition());
		if (distance < minDistance) {
			minDistance = distance;
			closestEnemy = enemy;
		}
	}
	targetEnemy_ = closestEnemy;
	lockOnMarker_->SetVisible(targetEnemy_ != nullptr);
}

Vector3 Player::WorldToScreen(const Vector3& worldPos) const {
	DirectX::XMVECTOR worldVec = DirectX::XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);
	DirectX::XMMATRIX viewProjMatrix = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&camera_->GetViewProjectionMatrix()));
	DirectX::XMVECTOR screenVec = DirectX::XMVector3TransformCoord(worldVec, viewProjMatrix);
	float screenX = (DirectX::XMVectorGetX(screenVec) + 1.0f) * 0.5f * windo->GetWindowWidth();
	float screenY = (1.0f - DirectX::XMVectorGetY(screenVec)) * 0.5f * windo->GetWindowHeight();
	return { screenX, screenY, 0.0f };
}
