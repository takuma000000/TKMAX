#pragma once
#include "BaseScene.h"

//=============================================================
// GameClearSceneクラス
// ゲームクリア画面を管理するシーンクラス。
// 背景スカイボックス回転＋自機のジェットコースター演出。
//=============================================================
class GameClearScene : public TKM::BaseScene{
public:
	GameClearScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
		: dxCommon_(dxCommon), srvManager_(srvManager) {
	}
	~GameClearScene() = default;

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