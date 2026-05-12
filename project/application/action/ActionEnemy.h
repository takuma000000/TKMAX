#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"
#include <vector>
#include <random>
#include <cmath>

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

	bool IsHitAttack(const AABB& playerAABB);

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

	void SetActionOffset(float actionOffset);

	void StartKnockbackDeath(float hitDirection);
	void StartMagicExplosionDeath(float scrollX);

	bool IsDying() const { return deathType_ != EnemyDeathType::None; }
	bool IsMagicVanishing() const { return deathType_ == EnemyDeathType::MagicExplosion; }

	void SetPosition(const Vector2& position) { position_ = position; }
	void ReverseDirection() { direction_ *= -1.0f; }

	void SetTargetPosition(const Vector2& targetPosition) { targetPosition_ = targetPosition; }

private:
	std::string GetTexturePathByType_(EnemyType type);

	void UpdateNormal_();
	void UpdateKnockbackDeath_();
	void UpdateMagicExplosionDeath_();

	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;
	std::unique_ptr<TKM::Sprite> dropSprite_ = nullptr;
	std::string dropTexturePath_;

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

	struct DropObject {
		Vector2 position = { 0.0f, 0.0f };
		Vector2 velocity = { 0.0f, 0.0f };
		float lifeTimer = 0.0f;
		bool isActive = false;
	};

	void UpdateTypeB_();
	void UpdateTypeC_();
	void UpdateDropObjects_();
	void SpawnDropObject_();
	void SpawnTypeCBullet_();

	AABB GetDropObjectAABB_(const DropObject& drop) const;

	std::vector<DropObject> dropObjects_;

	float floatTimer_ = 0.0f;
	float dropTimer_ = 0.0f;
	int dropIndex_ = 0;

	static constexpr float kTypeBFloatRange_ = 12.0f;
	static constexpr float kTypeBFloatSpeed_ = 0.05f;

	static constexpr float kDropInterval_ = 1.2f;
	static constexpr float kDropSize_ = 32.0f;
	static constexpr float kDropGravity_ = 0.45f;
	static constexpr float kDropThrowSpeedX_ = 3.5f;
	static constexpr float kDropThrowSpeedY_ = -2.5f;
	static constexpr float kDropBottomY_ = 900.0f;
	static constexpr int kDropMax_ = 8;
	static constexpr float kDropRange_ = 160.0f;

	float typeARandomTimer_ = 0.0f;
	float typeARandomInterval_ = 0.0f;
	float baseMoveSpeed_ = 2.0f;
	std::mt19937 randomEngine_;

	static constexpr float kTypeCBulletInterval_ = 1.6f;
	static constexpr float kTypeCBulletSpeed_ = 4.2f;
	static constexpr float kTypeCBulletLifetime_ = 2.0f;
	Vector2 targetPosition_ = { 0.0f, 0.0f };
};