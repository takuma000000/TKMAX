#pragma once
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "MyMath.h"

//=============================================================
// EnemyBulletクラス
// 敵が発射する弾を管理するクラス。
//=============================================================
class EnemyBullet {
public:
	EnemyBullet() = default;
	~EnemyBullet() = default;

	/// <summary>
	/// 敵弾を初期化します。
	/// </summary>
	void Initialize(
		TKM::Object3dCommon* common,
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera,
		const Vector3& position,
		const Vector3& velocity
	);
	/// <summary>
	/// 敵弾を更新します。
	/// </summary>
	void Update(float dt);
	/// <summary>
	/// 敵弾を描画します。
	/// </summary>
	void Draw(TKM::DirectXCommon* dx);

	/// <summary>
	/// 敵弾が死亡しているかを返します。
	/// </summary>
	bool IsDead() const { return isDead_; }

	// Getter===================================
	/// <summary>
	/// 敵弾のワールド座標を返します。
	/// </summary>
	Vector3 GetWorldPosition() const;
	/// <summary>
	/// 当たり判定半径を返します。
	/// </summary>
	float GetRadius() const { return radius_; }
	// =========================================

private:
	std::unique_ptr<TKM::Object3d> object_ = nullptr;
	TKM::Camera* camera_ = nullptr;

	Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };

	float radius_ = 0.8f;
	float lifeTimer_ = 0.0f;
	float lifeTime_ = 4.0f;

	bool isDead_ = false;
};