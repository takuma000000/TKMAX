#include "GameScene.h"
#include "Input.h"
#include "SceneManager.h"
#include "TextureManager.h"

namespace {
	struct EnemySpawnData {
		EnemyType type;
		Vector2 position;
		float moveRange;
		float moveSpeed;
		bool useGroundTop;
		float actionOffset;
	};

	constexpr EnemySpawnData kEnemySpawns[] = {
		// 序盤：地上敵で基本の回避
		{
			EnemyType::TypeA,
			{ 700.0f, 0.0f },
			120.0f,
			2.0f,
			true,
			0.0f
		},

		// 中盤：空中から落下物を投げる敵
		{
			EnemyType::TypeB,
			{ 1500.0f, 320.0f },
			80.0f,
			0.8f,
			false,
			0.35f
		},

		// 中盤：別タイミングのTypeB
		{
			EnemyType::TypeB,
			{ 1800.0f, 300.0f },
			80.0f,
			0.8f,
			false,
			1.1f
		},

		// 終盤：ゴール前の地上敵
		{
			EnemyType::TypeA,
			{ 2500.0f, 0.0f },
			140.0f,
			2.2f,
			true,
			0.6f
		},

		// 終盤：空中敵で最後にプレッシャー
		{
			EnemyType::TypeB,
			{ 3000.0f, 300.0f },
			100.0f,
			1.0f,
			false,
			1.8f
		}
	};
}

void GameScene::Initialize() {
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/circle2.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/goal.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/gradationLine.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/enemy_typeA.png");
	TKM::TextureManager::GetInstance()->LoadTexture("./resources/texture/player.png");

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

	for (const auto& spawn : kEnemySpawns) {
		auto enemy = std::make_unique<ActionEnemy>();

		enemy->Initialize(
			dxCommon_,
			spawn.position,
			spawn.type,
			spawn.moveRange,
			spawn.moveSpeed
		);

		enemy->SetActionOffset(spawn.actionOffset);

		if (spawn.useGroundTop) {
			enemy->SetGroundTopY(ground_->GetTopY());
		}

		enemies_.push_back(std::move(enemy));
	}

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
	addBlock(548, 610);

	addBlock(1000, 550);
	addBlock(1048, 550);

	addBlock(2000, 500);
	addBlock(2048, 452);
	addBlock(2096, 404);
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

	timer_->Update();

	ground_->Update();

	back_->Update();

	lifeUI_->Update(player_->GetHP());

	bool isTimeUp = timer_->IsTimeUp();
	bool isAllEnemyDead = true;

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

		if (!enemy->IsDying() &&
			enemy->IsHitAttack(player_->GetAABB())) {
			player_->TakeDamage();
		}

		if (!enemy->IsDead()) {
			isAllEnemyDead = false;
		}
	}

	goal_->SetOpen(isAllEnemyDead);
	goal_->Update();

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

	if (isAllEnemyDead &&
		player_->GetAABB().IsCollidingWithAABB(goal_->GetAABB())) {
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