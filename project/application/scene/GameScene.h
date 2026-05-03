#pragma once
#include "BaseScene.h"


//=============================================================
// GameSceneクラス
// ゲーム本編を管理するシーンクラス。
//=============================================================
class GameScene : public TKM::BaseScene {
public:
	GameScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager) : dxCommon_(dxCommon), srvManager_(srvManager) {}
	~GameScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;
	void Draw3D() override;
	void DrawSprite() override;
	void DrawBack() override;

private:
	//======================================================================
	// 基本システム
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX共通（デバイス/コマンド等）
	TKM::SrvManager* srvManager_ = nullptr; // SRV管理
	//======================================================================
};