#pragma once
#include "BaseScene.h"
#include <memory>
#include <vector>
#include "ActionPlayer.h"
#include "ActionEnemy.h"
#include "ActionGoal.h"
#include "ActionTimer.h"
#include "ActionLifeUI.h"
#include "ActionGround.h"
#include "ActionPlayerBulletManager.h"
#include "ActionPlayerMagic.h"
#include "ActionBack.h"
#include "ActionBlock.h"

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
	void ResolvePlayerBlockCollision(const Vector2& prevPlayerPos);
	void ResolveEnemyBlockCollision(ActionEnemy& enemy, const Vector2& prevEnemyPos);

	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;

	std::unique_ptr<ActionPlayer> player_ = nullptr;
	std::vector<std::unique_ptr<ActionEnemy>> enemies_;
	std::unique_ptr<ActionGoal> goal_ = nullptr;
	std::unique_ptr<ActionTimer> timer_ = nullptr;
	std::unique_ptr<ActionLifeUI> lifeUI_ = nullptr;
	std::unique_ptr<ActionGround> ground_ = nullptr;
	std::unique_ptr<ActionPlayerBulletManager> bulletManager_ = nullptr;
	std::unique_ptr<ActionPlayerMagic> magic_ = nullptr;
	std::unique_ptr<ActionBack> back_ = nullptr;
	std::vector<std::unique_ptr<ActionBlock>> blocks_;

	float scrollX_ = 0.0f;

	static constexpr float kScreenWidth_ = 1280.0f;
	static constexpr float kGroundWidth_ = 1280.0f;
	static constexpr int kGroundCount_ = 3;
	static constexpr float kStageWidth_ = kGroundWidth_ * kGroundCount_;

	float dt_ = 1.0f / 60.0f;
};