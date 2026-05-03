#pragma once
#include <memory>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

//=============================================================
// ActionGoal
// 2Dアクション用ゴール
//=============================================================
class ActionGoal {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw(float scrollX);

	void SetPosition(const Vector2& position) {
		position_ = position;
	}

	const Vector2& GetPosition() const { return position_; }
	Vector2 GetSize() const { return { kGoalWidth_, kGoalHeight_ }; }

	AABB GetAABB() const {
		return AABB(
			{
				position_.x + kGoalWidth_ * 0.5f,
				position_.y + kGoalHeight_ * 0.5f,
				0.0f
			},
			{
				kGoalWidth_,
				kGoalHeight_,
				1.0f
			}
		);
	}
	void SetGroundTopY(float groundTopY) {
		position_.y = groundTopY - kGoalHeight_;
	}
private:
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;

	Vector2 position_ = { 900.0f, 500.0f };

	static constexpr float kGoalWidth_ = 48.0f;
	static constexpr float kGoalHeight_ = 48.0f;
};