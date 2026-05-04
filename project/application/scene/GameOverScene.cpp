#include "GameOverScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void GameOverScene::Initialize() {

	const std::string texturePath = "./resources/texture/over.png";
	TKM::TextureManager::GetInstance()->LoadTexture(texturePath);

	overSprite_ = std::make_unique<TKM::Sprite>();

	overSprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		texturePath
	);

	overSprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath);

	Vector2 texSize = {
		(float)meta.width,
		(float)meta.height
	};

	overSprite_->SetTextureLeftTop({ 0,0 });
	overSprite_->SetTextureSize(texSize);

	overSprite_->SetPosition({
		(1280.0f - texSize.x) * 0.5f,
		(720.0f - texSize.y) * 0.5f
		});

	overSprite_->SetSize(texSize);
}

void GameOverScene::Finalize() {
}

void GameOverScene::Update() {
	TKM::Input::GetInstance()->Update();

	overSprite_->Update();

	if (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager_->ChangeScene("TITLE");
	}
}

void GameOverScene::Draw() {
	DrawSprite();
}

void GameOverScene::Draw3D() {
}

void GameOverScene::DrawSprite() {
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	if (overSprite_) {
		overSprite_->Draw();
	}
}

void GameOverScene::DrawBack() {
}