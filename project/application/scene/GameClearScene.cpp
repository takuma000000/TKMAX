#include "GameClearScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void GameClearScene::Initialize() {

	const std::string texturePath = "./resources/texture/clear.png";
	TKM::TextureManager::GetInstance()->LoadTexture(texturePath);

	clearSprite_ = std::make_unique<TKM::Sprite>();

	clearSprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		texturePath
	);

	clearSprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath);

	Vector2 texSize = {
		(float)meta.width,
		(float)meta.height
	};

	clearSprite_->SetTextureLeftTop({ 0,0 });
	clearSprite_->SetTextureSize(texSize);

	// 画面中央
	clearSprite_->SetPosition({
		(1280.0f - texSize.x) * 0.5f,
		(720.0f - texSize.y) * 0.5f
		});

	clearSprite_->SetSize(texSize);
}

void GameClearScene::Finalize() {
}

void GameClearScene::Update() {
	TKM::Input::GetInstance()->Update();

	clearSprite_->Update();

	if (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager_->ChangeScene("TITLE");
	}
}

void GameClearScene::Draw() {
	DrawSprite();
}

void GameClearScene::Draw3D() {
}

void GameClearScene::DrawSprite() {
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	if (clearSprite_) {
		clearSprite_->Draw();
	}
}

void GameClearScene::DrawBack() {
}