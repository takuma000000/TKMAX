#define NOMINMAX
#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include <algorithm>
#include <Windows.h>
#include <cmath>
#include "AudioCatalog.h"
#include "TextureCatalog.h"
#include "ModelCatalog.h"
#include "ParticleGroupsCatalog.h"
#include "TitleFlowStates.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using TKM::Camera;
using TKM::TextureManager;
using TKM::ModelManager;
using TKM::Sprite;

static float DegToRad_(float deg) {
	return deg * 3.14159265f / 180.0f;
}

static void VisibleHalfExtentsAtZ_(
	float& outHalfW, float& outHalfH,
	float z, float camZ,
	float fovYRad, float aspect
) {
	float d = z - camZ;
	if (d < 0.01f) { d = 0.01f; }

	outHalfH = std::tan(fovYRad * 0.5f) * d;
	outHalfW = outHalfH * aspect;
}

static float LookAtPitch_(const Vector3& from, const Vector3& to) {
	Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };
	const float horiz = std::sqrt(d.x * d.x + d.z * d.z);

	return -std::atan2f(d.y, (horiz < 0.0001f ? 0.0001f : horiz));
}

void TitleScene::Initialize() {
	/// ──────────────── カメラ初期化 ───────────────
	const Vector3 mainRot = { 0.0f, 0.0f, 0.0f };
	const Vector3 mainPos = { 0.0f, camY_, camDist_ };
	const Vector3 debugTarget = { 0.0f, 0.0f, 0.0f };

	TKM::CameraManager::GetInstance()->Initialize(mainRot, mainPos, debugTarget);

	camera_ = TKM::CameraManager::GetInstance()->GetMainCamera();

	/// ──────────────── テクスチャ読み込み ───────────────
	TextureCatalog::LoadTextureCatalogs();

	/// ──────────────── モデル読み込み ───────────────
	ModelCatalog::LoadModelCatalogs(dxCommon_);

	/// ──────────────── パーティクル初期化 ───────────────
	TKM::ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, TKM::CameraManager::GetInstance()->GetMainCamera());
	TKM::ParticleManager::GetInstance()->ClearAllGroups();
	TKM::ParticleGroupsCatalog::RegisterScene(TKM::ParticleManager::GetInstance());

	/// ──────────────── タイトル演出フロー初期化 ───────────────
	flowSM_.Initialize(this);
	flowSM_.Change(std::make_unique<TitleFlowIntroIrisOpenState>());

	/// ──────────────── タイトルスプライト初期化 ───────────────
	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/title_kuraran.dds");
	sprite_->SetPosition({ -10.0f,-290.0f });
	sprite_->SetSize({ 1.0f, 1.0f });

	/// ──────────────── ライト初期化 ───────────────
	dirLight_ = std::make_unique<TKM::DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f, -1.0f, 0.0f }, 1.0f);

	/// ──────────────── スカイボックス初期化 ───────────────
	skybox_ = std::make_unique<TKM::Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(TKM::CameraManager::GetInstance()->GetMainCamera());

	/// ──────────────── アイリス初期化 ───────────────
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMax_, "./resources/texture/circle2.png");
	iris_->SetColor({ 1,1,1,1 });

	irisStartScale_ = irisMax_;
	irisEndScale_ = 0.0f;
	irisScale_ = irisStartScale_;
	iris_->SetSize({ irisScale_, irisScale_ });

	irisTween_.Reset(
		irisStartScale_,
		irisEndScale_,
		kIrisDurationSec_,
		Ease::Type::OutBack
	);

	irisOpening_ = true;
	irisClosing_ = false;

	/// ──────────────── 水面波紋エフェクト初期化 ───────────────
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();
	rippleEffect_->Initialize(dxCommon_);
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());

	/// ──────────────── BGM読み込み・再生 ───────────────
	AudioCatalog::LoadTitleAudios();
	TKM::AudioManager::GetInstance()->PlaySound("title", 0.1f, true);

	/// ──────────────── タイトルメニュー初期化 ───────────────
	titleMenu_ = std::make_unique<TitleMenuController>();
	titleMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		1280.0f,
		720.0f
	);

	/// ──────────────── タイトル敵初期化 ───────────────
	CreateTitleEnemies_();

	/// ──────────────── 見つめ合い演出初期化 ───────────────
	titleShowdown_ = std::make_unique<TitleShowdownController>();
	titleShowdown_->Initialize(this, dxCommon_, srvManager_, camera_);
	titleShowdown_->SetBeamActive(true);

	/// ──────────────── 初期表示状態設定 ───────────────
	showUi_ = false;
	titleMenu_->SetVisible(false);

	/// ──────────────── 分岐フラグ初期化 ───────────────
	showMenuAfterVanish_ = false;

	/// ──────────────── タイマー初期化 ───────────────
	seqTimer_ = 0.0f;
	vanishTimer_ = 0.0f;
	rippleTimer_ = 0.0f;
}

