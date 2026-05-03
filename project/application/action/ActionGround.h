#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include <array>

//=============================================================
// ActionGround
// 地面Spriteを表示するクラス
//=============================================================
class ActionGround {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw(float scrollX);

	float GetTopY() const { return position_.y; }

private:
	static constexpr int kGroundCount_ = 3;

	std::array<std::unique_ptr<TKM::Sprite>, kGroundCount_> sprites_;

	const std::string texturePath_ = "./resources/texture/ground.png";

	Vector2 position_ = { 0.0f, 656.0f };

	static constexpr float kGroundWidth_ = 1280.0f;
	static constexpr float kGroundHeight_ = 64.0f;
};