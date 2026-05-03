#pragma once
#include "BaseScene.h"

//=============================================================
// GameOverScene
// ゲームオーバー画面を管理するシーンクラス。
//=============================================================
class GameOverScene : public TKM::BaseScene {
public:
	GameOverScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
		: dxCommon_(dxCommon), srvManager_(srvManager) {
	}

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;
	void Draw3D() override;
	void DrawSprite() override;
	void DrawBack() override;

private:
	//======================================================================
	// システム参照
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;
};