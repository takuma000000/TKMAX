#pragma once
#include <memory>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

class ActionBlock {
public:
	void Initialize(TKM::DirectXCommon* dxCommon, const Vector2& position);
	void Update();
	void Draw(float scrollX);

	AABB GetAABB() const {
		return AABB(
			{
				position_.x + kSize_ * 0.5f,
				position_.y + kSize_ * 0.5f,
				0.0f
			},
			{
				kSize_,
				kSize_,
				1.0f
			}
		);
	}

private:
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;

	Vector2 position_ = { 0,0 };

	static constexpr float kSize_ = 48.0f;
};