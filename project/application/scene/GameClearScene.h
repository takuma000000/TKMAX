#pragma once
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "SrvManager.h"

//=============================================================
// GameClearSceneクラス
// ゲームクリア画面を管理するシーンクラス。
//=============================================================
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
