#pragma once
#include <memory>
#include "Object3d.h"
#include "Vector3.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "Sprite.h"
#include "Camera.h"
#include "Input.h"
#include <windows.h>

#include <vector>
//#include "PlayerBullet.h"
//#include "Enemy.h"

class Player {
public:
	void Initialize(Object3dCommon* object3dCommon, DirectXCommon* dxCommon, Camera* camera, Input* input, SpriteCommon* spriteCommon);
	void Update(const std::vector<Enemy*>& enemies);
	void Draw();

	void FireBullet();
	Vector3 GetTargetDirection() const;

	void OnCollision();
	void SetScale(const Vector3& scale) { transform_.scale = scale; }
	void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
	void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

	const Vector3& GetScale() const { return transform_.scale; }
	const Vector3& GetRotate() const { return transform_.rotate; }
	const Vector3& GetTranslate() const { return transform_.translate; }

	void SetCamera(Camera* camera);
	Vector3 ScreenToWorld(const POINT& screenPos) const;

	void DrawImGui();
	void SetSpriteCommon(SpriteCommon* spriteCommon);

	int GetBulletCount() const { return bulletCount_; }
	const std::vector<std::unique_ptr<PlayerBullet>>& GetBullets() const { return bullets_; }

	void ResetBulletCount() { bulletCount_ = 15; }
	bool IsOverTimerExpired() const { return overTimer_ == 0.0f; }

	void LockOnTarget(const std::vector<Enemy*>& enemies);
	Vector3 WorldToScreen(const Vector3& worldPos) const;

	bool IsInvincible() const { return isInvincible_; }
	bool IsHPOverTimerExpired() const { return hpOverTimer_ == 0.0f; }
	void ResetHP() { hp_ = 3; hpOverTimer_ = -1.0f; }
	bool IsDead() const { return hp_ <= 0; }
	bool IsHPOverTimerRunning() const { return hpOverTimer_ > 0.0f; }
	int GetHP() const { return hp_; }
	void DecreaseHP() { if (hp_ > 0) hp_--; }

private:
	Transform transform_;
	std::unique_ptr<Object3d> object3d_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;
	Object3dCommon* obj3dCo_ = nullptr;
	Camera* camera_ = nullptr;
	Input* input_ = nullptr;
	WindowsAPI* windo = nullptr;
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;
	std::unique_ptr<Sprite> mouseSprite_;
	SpriteCommon* spriteCommon_ = nullptr;
	int bulletCount_ = 15;
	float overTimer_ = -1.0f;
	float hpOverTimer_ = -1.0f;

	Enemy* targetEnemy_ = nullptr;
	std::unique_ptr<Sprite> lockOnMarker_;

	bool isInvincible_ = false;
	float invincibleTime_ = 0.0f;
	float blinkInterval_ = 0.2f;
	float blinkTimer_ = 0.0f;
	bool isVisible_ = true;
	int hp_ = 3;
};
