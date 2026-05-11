#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TextureManager.h"

void GameScene::Initialize() {
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/circle2.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/goal.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/gradationLine.png");

	back_ = std::make_unique<ActionBack>();
	back_->Initialize(dxCommon_);

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

	magic_ = std::make_unique<ActionPlayerMagic>();
	magic_->Initialize(dxCommon_);

	blocks_.clear();

	auto addBlock = [&](float x, float y) {
		auto block = std::make_unique<ActionBlock>();
		block->Initialize(dxCommon_, { x, y });
		blocks_.push_back(std::move(block));
		};

	// 適当に配置
	addBlock(500, 610);
	addBlock(524, 610);
	addBlock(548, 610);

	addBlock(1000, 550);
	addBlock(1024, 550);

	addBlock(2000, 500);
}

void GameScene::Finalize() {
	player_.reset();
	enemies_.clear();
	goal_.reset();
	timer_.reset();
	lifeUI_.reset();
	ground_.reset();
	bulletManager_.reset();
	magic_.reset();
	back_.reset();
}

void GameScene::Update() {
	TKM::Input::GetInstance()->Update();

	magic_->Update(
		player_->GetPosition(),
		player_->GetSize(),
		player_->GetFacingDirection(),
		scrollX_,
		kScreenWidth_,
		enemies_
	);
	player_->SetControlLocked(magic_->IsPlayerControlLocked());

	//=============================================================
	// 地面を一旦通常地面に戻す
	//=============================================================
	player_->SetGroundTopY(ground_->GetTopY());

	Vector2 prevPlayerPos = player_->GetPosition();

	player_->Update();

	ResolvePlayerBlockCollision(prevPlayerPos);

	bulletManager_->Update(
		player_->GetPosition(),
		player_->GetSize(),
		player_->GetFacingDirection(),
		enemies_,
		blocks_,
		scrollX_,
		kScreenWidth_
	);

	player_->ImGuiDebug();

	goal_->Update();

	timer_->Update();

	ground_->Update();

	back_->Update();

	lifeUI_->Update(player_->GetHP());

	bool isTimeUp = timer_->IsTimeUp();

	for (auto& enemy : enemies_) {
		enemy->Update();

		//=============================================================	
		// プレイヤーと敵の当たり判定
		// （死亡中は無効にする）
		//=============================================================
		if (!enemy->IsDying() &&
			!enemy->IsMagicLocked() &&
			!enemy->IsMagicVanishing() &&
			player_->GetAABB().IsCollidingWithAABB(enemy->GetAABB())) {
			player_->TakeDamage();
		}
	}

	if (!magic_->IsPlayerControlLocked()) {
		scrollX_ = player_->GetPosition().x - kScreenWidth_ * 0.5f;

		if (scrollX_ < 0.0f) {
			scrollX_ = 0.0f;
		}

		float maxScrollX = kStageWidth_ - kScreenWidth_;

		if (scrollX_ > maxScrollX) {
			scrollX_ = maxScrollX;
		}
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

	if (back_) {
		back_->Draw(scrollX_);
	}
	if (ground_) {
		ground_->Draw(scrollX_);
	}
	for (auto& block : blocks_) {
		block->Draw(scrollX_);
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
	if (magic_) {
		magic_->Draw(scrollX_);
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

void GameScene::ResolvePlayerBlockCollision(const Vector2& prevPlayerPos) {
	const Vector2 playerSize = player_->GetSize();
	const Vector2 currentPlayerPos = player_->GetPosition();

	AABB prevPlayerAABB(
		{
			prevPlayerPos.x + playerSize.x * 0.5f,
			prevPlayerPos.y + playerSize.y * 0.5f,
			0.0f
		},
		{
			playerSize.x,
			playerSize.y,
			1.0f
		}
	);

	for (auto& block : blocks_) {
		AABB playerAABB = player_->GetAABB();
		AABB blockAABB = block->GetAABB();

		if (!playerAABB.IsCollidingWithAABB(blockAABB)) {
			continue;
		}

		Vector3 pCenter = playerAABB.GetCenter();
		Vector3 bCenter = blockAABB.GetCenter();

		Vector3 pHalf = playerAABB.GetHalfSize();
		Vector3 bHalf = blockAABB.GetHalfSize();

		Vector3 prevCenter = prevPlayerAABB.GetCenter();
		Vector3 prevHalf = prevPlayerAABB.GetHalfSize();

		const float playerLeft = pCenter.x - pHalf.x;
		const float playerRight = pCenter.x + pHalf.x;
		const float playerTop = pCenter.y - pHalf.y;
		const float playerBottom = pCenter.y + pHalf.y;

		const float blockLeft = bCenter.x - bHalf.x;
		const float blockRight = bCenter.x + bHalf.x;
		const float blockTop = bCenter.y - bHalf.y;
		const float blockBottom = bCenter.y + bHalf.y;

		const float prevPlayerLeft = prevCenter.x - prevHalf.x;
		const float prevPlayerRight = prevCenter.x + prevHalf.x;
		const float prevPlayerTop = prevCenter.y - prevHalf.y;
		const float prevPlayerBottom = prevCenter.y + prevHalf.y;

		const bool wasAbove = prevPlayerBottom <= blockTop;
		const bool wasBelow = prevPlayerTop >= blockBottom;
		const bool wasLeft = prevPlayerRight <= blockLeft;
		const bool wasRight = prevPlayerLeft >= blockRight;

		if (wasAbove && playerBottom >= blockTop) {
			player_->LandOnTop(blockTop);
			continue;
		}

		if (wasBelow && playerTop <= blockBottom) {
			player_->HitHead(blockBottom);
			continue;
		}

		if (wasLeft && playerRight >= blockLeft) {
			player_->PushOutLeft(blockLeft);
			continue;
		}

		if (wasRight && playerLeft <= blockRight) {
			player_->PushOutRight(blockRight);
			continue;
		}
	}
}