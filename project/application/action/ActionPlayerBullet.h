#pragma once
#include <memory>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

//=============================================================
// ActionPlayerBullet
// プレイヤーが発射する弾1発分を管理するクラス
//=============================================================
class ActionPlayerBullet {
public:
	void Initialize(TKM::DirectXCommon* dxCommon, const Vector2& position, float direction);
	void Update();
	void Draw(float scrollX);

	void Kill();

	bool IsDead() const { return isDead_; }
	AABB GetAABB() const;

private:
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;

	Vector2 position_ = { 0.0f, 0.0f };
	Vector2 velocity_ = { 12.0f, 0.0f };

	bool isDead_ = false;

	static constexpr float kBulletWidth_ = 20.0f;
	static constexpr float kBulletHeight_ = 12.0f;

	static constexpr float kDeadX_ = 4000.0f;
};