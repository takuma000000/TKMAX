#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include "TextureManager.h"

void TitleScene::Initialize() {

	const std::string texturePath = "./resources/texture/title_back.png";
	TKM::TextureManager::GetInstance()->LoadTexture(texturePath);

	backSprite_ = std::make_unique<TKM::Sprite>();

	backSprite_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		texturePath
	);

	backSprite_->SetAutoAdjustTextureSize(false);

	const auto& meta = TKM::TextureManager::GetInstance()->GetMetadata(texturePath);

	Vector2 texSize = {
		(float)meta.width,
		(float)meta.height
	};

	const float kUvInset_ = 0.5f;

	backSprite_->SetTextureLeftTop({ kUvInset_, kUvInset_ });
	backSprite_->SetTextureSize({
		texSize.x - kUvInset_ * 2.0f,
		texSize.y - kUvInset_ * 2.0f
		});
	backSprite_->SetPosition({ 0.0f, 0.0f });
	backSprite_->SetSize({ 1280.0f, 720.0f });
}

void TitleScene::Finalize() {
}

void TitleScene::Update() {

	backSprite_->Update();

	TKM::Input::GetInstance()->Update();

	if (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		sceneManager_->ChangeScene("GAME");
	}
}

void TitleScene::Draw() {
}

void TitleScene::Draw3D() {
}

void TitleScene::DrawSprite() {

	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	if (backSprite_) {
		backSprite_->Draw();
	}
}

void TitleScene::DrawBack() {
}