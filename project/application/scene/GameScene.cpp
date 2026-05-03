#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TextureManager.h"

void GameScene::Initialize() {
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/circle2.png");

	ground_ = std::make_unique<ActionGround>();
	ground_->Initialize(dxCommon_);

	player_ = std::make_unique<ActionPlayer>();
	player_->Initialize(dxCommon_);
	player_->SetGroundTopY(ground_->GetTopY());

	enemy_ = std::make_unique<ActionEnemy>();
	enemy_->Initialize(dxCommon_);
	enemy_->SetGroundTopY(ground_->GetTopY());

	goal_ = std::make_unique<ActionGoal>();
	goal_->Initialize(dxCommon_);
	goal_->SetGroundTopY(ground_->GetTopY());

	timer_ = std::make_unique<ActionTimer>();
	timer_->Initialize(dxCommon_, 100);

	lifeUI_ = std::make_unique<ActionLifeUI>();
	lifeUI_->Initialize(dxCommon_);
}

void GameScene::Finalize() {
	player_.reset();
	enemy_.reset();
	goal_.reset();
	timer_.reset();
	lifeUI_.reset();
	ground_.reset();
}

void GameScene::Update() {
	TKM::Input::GetInstance()->Update();

	player_->Update();
	player_->ImGuiDebug();

	enemy_->Update();

	goal_->Update();

	timer_->Update();

	ground_->Update();

	lifeUI_->Update(player_->GetHP());

	bool isTimeUp = timer_->IsTimeUp();

	bool isHitEnemy = player_->GetAABB().IsCollidingWithAABB(enemy_->GetAABB());
	if (isHitEnemy) {
		player_->TakeDamage();
	}

	if (isTimeUp || player_->IsDead()) {
		sceneManager_->ChangeScene("GAMEOVER");
		return;
	}

	if (player_->GetAABB().IsCollidingWithAABB(goal_->GetAABB())) {
		sceneManager_->ChangeScene("CLEAR");
	}
}

void GameScene::Draw() {
	DrawSprite();
}

void GameScene::Draw3D() {
}

void GameScene::DrawSprite() {
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	if (ground_) {
		ground_->Draw();
	}
	if (player_) {
		player_->Draw();
	}
	if (enemy_) {
		enemy_->Draw();
	}
	if (goal_) {
		goal_->Draw();
	}
	if (timer_) {
		timer_->Draw();
	}
	if (lifeUI_) {
		lifeUI_->Draw();
	}
}

void GameScene::DrawBack() {
}