void TitleScene::Finalize() {
	/// ──────────────── 各種終了処理 ───────────────
	TKM::AudioManager::GetInstance()->Finalize();
	TKM::ParticleManager::GetInstance()->ClearAllGroups();
}

void TitleScene::Update() {
	/// ──────────────── パフォーマンス情報更新 ───────────────
	UpdatePerformanceInfo();

	/// ──────────────── 入力更新 ───────────────
	TKM::Input::GetInstance()->Update();

	/// ──────────────── 基本システム更新 ───────────────
	dirLight_->Update();
	TKM::CameraManager::GetInstance()->Update();
	sprite_->Update();
	rippleEffect_->Update(dt_);

	/// ──────────────── タイトル演出フロー更新 ───────────────
	earlyExitUpdate_ = false;
	flowSM_.Update(dt_);

	if (earlyExitUpdate_) { return; }

	/// ──────────────── デバッグ遷移 ───────────────
	if (TKM::Input::GetInstance()->TriggerKey(DIK_T)) {
		sceneManager_->SetNextScene(
			std::make_unique<GameClearScene>(dxCommon_, srvManager_)
		);
		return;
	}

	/// ──────────────── スカイボックス回転更新 ───────────────
	constexpr float kTwoPi = 6.2831853f;
	skyPitch_ -= skyRotSpeedX_;

	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;

	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	/// ──────────────── パーティクル更新 ───────────────
	TKM::ParticleManager::GetInstance()->Update(dt_);

#ifdef USE_IMGUI
	/// ──────────────── ImGuiデバッグ表示 ───────────────
	ImGui::Begin("タイトルシーン デバッグ");

	/// ──────────────── パフォーマンス情報 ───────────────
	ImGuiDebugInfo();

	/// ──────────────── タイトル敵情報 ───────────────
	if (ImGui::CollapsingHeader("タイトル敵情報", ImGuiTreeNodeFlags_DefaultOpen)) {

		int aliveCount = 0;
		int totalCount = static_cast<int>(titleEnemies_.size());

		for (const auto& u : titleEnemies_) {
			if (u.alive_) {
				aliveCount++;
			}
		}

		ImGui::Text("生存数 : %d", aliveCount);
		ImGui::Text("総数   : %d", totalCount);
		ImGui::Text("消滅数 : %d", totalCount - aliveCount);

		ImGui::Separator();
	}

	/// ──────────────── Flow・タイマー情報 ───────────────
	if (ImGui::CollapsingHeader("状態 / タイマー", ImGuiTreeNodeFlags_DefaultOpen)) {

		const char* flowName = "（なし）";
		if (flowSM_.GetState()) {
			if (dynamic_cast<TitleFlowIntroIrisOpenState*>(flowSM_.GetState())) { flowName = "アイリスオープン中"; } else if (dynamic_cast<TitleFlowIdleState*>(flowSM_.GetState())) { flowName = "待機中"; } else if (dynamic_cast<TitleFlowVanishingState*>(flowSM_.GetState())) { flowName = "消滅演出中"; } else if (dynamic_cast<TitleFlowRippleState*>(flowSM_.GetState())) { flowName = "波紋演出中"; } else if (dynamic_cast<TitleFlowIrisCloseState*>(flowSM_.GetState())) { flowName = "アイリスクローズ中"; }
		}

		ImGui::Text("現在の状態 : %s", flowName);
		ImGui::Separator();
		ImGui::Text("シーケンスタイマー : %.2f 秒", seqTimer_);
		ImGui::Text("消滅タイマー       : %.2f 秒", vanishTimer_);
		ImGui::Text("波紋タイマー       : %.2f 秒", rippleTimer_);
	}

	ImGui::End();
#endif
}

void TitleScene::Draw() {
	DrawBack();
	Draw3D();
	DrawSprite();
}

void TitleScene::Draw3D() {
	/// ──────────────── 3D描画共通設定 ───────────────
	TKM::Object3dCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── タイトル敵描画 ───────────────
	for (auto& u : titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; }
		u.enemy_->Draw(dxCommon_);
	}

	/// ──────────────── 見つめ合い演出描画 ───────────────
	if (showUi_ && titleMenu_->IsVisible()) {
		titleShowdown_->Draw(dxCommon_);
	}

	/// ──────────────── パーティクル描画 ───────────────
	TKM::ParticleManager::GetInstance()->Draw();
}

