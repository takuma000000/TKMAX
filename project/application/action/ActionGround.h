#pragma once
#include <memory>
#include <string>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"

//=============================================================
// ActionGround
// 地面Spriteを表示するクラス
//=============================================================
class ActionGround {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw();

	float GetTopY() const { return position_.y; }

private:
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;

	const std::string texturePath_ = "./resources/texture/ground.png";

	Vector2 position_ = { 0.0f, 656.0f };

	static constexpr float kGroundWidth_ = 1280.0f;
	static constexpr float kGroundHeight_ = 64.0f;
};