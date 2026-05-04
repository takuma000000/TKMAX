#pragma once
#include <array>
#include <memory>
#include <string>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"

//=============================================================
// ActionBack
// 横スクロール用の背景Spriteを管理するクラス
//=============================================================
class ActionBack {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw(float scrollX);

private:
	static constexpr int kBackCount_ = 4;

	static constexpr float kBackWidth_ = 1280.0f;
	static constexpr float kBackHeight_ = 720.0f;

	static constexpr float kParallaxRate_ = 0.25f;

	const std::string texturePath_ = "./resources/texture/back.jpg";

	std::array<std::unique_ptr<TKM::Sprite>, kBackCount_> sprites_;
};