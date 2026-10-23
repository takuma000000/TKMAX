#include "TitleScene.h"

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

using namespace TKM;

void TitleScene::Initialize() {
	/// ──────────────── NULLチェック ────────────────
	assert(dxCommon_ != nullptr && "dxCommon is nullptr in TitleScene::Initialize");
	assert(srvManager_ != nullptr && "srvManager is nullptr in TitleScene::Initialize");

	/// ──────────────── 各種初期化処理 ───────────────
	AudioCatalog::LoadTitleAudios();
	TextureCatalog::LoadTextureCatalogs();
	ModelCatalog::LoadModelCatalogs(dxCommon_);

	InitializeObjects();
	InitializeSprite();
	InitializeCamera();

	/// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize(
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		{ 0.0f, -1.0f, 0.0f },
		1.0f
	);
}

void TitleScene::Finalize() {
	/// ──────────────── ライトの終了 ───────────────
	directionalLight_.reset();

	/// ──────────────── 各種終了処理 ───────────────
	TextureManager::GetInstance()->Finalize();
	AudioManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
}

void TitleScene::Update() {
	/// ──────────────── カメラ更新 ───────────────
	UpdateActiveCamera();

	/// ──────────────── ライト更新 ───────────────
	directionalLight_->Update();

#ifdef USE_IMGUI
	/// ──────────────── デバッグ表示更新 ───────────────
	ImGuiDebug();
#endif
}

void TitleScene::Draw() {
	DrawBack();
	Draw3D();
	DrawSprite();
}

void TitleScene::Draw3D() {
	/// ──────────────── 3Dオブジェクト描画 ───────────────
	Object3dCommon::GetInstance()->DrawSetCommon();
}

void TitleScene::DrawSprite() {
	/// ──────────────── 2D描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
}

void TitleScene::DrawBack() {
	// 背景の描画処理を追加する
}

TKM::Camera* TitleScene::UpdateActiveCamera() {
	return CameraManager::GetInstance()->Update();
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void TitleScene::InitializeSprite() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void TitleScene::InitializeObjects() {}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void TitleScene::InitializeCamera() {
	/// ──────────────── カメラ初期値 ───────────────
	const Vector3 mainRot = { 0.0f, 0.0f, 0.0f };
	const Vector3 mainPos = { 0.0f, 3.0f, -30.0f };
	const Vector3 debugTarget = { 0.0f, 0.0f, 0.0f };

	/// ──────────────── カメラマネージャー初期化 ───────────────
	CameraManager::GetInstance()->Initialize(
		mainRot,
		mainPos,
		debugTarget
	);

	/// ──────────────── 行列の即時更新 ───────────────
	if (auto* camera = CameraManager::GetInstance()->GetMainCamera()) {
		camera->Update();
	}
}

void TitleScene::ImGuiDebug() {
#ifdef USE_IMGUI
	// デバッグ表示を追加する
#endif
}