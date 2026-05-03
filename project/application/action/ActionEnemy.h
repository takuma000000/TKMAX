#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

//=============================================================
// 敵の種類
// 種類ごとに画像や挙動を変えられるようにする
//=============================================================
enum class EnemyType {
	TypeA, // 基本敵
	TypeB, // 後で追加する敵
	TypeC  // 後で追加する敵
};

//=============================================================
// ActionEnemy
// 左右に歩く2Dアクション用の敵
//=============================================================
class ActionEnemy {
public:
	void Initialize(
		TKM::DirectXCommon* dxCommon,
		const Vector2& position,
		EnemyType type,
		float moveRange,
		float moveSpeed
	);

	void Update();
	void Draw(float scrollX);

	void TakeDamage(float hitDirection);
	bool IsDead() const { return isDead_; }
	bool IsDying() const { return isDying_; }

	const Vector2& GetPosition() const { return position_; }
	Vector2 GetSize() const { return { kEnemyWidth_, kEnemyHeight_ }; }

	AABB GetAABB() const {
		return AABB(
			{
				position_.x + kEnemyWidth_ * 0.5f,
				position_.y + kEnemyHeight_ * 0.5f,
				0.0f
			},
			{
				kEnemyWidth_,
				kEnemyHeight_,
				1.0f
			}
		);
	}

	void SetGroundTopY(float groundTopY) {
		position_.y = groundTopY - kEnemyHeight_;
		basePosition_.y = position_.y;
	}

private:
	std::string GetTexturePathByType_(EnemyType type);

private:
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;

	Vector2 position_ = { 0.0f, 0.0f };
	Vector2 basePosition_ = { 0.0f, 0.0f };

	EnemyType type_ = EnemyType::TypeA;
	std::string texturePath_;

	float moveRange_ = 100.0f;
	float moveSpeed_ = 2.0f;
	float direction_ = 1.0f;

	static constexpr float kEnemyWidth_ = 48.0f;
	static constexpr float kEnemyHeight_ = 48.0f;

	bool isDead_ = false;
	bool isDying_ = false;

	Vector2 knockbackVelocity_ = { 0.0f, 0.0f };

	static constexpr float kKnockbackSpeedX_ = 8.0f;
	static constexpr float kKnockbackSpeedY_ = -10.0f;
	static constexpr float kKnockbackGravity_ = 0.6f;
	static constexpr float kDeadBottomY_ = 900.0f;
};