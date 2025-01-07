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

	// Spriteの初期化
	mouseSprite_ = std::make_unique<Sprite>();
	mouseSprite_->Initialize(spriteCommon_, dxCommon_, "./resources/point.png");
	mouseSprite_->SetAnchorPoint({ 0.5f, 0.5f }); // 初期位置


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

	if (input_->TriggerKey(DIK_SPACE)) {
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

	// 10発撃ち切った後のカウントダウン処理
	if (overTimer_ > 0.0f) {
		overTimer_ -= 1.0f / 60.0f; // 1フレームごとに約1/60秒減少
		if (overTimer_ <= 0.0f) {
			overTimer_ = 0.0f; // 0以下にならないように
		}
	}

	// オブジェクトの更新
	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);
	object3d_->Update();

	// マウス座標を取得
	POINT mousePos = input_->GetMousePosition();

	// **スプライトの位置をマウス座標に直接設定**
	mouseSprite_->SetPosition({ static_cast<float>(mousePos.x), static_cast<float>(mousePos.y) });

	// スプライトの更新
	mouseSprite_->Update();
}

void Player::Draw() {

	// 弾の描画
	for (auto& bullet : bullets_) {
		bullet->Draw();
	}

	object3d_->Draw(dxCommon_);

	// Spriteの描画
	mouseSprite_->Draw();
}

void Player::FireBullet()
{
	if (bulletCount_ <= 0) {
		return; // 弾がない場合は発射しない
	}

	// マウス座標を取得
	POINT mousePos = input_->GetMousePosition();
	Vector3 targetWorldPos = ScreenToWorld(mousePos);
	Vector3 direction = MyMath::Normalize(targetWorldPos - transform_.translate);
	const float bulletSpeed = 1.0f;

	auto bullet = std::make_unique<PlayerBullet>();
	bullet->Initialize(transform_.translate, direction * bulletSpeed, obj3dCo_, dxCommon_);
	bullet->SetCamera(camera_);
	bullets_.push_back(std::move(bullet));

	// 弾の残数を減らす
	bulletCount_--;

	// 10発目を撃ったタイミングで overTimer_ を開始（5秒）
	if (bulletCount_ == 0) {
		overTimer_ = 5.0f;
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
