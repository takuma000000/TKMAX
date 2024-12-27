#include "TextureManager.h"
#include "Title.h"

Title::~Title()
{
	delete crushRingSprite;
}

void Title::Initialize(SpriteCommon* spriteCommon, DirectXCommon* dxCommon) {
	crushRingSprite = new Sprite();
	crushRingSprite->Initialize(spriteCommon, dxCommon, "./resources/crushring.png");
	crushRingSprite->SetPosition({ 0.0f, 0.0f }); // 表示位置を調整
}

void Title::Update() {
	if (crushRingSprite) {
		crushRingSprite->Update();
	}
}

void Title::Draw() {
	if (crushRingSprite) {
		crushRingSprite->Draw();
	}
}
