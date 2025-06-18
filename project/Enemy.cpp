#include "Enemy.h"
#include "Player.h"

void Enemy::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, const Vector3& position) {
	dxCommon_ = dxCommon;
	obj3dCo_ = object3dCommon;
	camera_ = camera;

	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(obj3dCo_, dxCommon_);
	object3d_->SetModel("enemy.obj");

	// 初期位置の設定
	transform_.scale = { 3.0f, 3.0f, 3.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = position;

	initialPosition_ = position; // 初期位置を保存

	// ランダムな初速度を設定
	ChangeDirection();

	// 弾の発射タイマーをランダムに設定（0～fireInterval_ の範囲）
	fireTimer_ = rand() % fireInterval_;
}

void Enemy::Update(Player* player) {

	// 
	if (hitMoveTime_ > 0.0f) {
		transform_.translate.x += hitMoveVelocity_.x;
		transform_.translate.y += hitMoveVelocity_.y;
		hitMoveTime_ -= 1.0f / 60.0f;

		if (hitMoveTime_ <= 0.0f) {
			hitMoveVelocity_ = { 0, 0, 0 };
			ChangeDirection(); // ←方向を再設定すると自然に戻る
		}
	} else {
		moveChangeTimer_--;

		if (moveChangeTimer_ <= 0) {
			ChangeDirection();
			moveChangeTimer_ = rand() % 90 + 30;
		}

		transform_.translate.x += velocity_.x;
		transform_.translate.y += velocity_.y;

		if (transform_.translate.x > initialPosition_.x + moveRadius_ || transform_.translate.x < initialPosition_.x - moveRadius_) {
			velocity_.x = -velocity_.x;
		}
		if (transform_.translate.y > initialPosition_.y + moveRadius_ || transform_.translate.y < initialPosition_.y - moveRadius_) {
			velocity_.y = -velocity_.y;
		}
	}


	moveChangeTimer_--;

	// 一定時間ごとに方向変更
	if (moveChangeTimer_ <= 0) {
		ChangeDirection();
		moveChangeTimer_ = rand() % 90 + 30; // 60～180フレームに変更
	}

	// 移動
	transform_.translate.x += velocity_.x;
	transform_.translate.y += velocity_.y;

	// 画面外や移動範囲外に出たら方向を変える
	if (transform_.translate.x > initialPosition_.x + moveRadius_ || transform_.translate.x < initialPosition_.x - moveRadius_) {
		velocity_.x = -velocity_.x;
	}
	if (transform_.translate.y > initialPosition_.y + moveRadius_ || transform_.translate.y < initialPosition_.y - moveRadius_) {
		velocity_.y = -velocity_.y;
	}

	// 弾の発射タイマーを管理
	// 弾の発射タイマーを管理（HPが1のときだけ発射）
	if (hp_ == 1) {
		fireTimer_--;
		if (fireTimer_ <= 0) {
			FireBullet(player->GetTranslate());
			fireTimer_ = fireInterval_;  // タイマーをリセット
		}
	}

	// 弾の更新
	for (auto& bullet : bullets_) {
		bullet->Update();
	}

	bullets_.erase(
		std::remove_if(bullets_.begin(), bullets_.end(),
			[](const std::unique_ptr<EnemyBullet>& bullet) {
				return !bullet->IsActive();
			}),
		bullets_.end());

	object3d_->SetScale(transform_.scale);
	object3d_->SetRotate(transform_.rotate);
	object3d_->SetTranslate(transform_.translate);

	object3d_->Update();
}


void Enemy::Draw() {
	object3d_->Draw(dxCommon_);
	// 弾の描画
	for (const auto& bullet : bullets_) {
		bullet->Draw();
	}
}

void Enemy::SetCamera(Camera* camera) {
	object3d_->SetCamera(camera);
}

void Enemy::ChangeDirection()
{
	float angle = (rand() % 360) * (3.141592f / 180.0f);
	Vector3 newVelocity = { cos(angle) * moveSpeed_, sin(angle) * moveSpeed_, 0.0f };

	// 徐々に現在の速度に近づける
	velocity_.x = velocity_.x * 0.8f + newVelocity.x * 0.2f;
	velocity_.y = velocity_.y * 0.8f + newVelocity.y * 0.2f;
}

void Enemy::DecreaseHP(int amount)
{
	hp_ -= amount;
	if (hp_ <= 0) {
		isDead_ = true;
	}
}

void Enemy::ApplyHitReaction(const Vector3& sourcePosition, float speed)
{
	Vector3 diff = transform_.translate - sourcePosition;
	diff.z = 0.0f; // Z方向は無視

	Vector3 dir = MyMath::Normalize(diff);
	hitMoveVelocity_ = dir * speed;
	hitMoveTime_ = 0.5f; // 0.5秒間だけ滑るように移動
}



void Enemy::FireBullet(const Vector3& playerPos)
{
	Vector3 direction = MyMath::Normalize(playerPos - transform_.translate);
	float bulletSpeed = 0.8f;

	auto bullet = std::make_unique<EnemyBullet>();
	bullet->Initialize(transform_.translate, direction * bulletSpeed, obj3dCo_, dxCommon_);
	bullet->SetCamera(camera_);
	bullets_.push_back(std::move(bullet));
}

