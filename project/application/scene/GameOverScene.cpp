#include "GameOverScene.h"
#include "TextureManager.h"
#include "Input.h"
#include "SceneManager.h"
#include "engine/2d/SpriteCommon.h"
#include "engine/3d/Object3dCommon.h"

// #include "TitleScene.h"
// #include "GameScene.h"

void GameOverScene::Initialize()
{
	ModelManager::GetInstance()->LoadModel("jett.obj", dxCommon_);
	TextureManager::GetInstance()->LoadTexture("./resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");

	// --- カメラ ---
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 2.0f, -15.0f });
	camera_->Update();

	// ライト
	dirLight_ = std::make_unique<DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f,-1.0f,0.0f }, 1.0f);

	// パーティクル
	ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());

	// --- Player ---
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon_);
	player_->SetCamera(camera_.get());
	player_->SetPosition({ 0.0f, 0.0f, 0.0f }); // 初期位置
	player_->SetEnableJetSmoke(false); // ジェット噴射無効
}

void GameOverScene::Finalize()
{
	
}

void GameOverScene::Update()
{
	//Input::GetInstance()->Update();

	if (player_) { player_->Update(); }
	if (camera_) { camera_->Update(); }
	if (dirLight_) { dirLight_->Update(); }
	ParticleManager::GetInstance()->Update();
}

void GameOverScene::Draw()
{
	// 3D
	Object3dCommon::GetInstance()->DrawSetCommon();
	if (player_) { player_->Draw(dxCommon_); }

	// パーティクル描画
	ParticleManager::GetInstance()->Draw();

	// 2D（任意のオーバーレイ）
	SpriteCommon::GetInstance()->DrawSetCommon();
	//if (gameOverSprite_) { gameOverSprite_->Draw(); }
}
