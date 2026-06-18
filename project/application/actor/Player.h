#pragma once
#include "Sprite.h"
#include "DirectXCommon.h"
#include <memory>

class Player {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw();

	const Vector2& GetPosition() const { return position_; }
	Vector2 GetSize() const { return { 64.0f, 64.0f }; }

private:
	std::unique_ptr<TKM::Sprite> sprite_;
	Vector2 position_ = { 100.0f, 300.0f };
	float speed_ = 5.0f;
};