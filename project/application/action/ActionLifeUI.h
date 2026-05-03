#pragma once
#include <array>
#include <memory>
#include <string>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"

//=============================================================
// ActionLifeUI
// プレイヤーHPをハートSpriteで表示するクラス
//=============================================================
class ActionLifeUI {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update(int currentHP);
	void Draw();

private:
	static constexpr int kMaxLife_ = 3;

	static constexpr float kLifeWidth_ = 40.0f;
	static constexpr float kLifeHeight_ = 40.0f;
	static constexpr float kLifeSpacing_ = 8.0f;

	const std::string lifeTexturePath_ = "./resources/texture/life.png";
	const std::string lifeOutTexturePath_ = "./resources/texture/life_out.png";

	Vector2 basePosition_ = { 24.0f, 640.0f };

	std::array<std::unique_ptr<TKM::Sprite>, kMaxLife_> lifeSprites_;
	std::array<std::unique_ptr<TKM::Sprite>, kMaxLife_> lifeOutSprites_;
};