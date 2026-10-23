#define NOMINMAX
#include "GameScene.h"
#include "AudioCatalog.h"
#include "TextureCatalog.h"
#include "ModelCatalog.h"
#include "CameraManager.h"
#include "ParticleManager.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using namespace TKM;

void GameScene::Initialize() {
	/// ──────────────── NULLチェック ────────────────
	assert(this != nullptr && "this is nullptr in GameScene::Initialize");
	assert(dxCommon_ != nullptr && "dxCommon is nullptr in GameScene::Initialize");

	/// ──────────────── 各種初期化処理 ───────────────
	AudioCatalog::LoadGameAudios();
	TextureCatalog::LoadTextureCatalogs();
	ModelCatalog::LoadModelCatalogs(dxCommon_);
	InitializeObjects();
	InitializeSprite();
	InitializeCamera();

	/// ──────────────── パーティクル初期化 ───────────────
	auto* particleManager = TKM::ParticleManager::GetInstance();
	particleManager->Initialize(dxCommon_, srvManager_, TKM::CameraManager::GetInstance()->GetMainCamera());

	/// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize({ 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);
}

void GameScene::Finalize() {
	/// ──────────────── 各種終了処理 ───────────────
	TextureManager::GetInstance()->Finalize();
	AudioManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();

	/// ──────────────── パーティクルの終了 ───────────────
	ParticleManager::GetInstance()->ClearAllGroups();
}

void GameScene::Update() {
	/// ──────────────── カメラ更新 ───────────────
	UpdateActiveCamera();

	/// ──────────────── ライト更新 ───────────────
	directionalLight_->Update();

	/// ──────────────── パーティクル更新 ───────────────
	constexpr float deltaTime = 1.0f / 60.0f;
	TKM::ParticleManager::GetInstance()->Update(deltaTime);

#ifdef USE_IMGUI
	ImGuiDebug();
#endif
}

void GameScene::Draw() {
	Draw3D();
	DrawSprite();
}

void GameScene::Draw3D() {

	/// ──────────────── 3Dオブジェクト描画 ───────────────
	Object3dCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── パーティクル描画 ───────────────
	ParticleManager::GetInstance()->Draw();
}

void GameScene::DrawSprite() {
	/// ──────────────── 2D描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
}

TKM::Camera* GameScene::UpdateActiveCamera() {
	auto* activeCamera =
		TKM::CameraManager::GetInstance()->Update();

	if (activeCamera) {
		TKM::ParticleManager::GetInstance()->SetCamera(activeCamera);
	}

	return activeCamera;
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeObjects() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを作成し、各オブジェクトに適用する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeCamera() {
	/// ──────────────── カメラ初期値 ───────────────
	Vector3 mainRot = { 0.12f, -1.2f, 0.0f };
	Vector3 mainPos = { 0.0f, 0.0f, -30.0f };
	Vector3 debugTarget = { 0.0f, 0.0f, 0.0f };

	/// ──────────────── カメラマネージャー初期化 ───────────────
	TKM::CameraManager::GetInstance()->Initialize(mainRot, mainPos, debugTarget);

	// 行列を即更新して、1フレームだけ古いカメラになるのを防ぐ
	if (TKM::CameraManager::GetInstance()->GetMainCamera()) {
		TKM::CameraManager::GetInstance()->GetMainCamera()->Update();
	}
}

void GameScene::ImGuiDebug() {
#ifdef USE_IMGUI

#endif
}

