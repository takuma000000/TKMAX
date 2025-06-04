#pragma once
#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "EnemyBullet.h"

class Player;

class Enemy {
public:
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, const Vector3& position);
	void Update(Player* player);
	void Draw();

	// スケール、回転、位置の設定
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

	const Vector3& GetScale() const { return transform_.scale; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }

	void SetCamera(Camera* camera);

	bool IsDead() const { return isDead_; }
	void OnCollision() { isDead_ = true; }

	// 弾の取得
	const Vector3& GetPosition() const { return transform_.translate; }

	void ChangeDirection();

	// 敵の弾のリストを取得
	const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return bullets_; }


private:
	Transform transform_;

	std::unique_ptr<Object3d> object3d_ = nullptr;

	DirectXCommon* dxCommon_ = nullptr;
	Object3dCommon* obj3dCo_ = nullptr;
	Camera* camera_ = nullptr;

	bool isDead_ = false;

private:
	int hp_ = 3; // ★追加

	Vector3 hitMoveVelocity_ = { 0, 0, 0 };
	float hitMoveTime_ = 0.0f;

public:
	void DecreaseHP(int amount = 1); // ★追加
	void ApplyHitReaction(const Vector3& sourcePosition, float speed);



private:
	Vector3 velocity_;  // 速度ベクトル
	Vector3 initialPosition_; // 初期位置を保持
	float moveRadius_ = 3.0f; // 移動範囲の制限
	float moveSpeed_ = 0.25f; // 移動速度
	int moveChangeTimer_ = 0; // 方向変更用のタイマー

	std::vector<std::unique_ptr<EnemyBullet>> bullets_;
	int fireTimer_ = 0; // 発射タイマー
	const int fireInterval_ = 120; // 弾の発射間隔
	
	void FireBullet(const Vector3& playerPos);// 弾の発射

};
