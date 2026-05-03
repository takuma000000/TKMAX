#pragma once
#include <memory>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

class ActionEnemy {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw();

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
	}

private:
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;

	Vector2 position_ = { 600.0f, 500.0f };

	static constexpr float kEnemyWidth_ = 48.0f;
	static constexpr float kEnemyHeight_ = 48.0f;
};