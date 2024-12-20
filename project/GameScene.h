#pragma once
#include <memory>
#include "engine/audio/AudioManager.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "srvManager.h"
#include "engine/2d/Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"

class GameScene
{
public:
	GameScene(DirectXCommon* dxCommon, SrvManager* srvManager)
		: dxCommon(dxCommon), srvManager(srvManager) {
	}

	void Initialize();
	void Finalize();
	void Update();
	void Draw();
private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	std::unique_ptr<Sprite> sprite = nullptr;
	std::unique_ptr<Camera> camera = nullptr;

	Object3d* object3d = new Object3d();
	Object3d* anotherObject3d = new Object3d();
};

