#pragma once
#include "BaseScene.h"

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
#include "Input.h"

#include "SceneManager.h"

class TitleScene : public BaseScene
{
public:
	TitleScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;
private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	std::unique_ptr<Sprite> sprite = nullptr;
	std::unique_ptr<Camera> camera = nullptr;

};

