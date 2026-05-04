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
// 敵の死亡タイプ
// 敵が死ぬときの演出の種類
// 例えば、ノックバックで吹き飛ぶ、魔法の爆発で消えるなど
//=============================================================
enum class EnemyDeathType {
	None,
	Knockback,
	MagicExplosion
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

	bool IsMagicLocked() const { return isMagicLocked_; }

	void SetMagicLocked(bool locked) { isMagicLocked_ = locked; }

	bool IsDead() const { return isDead_; }

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

	void StartKnockbackDeath(float hitDirection);
	void StartMagicExplosionDeath();

	bool IsDying() const { return deathType_ != EnemyDeathType::None; }
	bool IsMagicVanishing() const { return deathType_ == EnemyDeathType::MagicExplosion; }

private:
	std::string GetTexturePathByType_(EnemyType type);

	void UpdateNormal_();
	void UpdateKnockbackDeath_();
	void UpdateMagicExplosionDeath_();

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

	Vector2 knockbackVelocity_ = { 0.0f, 0.0f };

	static constexpr float kKnockbackSpeedX_ = 8.0f;
	static constexpr float kKnockbackSpeedY_ = -10.0f;
	static constexpr float kKnockbackGravity_ = 0.6f;
	static constexpr float kDeadBottomY_ = 900.0f;
	bool isMagicLocked_ = false;

	static constexpr float kMagicVanishDuration_ = 0.35f;
	static constexpr float kFrameTime_ = 1.0f / 60.0f;

	EnemyDeathType deathType_ = EnemyDeathType::None;
	float deathTimer_ = 0.0f;
};