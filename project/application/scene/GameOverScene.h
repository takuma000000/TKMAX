#pragma once
#include "BaseScene.h"
#include <memory>
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Camera.h"
#include "application/player/Player.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "engine/effect/light/DirectionalLight.h"
#include "engine/effect/particle/ParticleManager.h"

//=============================================================
// GameOverScene
//=============================================================
class GameOverScene : public BaseScene
{
public:
	GameOverScene(DirectXCommon* dxCommon, SrvManager* srvManager)
		: dxCommon_(dxCommon), srvManager_(srvManager) {
	}

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	std::unique_ptr<Camera> camera_;
	std::unique_ptr<Player> player_;
	std::unique_ptr<DirectionalLight> dirLight_;

	// GAME OVER の文字を出したいとき用
	//std::unique_ptr<Sprite> gameOverSprite_;
};
