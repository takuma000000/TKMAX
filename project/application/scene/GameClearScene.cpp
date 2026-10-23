#include "GameClearScene.h"

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

void GameClearScene::Initialize() {
	/// ──────────────── NULLチェック ────────────────
	assert(dxCommon_ != nullptr && "dxCommon is nullptr in GameClearScene::Initialize");
	assert(srvManager_ != nullptr && "srvManager is nullptr in GameClearScene::Initialize");

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

void GameClearScene::Finalize() {
	/// ──────────────── ライトの終了 ───────────────
	directionalLight_.reset();

	/// ──────────────── 各種終了処理 ───────────────
	TKM::TextureManager::GetInstance()->Finalize();
	TKM::AudioManager::GetInstance()->Finalize();
	TKM::ModelManager::GetInstance()->Finalize();
}

void GameClearScene::Update() {
	/// ──────────────── カメラ更新 ───────────────
	UpdateActiveCamera();

	/// ──────────────── ライト更新 ───────────────
	directionalLight_->Update();

#ifdef USE_IMGUI
	/// ──────────────── デバッグ表示更新 ───────────────
	ImGuiDebug();
#endif
}

void GameClearScene::Draw() {
	DrawBack();
	Draw3D();
	DrawSprite();
}

void GameClearScene::Draw3D() {
	/// ──────────────── 3Dオブジェクト描画 ───────────────
	TKM::Object3dCommon::GetInstance()->DrawSetCommon();
}

void GameClearScene::DrawSprite() {
	/// ──────────────── 2D描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
}

void GameClearScene::DrawBack() {}

TKM::Camera* GameClearScene::UpdateActiveCamera() {
	return TKM::CameraManager::GetInstance()->Update();
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameClearScene::InitializeSprite() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameClearScene::InitializeObjects() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameClearScene::InitializeCamera() {
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

void GameClearScene::ImGuiDebug() {
#ifdef USE_IMGUI

#endif
}