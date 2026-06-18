#pragma once

#include "Sprite.h"
#include <memory>

class Road {
public:
	void Initialize(
		TKM::DirectXCommon* dxCommon,
		const Vector2& pos,
		bool isCorrect
	);

	void Update();
	void Draw();

	bool IsCorrect() const { return isCorrect_; }

	const Vector2& GetPosition() const { return position_; }
	const Vector2& GetSize() const { return size_; }

	void SetHit(bool isHit) { isHit_ = isHit; }
	bool IsHit() const { return isHit_; }

private:
	std::unique_ptr<TKM::Sprite> sprite_;

	Vector2 position_;
	Vector2 size_ = { 100.0f, 100.0f };

	bool isCorrect_ = false;
	bool isHit_ = false;
};