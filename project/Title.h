#pragma once
#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"

class Title
{
public:

	~Title();

	void Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon);
	void Update();
	void Draw();

private:
	Sprite* crushRingSprite = nullptr;
};

