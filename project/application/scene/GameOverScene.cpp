#include "GameOverScene.h"

#include "AudioCatalog.h"
#include "TextureCatalog.h"
#include "ModelCatalog.h"

#include "AudioManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "SpriteCommon.h"

#include <cassert>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void GameOverScene::Initialize() {
	/// ──────────────── NULLチェック ────────────────
	assert(dxCommon_ != nullptr && "dxCommon is nullptr in GameOverScene::Initialize");
	assert(srvManager_ != nullptr && "srvManager is nullptr in GameOverScene::Initialize");

	/// ──────────────── 各種初期化処理 ───────────────
	AudioCatalog::LoadResultAudios();
	TextureCatalog::LoadTextureCatalogs();
	ModelCatalog::LoadModelCatalogs(dxCommon_);

	InitializeObjects();
	InitializeSprite();
	InitializeCamera();

	/// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<TKM::DirectionalLight>();
	directionalLight_->Initialize(
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		{ 0.0f, -1.0f, 0.0f },
		1.0f
	);
}

void GameOverScene::Finalize() {
	/// ──────────────── ライトの終了 ───────────────
	directionalLight_.reset();

	/// ──────────────── 各種終了処理 ───────────────
	TKM::TextureManager::GetInstance()->Finalize();
	TKM::AudioManager::GetInstance()->Finalize();
	TKM::ModelManager::GetInstance()->Finalize();
}

void GameOverScene::Update() {
	/// ──────────────── カメラ更新 ───────────────
	UpdateActiveCamera();

	/// ──────────────── ライト更新 ───────────────
	directionalLight_->Update();

#ifdef USE_IMGUI
	/// ──────────────── デバッグ表示更新 ───────────────
	ImGuiDebug();
#endif
}

void GameOverScene::Draw() {
	DrawBack();
	Draw3D();
	DrawSprite();
}

void GameOverScene::Draw3D() {
	/// ──────────────── 3Dオブジェクト描画 ───────────────
	TKM::Object3dCommon::GetInstance()->DrawSetCommon();
}

void GameOverScene::DrawSprite() {
	/// ──────────────── 2D描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
}

void GameOverScene::DrawBack() {}

TKM::Camera* GameOverScene::UpdateActiveCamera() {
	return TKM::CameraManager::GetInstance()->Update();
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameOverScene::InitializeSprite() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameOverScene::InitializeObjects() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameOverScene::InitializeCamera() {
	/// ──────────────── カメラ初期値 ───────────────
	const Vector3 mainRot = { 0.0f, 0.0f, 0.0f };
	const Vector3 mainPos = { 0.0f, 3.0f, -30.0f };
	const Vector3 debugTarget = { 0.0f, 0.0f, 0.0f };

	/// ──────────────── カメラマネージャー初期化 ───────────────
	auto* cameraManager = TKM::CameraManager::GetInstance();
	cameraManager->Initialize(mainRot, mainPos, debugTarget);

	/// ──────────────── 行列の即時更新 ───────────────
	if (auto* camera = cameraManager->GetMainCamera()) {
		camera->Update();
	}
}

void GameOverScene::ImGuiDebug() {
#ifdef USE_IMGUI

#endif
}