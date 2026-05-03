#pragma once
#include "BaseScene.h"

#include <memory>
#include <cmath>
#include <vector>
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "CameraManager.h"
#include "Input.h"
#include "SceneManager.h"
#include <SkyBox.h> 
#include "WaterRippleEffect.h"
#include "TitleMenuController.h"
#include "Enemy.h"
#include <random>
#include "Player.h"
#include "BossEnemy.h"
#include "StateMachine.h"
#include "GameOverScene.h"
#include "GameScene.h"
#include "GameClearScene.h"
#include "TitleShowdownController.h"

//=============================================================
// TitleSceneクラス
// タイトル画面を管理するシーンクラス。
//=============================================================
class TitleScene : public TKM::BaseScene, public TKM::IStateContext {
public:
	TitleScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager) : dxCommon_(dxCommon), srvManager_(srvManager) {}

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
	TKM::Camera* camera_ = nullptr; // 今フレームのアクティブカメラ（CameraManagerから取得）
};