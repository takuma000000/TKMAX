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

	// 初期位置の設定
	transform_.scale = { 0.6f, 0.6f, 0.6f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };

	TextureManager::GetInstance()->LoadTexture("./resources/point.png");
	TextureManager::GetInstance()->LoadTexture("./resources/rockon.png");

	// ロックオンマーカーの初期化
	lockOnMarker_ = std::make_unique<Sprite>();
	lockOnMarker_->Initialize(spriteCommon_, dxCommon_, "./resources/rockon.png"); // マーカー用のテクスチャ
	lockOnMarker_->SetAnchorPoint({ 0.5f, 0.5f });
	lockOnMarker_->SetSize({ 0.5f, 0.5f }); // スプライトのサイズを調整
	lockOnMarker_->SetVisible(false); // 初期状態は非表示


}

void Player::Update(const std::vector<Enemy*>& enemies) {
	// **最も近いエネミーをロックオン**
	LockOnTarget(enemies);

	// **ロックオンした敵のマーカーを更新**
	if (targetEnemy_) {
		Vector3 enemyPos = targetEnemy_->GetPosition();
		enemyPos.y += 2.0f; // 頭上にマーカーを表示

		// ワールド座標をスクリーン座標に変換
		Vector3 screenPos = WorldToScreen(enemyPos);
		lockOnMarker_->SetPosition({ screenPos.x, screenPos.y });
		lockOnMarker_->SetVisible(true);
	} else {
		lockOnMarker_->SetVisible(false);
	}

	lockOnMarker_->Update();

	// **プレイヤーの移動処理**
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

	// **弾を発射**
	if (input_->TriggerKey(DIK_SPACE)) {
		FireBullet();
	}

	// **弾の更新**
	for (auto& bullet : bullets_) {
		bullet->Update();
	}

	// **不要な弾の削除**
	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<PlayerBullet>& bullet) {
				return !bullet->IsActive();
			}),
		bullets_.end());

	// **弾のカウントダウン処理**
	if (overTimer_ > 0.0f) {
		overTimer_ -= 1.0f / 60.0f;
		if (overTimer_ <= 0.0f) {
			overTimer_ = 0.0f;
		}
	}

	// **プレイヤーの3Dオブジェクトを更新**
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

	lockOnMarker_->Draw();
}

void Player::FireBullet()
{
	if (bulletCount_ <= 0 || targetEnemy_ == nullptr) {
		return; // 弾がない、またはターゲットがいない場合は発射しない
	}

	Vector3 targetPos = targetEnemy_->GetPosition();
	Vector3 direction = MyMath::Normalize(targetPos - transform_.translate);
	//弾の速度
	const float bulletSpeed = 3.0f;

	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(transform_.translate, direction * bulletSpeed, obj3dCo_, dxCommon_);
	bullet->SetCamera(camera_);
	bullets_.push_back(std::move(bullet));

	bulletCount_--;

	if (bulletCount_ == 0) {
		overTimer_ = 2.0f; // 10発撃ち切ったらリロードタイム開始
	}
}

Vector3 Player::GetTargetDirection() const
{
	// 仮のターゲットとして、Z方向（画面奥）を基準に計算
	Vector3 targetPosition = { transform_.translate.x, transform_.translate.y, transform_.translate.z + 50.0f };
	// プレイヤーからターゲットへの方向ベクトルを計算
	Vector3 direction = targetPosition - transform_.translate;
	return MyMath::Normalize(direction); // 正規化して方向ベクトルを返す
}

void Player::OnCollision()
{

}

void Player::SetCamera(Camera* camera)
{
	object3d_->SetCamera(camera);
}

Vector3 Player::ScreenToWorld(const POINT& screenPos) const
{
	// スクリーン座標を正規化デバイス座標 (NDC) に変換
	float ndcX = (2.0f * screenPos.x) / windo->GetWindowWidth() - 1.0f;
	float ndcY = 1.0f - (2.0f * screenPos.y) / windo->GetWindowHeight();

	// NDCを視点空間座標に変換
	DirectX::XMVECTOR screenSpace = DirectX::XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);
	DirectX::XMVECTOR viewSpace = DirectX::XMVector3TransformCoord(screenSpace, camera_->GetInverseProjectionMatrix());

	// 視点空間座標をワールド空間座標に変換
	DirectX::XMVECTOR worldSpace = DirectX::XMVector3TransformCoord(viewSpace, camera_->GetInverseViewMatrix());

	return { DirectX::XMVectorGetX(worldSpace), DirectX::XMVectorGetY(worldSpace), DirectX::XMVectorGetZ(worldSpace) };
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

void Player::SetSpriteCommon(SpriteCommon* spriteCommon)
{
	spriteCommon_ = spriteCommon;
}

void Player::LockOnTarget(const std::vector<Enemy*>& enemies)
{
	if (enemies.empty()) {
		targetEnemy_ = nullptr;
		lockOnMarker_->SetVisible(false); // ロックオン対象がいない場合は非表示
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

	if (targetEnemy_) {
		lockOnMarker_->SetVisible(true); // マーカーを表示
	} else {
		lockOnMarker_->SetVisible(false); // ターゲットがいない場合は非表示
	}
}

Vector3 Player::WorldToScreen(const Vector3& worldPos) const
{
	DirectX::XMVECTOR worldVec = DirectX::XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);

	// `Matrix4x4` を `DirectX::XMMATRIX` に変換
	DirectX::XMMATRIX viewProjMatrix = DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&camera_->GetViewProjectionMatrix()));

	// ワールド座標をスクリーン座標へ変換
	DirectX::XMVECTOR screenVec = DirectX::XMVector3TransformCoord(worldVec, viewProjMatrix);

	float screenX = (DirectX::XMVectorGetX(screenVec) + 1.0f) * 0.5f * windo->GetWindowWidth();
	float screenY = (1.0f - DirectX::XMVectorGetY(screenVec)) * 0.5f * windo->GetWindowHeight();

	return { screenX, screenY, 0.0f };
}