void TitleScene::DrawSprite() {
	/// ──────────────── 2D描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── メニュー描画 ───────────────
	if (showUi_) {
		titleMenu_->Draw();
	}

	/// ──────────────── アイリス描画 ───────────────
	if (irisOpening_ || irisClosing_) {
		iris_->Draw();
	}
}

void TitleScene::DrawBack() {
	/// ──────────────── スカイボックス描画 ───────────────
	skybox_->Draw();

	/// ──────────────── 背景スプライト描画共通設定 ───────────────
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	/// ──────────────── タイトル画像描画 ───────────────
	if (showUi_) {
		sprite_->Draw();
	}
}

void TitleScene::CreateTitleEnemies_() {
	/// ──────────────── タイトル敵配列初期化 ───────────────
	titleEnemies_.clear();
	titleEnemies_.reserve(kEnemyCount);

	/// ──────────────── 画面・カメラ情報設定 ───────────────
	const float screenW = 1280.0f;
	const float screenH = 720.0f;
	const float aspect = screenW / screenH;

	const float fovY = DegToRad_(60.0f);
	const float camZ = camera_->GetTranslate().z;

	/// ──────────────── 出現Z範囲設定 ───────────────
	std::uniform_real_distribution<float> nearZ(6.0f, 14.0f);
	std::uniform_real_distribution<float> farZ(14.0f, 28.0f);

	const float kNearRatio = 0.65f;
	const int nearCount = static_cast<int>(kEnemyCount * kNearRatio);

	/// ──────────────── タイトル敵生成 ───────────────
	for (int i = 0; i < kEnemyCount; ++i) {
		const bool isNear = (i < nearCount);

		float z = isNear ? nearZ(rng_) : farZ(rng_);

		float halfW = 0.0f, halfH = 0.0f;
		VisibleHalfExtentsAtZ_(halfW, halfH, z, camZ, fovY, aspect);

		const float margin = isNear ? 6.0f : 10.0f;
		halfW = std::max(1.0f, halfW - margin);
		halfH = std::max(1.0f, halfH - margin);

		std::uniform_real_distribution<float> rx(-halfW, halfW);
		std::uniform_real_distribution<float> ry(-halfH, halfH);

		Vector3 pos = { rx(rng_), ry(rng_), z };

		TitleEnemyUnit u{};
		u.enemy_ = std::make_unique<Enemy>();
		u.enemy_->SetCamera(camera_);
		u.enemy_->SetParentScene(this);
		u.enemy_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);

		u.enemy_->SetModel("jerryfish.obj");
		u.enemy_->SetTentacleModel("tentacle.obj");
		u.enemy_->SetTentacleLocal({ 0,0,0 }, { 0,0,0 }, { 1,1,1 });

		u.enemy_->SetScale(isNear ? Vector3{ 1.35f, 1.35f, 1.35f } : Vector3{ 1.10f, 1.10f, 1.10f });
		u.enemy_->SetPosition(pos);

		u.enemy_->SetBehavior(EnemyBehavior::FreeRoam);

		Vector3 roamMin = { -halfW, -halfH, z };
		Vector3 roamMax = { halfW,  halfH, z };
		u.enemy_->SetRoamArea(roamMin, roamMax);

		u.enemy_->SetRoamSpeed(0.10f, 0.10f);

		u.alive_ = true;
		u.vanishDelay_ = 0.0f;
		titleEnemies_.push_back(std::move(u));
	}
}

void TitleScene::ScheduleVanish_() {
	/// ──────────────── 消滅タイミング設定 ───────────────
	std::uniform_real_distribution<float> d(0.0f, kVanishDelayMaxSec_);

	for (auto& u : titleEnemies_) {
		u.vanishDelay_ = d(rng_);
	}

	/// ──────────────── 消滅タイマー初期化 ───────────────
	vanishTimer_ = 0.0f;
}

void TitleScene::EmitTitleExplode_(const Vector3& pos) {
	/// ──────────────── タイトル敵消滅爆発 ───────────────
	Vector3 p = pos;
	auto* pm = TKM::ParticleManager::GetInstance();

	pm->Emit("titleExplode_core", p, 28);
	pm->Emit("titleExplode_rays", p, 140);
	pm->Emit("titleExplode_debris", p, 90);
	pm->Emit("titleExplode_ring", p, 2);
}

bool TitleScene::AllEnemiesGone_() const {
	/// ──────────────── タイトル敵全消滅判定 ───────────────
	for (const auto& u : titleEnemies_) {
		if (u.alive_) { return false; }
	}

	return true;
}