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
	player_->SetStageWidth(kStageWidth_);

	bulletManager_ = std::make_unique<ActionPlayerBulletManager>();
	bulletManager_->Initialize(dxCommon_);

	enemies_.clear();
	enemies_.push_back(std::make_unique<ActionEnemy>());
	enemies_.back()->Initialize(
		dxCommon_,
		{ 700.0f, 0.0f },
		EnemyType::TypeA,
		120.0f,
		2.0f
	);
	enemies_.back()->SetGroundTopY(ground_->GetTopY());

	enemies_.push_back(std::make_unique<ActionEnemy>());
	enemies_.back()->Initialize(
		dxCommon_,
		{ 1500.0f, 0.0f },
		EnemyType::TypeA,
		160.0f,
		1.5f
	);
	enemies_.back()->SetGroundTopY(ground_->GetTopY());

	enemies_.push_back(std::make_unique<ActionEnemy>());
	enemies_.back()->Initialize(
		dxCommon_,
		{ 2800.0f, 0.0f },
		EnemyType::TypeA,
		100.0f,
		2.5f
	);
	enemies_.back()->SetGroundTopY(ground_->GetTopY());

	goal_ = std::make_unique<ActionGoal>();
	goal_->Initialize(dxCommon_);
	goal_->SetGroundTopY(ground_->GetTopY());
	goal_->SetPosition({ 3600.0f, goal_->GetPosition().y });

	timer_ = std::make_unique<ActionTimer>();
	timer_->Initialize(dxCommon_, 100);

	lifeUI_ = std::make_unique<ActionLifeUI>();
	lifeUI_->Initialize(dxCommon_);
}

void GameScene::Finalize() {
	player_.reset();
	enemies_.clear();
	goal_.reset();
	timer_.reset();
	lifeUI_.reset();
	ground_.reset();
	bulletManager_.reset();
}

void GameScene::Update() {
	TKM::Input::GetInstance()->Update();

	player_->Update();
	bulletManager_->Update(
		player_->GetPosition(),
		player_->GetSize(),
		player_->GetFacingDirection(),
		enemies_
	);
	player_->ImGuiDebug();

	goal_->Update();

	timer_->Update();

	ground_->Update();

	lifeUI_->Update(player_->GetHP());

	bool isTimeUp = timer_->IsTimeUp();

	for (auto& enemy : enemies_) {
		enemy->Update();

		if (player_->GetAABB().IsCollidingWithAABB(enemy->GetAABB())) {
			player_->TakeDamage();
		}
	}


	scrollX_ = player_->GetPosition().x - kScreenWidth_ * 0.5f;
	if (scrollX_ < 0.0f) {
		scrollX_ = 0.0f;
	}
	float maxScrollX = kStageWidth_ - kScreenWidth_;
	if (scrollX_ > maxScrollX) {
		scrollX_ = maxScrollX;
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
		ground_->Draw(scrollX_);
	}
	if (player_) {
		player_->Draw(scrollX_);
	}
	if (bulletManager_) {
		bulletManager_->Draw(scrollX_);
	}
	for (auto& enemy : enemies_) {
		enemy->Draw(scrollX_);
	}
	if (goal_) {
		goal_->Draw(scrollX_);
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