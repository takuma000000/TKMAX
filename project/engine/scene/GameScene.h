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
#include "DirectionalLight.h"
#include "engine/func/math/Vector3.h"
#include "Input.h"


#include "Player.h"
#include "Enemy.h"

class GameScene : public BaseScene
{
public:
	GameScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}
	~GameScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	void InitializeAudio();
	void LoadTextures();
	void InitializeSprite();
	void LoadModels();
	void InitializeObjects();
	void InitializeCamera();
	void UpdateObjectTransform(std::unique_ptr<Object3d>& obj, const Vector3& translate, const Vector3& rotate, const Vector3& scale);
	void UpdateMemory();
	void ImGui();

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	std::unique_ptr<Sprite> sprite = nullptr;
	std::unique_ptr<Camera> camera = nullptr;
	std::unique_ptr<Object3d> object3d = nullptr;
	std::unique_ptr<Object3d> anotherObject3d = nullptr;
	std::unique_ptr<Object3d> ground_ = nullptr;
	std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;

	//=========================================================
	// Player
	std::unique_ptr<Player> player_ = nullptr;
	std::unique_ptr<Input> input_ = nullptr;
	Object3dCommon* object3dCommon_ = nullptr;
	SpriteCommon* spriteCommon_ = nullptr;
	std::vector<std::unique_ptr<Sprite>> bulletSprites_;
	std::vector<std::unique_ptr<Sprite>> heartSprites_;
	//=========================================================
	// Enemy
	std::vector<std::unique_ptr<Enemy>> enemies_;
	//=========================================================

	static const int kMemoryHistorySize = 100;
	std::array<float, kMemoryHistorySize> memoryHistory_{};
	int memoryHistoryIndex_ = 0;
};
