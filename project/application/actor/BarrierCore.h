#pragma once
#include <memory>
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "BaseScene.h"
#include "MyMath.h"
#include "LineRenderer.h"

class BarrierCore {
public:
	BarrierCore() = default;
	~BarrierCore() = default;

	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	void Update(float dt);
	void Draw(TKM::DirectXCommon* dxCommon);

	void ImGuiDebug();

	/// <summary>
	/// ダメージを受けたときの処理。HPが0以下になった場合、死亡状態に移行します。
	/// </summary>
	/// <param name="damage">受けるダメージ量</param>
	void OnHitWithDamage(int damage);
	bool IsDead() const { return isDead_; }
	bool IsDying() const { return isDying_; }

	// Getter=========================================
	Vector3 GetWorldPosition() const;
	Vector3 GetColliderScale() const { return colliderScale_; }
	Vector3 GetScale() const { return baseScale_; }
	int GetHP() const { return hp_; }
	// ===============================================
	// Setter=========================================
	void SetCamera(TKM::Camera* camera);
	void SetParentScene(TKM::BaseScene* scene);
	void SetPosition(const Vector3& pos);
	void SetScale(const Vector3& scale);
	void SetColliderScale(const Vector3& scale);
	void SetModel(const std::string& modelName);
	void SetHP(int hp);
	// ===============================================

	/// <summary>
	/// オブジェクトのスケール、回転、平行移動をカメラに同期させます。
	/// </summary>
	void SyncTransform();

private:
	std::unique_ptr<TKM::Object3d> object_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	TKM::BaseScene* parentScene_ = nullptr;

	int hp_ = 3;
	int maxHP_ = 3;

	bool isDead_ = false;
	bool isDying_ = false;
	bool defeated_ = false;

	float deathTimer_ = 0.0f;
	float deathDuration_ = 0.35f;
	float deathAlpha_ = 1.0f;

	Vector3 colliderScale_ = { 2.5f, 2.5f, 2.5f };
	Vector3 baseScale_ = { 1.8f, 1.8f, 1.8f };
};