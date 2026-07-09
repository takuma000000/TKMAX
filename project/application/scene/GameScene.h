#pragma once
#include "BaseScene.h"
#include <memory>
#include <vector>

#include "Player.h"
#include "Road.h"
#include "HintLog.h"
#include "HintClient.h"

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

	HintLog hintLog_;
	HintClient hintClient_;

	std::unique_ptr<Player> player_;
	std::vector<std::unique_ptr<Road>> roads_;

	int selectedRoadIndex_ = -1;
	bool isSelectedCorrect_ = false;

	int hitRoadIndex_ = -1;

	bool CheckAABB(
		const Vector2& posA,
		const Vector2& sizeA,
		const Vector2& posB,
		const Vector2& sizeB
	);
};