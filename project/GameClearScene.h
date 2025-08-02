#pragma once
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "SrvManager.h"

class GameClearScene : public BaseScene {
public:
	GameClearScene(DirectXCommon* dxCommon, SrvManager* srvManager)
		: dxCommon(dxCommon), srvManager(srvManager) {
	}
	~GameClearScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	// UIなど必要に応じて
};
