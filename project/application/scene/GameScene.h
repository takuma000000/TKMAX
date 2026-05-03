#pragma once
#include "BaseScene.h"
#include <memory>
#include "ActionPlayer.h"
#include "ActionEnemy.h"
#include "ActionGoal.h"
#include "ActionTimer.h"
#include "ActionLifeUI.h"
#include "ActionGround.h"

//=============================================================
// GameSceneクラス
// ゲーム本編を管理するシーンクラス。
//=============================================================
class GameScene : public TKM::BaseScene {
public:
	GameScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager)
		: dxCommon_(dxCommon), srvManager_(srvManager) {
	}
	~GameScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;
	void Draw3D() override;
	void DrawSprite() override;
	void DrawBack() override;

private:
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;

	std::unique_ptr<ActionPlayer> player_ = nullptr;
	std::unique_ptr<ActionEnemy> enemy_ = nullptr;
	std::unique_ptr<ActionGoal> goal_ = nullptr;
	std::unique_ptr<ActionTimer> timer_ = nullptr;
	std::unique_ptr<ActionLifeUI> lifeUI_ = nullptr;
	std::unique_ptr<ActionGround> ground_ = nullptr;
};