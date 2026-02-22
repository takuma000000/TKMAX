#define NOMINMAX
#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include <algorithm>
#include <Windows.h>
#include <cmath>
#include "AudioManager.h"

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
	if (d < 0.01f) { d = 0.01f; } // カメラより後ろ/近すぎ防止
	outHalfH = std::tan(fovYRad * 0.5f) * d;
	outHalfW = outHalfH * aspect;
}

static float LookAtPitch_(const Vector3& from, const Vector3& to) {
	Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };
	const float horiz = std::sqrt(d.x * d.x + d.z * d.z);
	return -std::atan2f(d.y, (horiz < 0.0001f ? 0.0001f : horiz)); // ラジアン（上向きがマイナスになる系）
}

void TitleScene::Initialize() {
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, camY_, -30.0f });

	// ------------ テクスチャ読み込み -----------using TKM::Camera;---
	TextureManager::GetInstance()->LoadTexture("./resources/texture/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/title_kuraran.dds");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/start_title.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/end_title.png");
	TextureManager::GetInstance()->LoadTexture("./resources/texture/rostock_laage_airport_4k.dds");
	//--------------------------------------------
	// ------------ モデル読み込み --------------
	ModelManager::GetInstance()->LoadModel("turtle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("turtle_flipper.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("jerryfish_boss.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("jerryfish.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("tentacle.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("tentacle_boss.obj", dxCommon_);
	//-----------------------------------------
	//---------------パーティクル----------------
	TKM::ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());

	auto* pm = TKM::ParticleManager::GetInstance();

	pm->CreateParticleGroup("titleExplode_core", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL);
	pm->CreateParticleGroup("titleExplode_rays", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::CYLINDER);
	pm->CreateParticleGroup("titleExplode_debris", "./resources/texture/circle.png", TKM::ParticleManager::ParticleType::NORMAL);
	pm->CreateParticleGroup("titleExplode_ring", "./resources/texture/gradationLine.png", TKM::ParticleManager::ParticleType::RING);
	//-----------------------------------------

	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/texture/title_kuraran.dds");
	// 画面中央に表示
	sprite_->SetPosition({ 0.0f,0.0f });
	sprite_->SetSize({ 1.0f, 1.0f });

	dirLight_ = std::make_unique<TKM::DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f, -1.0f, 0.0f }, 1.0f);

	skybox_ = std::make_unique<TKM::Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/texture/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());

	// === Iris sprite (白円) 共通ユーティリティ版 ===
	// 画面中央配置＋画面を覆う最大スケール irisMax_ をまとめて計算
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMax_, "./resources/texture/circle2.png");
	// 色だけここで上書き（白・不透明）
	iris_->SetColor({ 1,1,1,1 });
	// 開幕は「覆っている状態」からスタートして、縮んで消える
	irisStartScale_ = irisMax_;     // 最初：画面を覆う
	irisEndScale_ = 0.0f;         // 最後：消える（小さく）
	irisScale_ = irisStartScale_;
	iris_->SetSize({ irisScale_, irisScale_ });

	// Tween：大きい → 小さい（開く）
	irisTween_.Reset(
		irisStartScale_,
		irisEndScale_,
		kIrisDurationSec_,
		Ease::Type::OutBack   // 好きなのでOK（OutBackでも可）
	);
	// 開幕は「開いている状態」からスタート
	irisOpening_ = true;
	irisClosing_ = false;

	// ---------------水面波紋エフェクト----------------
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();
	rippleEffect_->Initialize(dxCommon_);
	// DirectXCommon 側に「現在の ripple はこれだよ」と教える
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());

	// ---------------BGMロード・再生----------------
	// タイトルBGMロード
	//TKM::AudioManager::GetInstance()->LoadSound("title", "kuraran.wav");
	// タイトルBGM再生
	//TKM::AudioManager::GetInstance()->PlaySound("title", 0.05f, true); // 音量少し下げめでループ

	// タイトルメニューコントローラ初期化
	titleMenu_ = std::make_unique<TitleMenuController>();
	titleMenu_->Initialize(
		TKM::SpriteCommon::GetInstance(),
		dxCommon_,
		this,
		1280.0f,
		720.0f
	);

	// 敵の初期化
	CreateTitleEnemies_();
	// メニュー中の見つめ合い用
	CreateShowdownActors_();

	// シーケンス開始
	flow_ = Flow::IntroIrisOpen;
	// 最初は敵だけ見せたいのでUIは消す
	showUi_ = false;
	if (titleMenu_) { titleMenu_->SetVisible(false); }
	// 分岐フラグ初期化
	showMenuAfterVanish_ = false;
	// タイマー初期化
	seqTimer_ = 0.0f;
	vanishTimer_ = 0.0f;
	rippleTimer_ = 0.0f;
}

void TitleScene::Finalize() {}

void TitleScene::Update() {
	ResetDrawCallCount();
	UpdatePerformanceInfo();

	TKM::Input::GetInstance()->Update();

	dirLight_->Update(); // 平行光源更新
	camera_->Update(); // カメラ更新
	sprite_->Update(); // タイトル画像更新

	if (rippleEffect_) {
		rippleEffect_->Update(dt_);
	}

	// アイリス（開幕：開く）更新
	if (irisOpening_) {
		irisScale_ = UpdateIrisScale(iris_.get(), irisTween_, 0.016f);

		if (irisTween_.Finished()) {
			irisOpening_ = false;
			// 念のため完全に消す（Draw条件でも消えるけど保険）
			irisScale_ = 0.0f;
			if (iris_) { iris_->SetSize({ irisScale_, irisScale_ }); }
		}
	}

	// ------------------------------------------------
	// Flow（タイトル演出）
	// ------------------------------------------------
	switch (flow_) {
	case Flow::IntroIrisOpen:
		// 開幕アイリスが終わったらIdleへ
		if (!irisOpening_) {
			flow_ = Flow::Idle;
		}

		for (auto& u : titleEnemies_) {
			if (!u.alive_ || !u.enemy_) { continue; }
			u.enemy_->Update(dt_);
		}
		break;
	case Flow::Idle:
		// 敵うようよ更新（残ってる敵がいる場合だけ）
		for (auto& u : titleEnemies_) {
			if (!u.alive_ || !u.enemy_) { continue; }
			u.enemy_->Update(dt_);
		}

		// -------------------------
		// ① メニューが出てる時：メニュー操作
		// -------------------------
		if (showUi_) {
			if (titleMenu_) {
				const auto cmd = titleMenu_->Update(dt_);
				// メニューの見つめ合い更新
				UpdateShowdownActors_(dt_);
				if (cmd == TitleMenuController::Command::Start) {

					// ★メニューでStartしたら、従来の「波紋→アイリス閉」へ
					showMenuAfterVanish_ = false; // 念のため
					flow_ = Flow::Ripple;
					rippleTimer_ = 0.0f;

					// 波紋を出す
					if (rippleEffect_) {
						TKM::WaterRippleEffect::RippleDesc d{};
						d.duration_ = 1.0f;
						d.radiusMax_ = 0.857f;
						d.amplitude_ = 0.1f;
						d.frequency_ = 80.0f;
						d.width_ = 10.0f;
						d.color_ = { 1.0f, 1.0f, 1.0f };
						d.colorIntensity_ = 0.0f;
						rippleEffect_->Trigger({ 0.5f, 0.5f }, d);
					}

				} else if (cmd == TitleMenuController::Command::Exit) {
					sceneManager_->RequestQuit();
					PostQuitMessage(0);
					return;
				}
			}
			break; // メニュー表示中はここで終わり
		}

		// -------------------------
		// ② メニューが出てない時：A待ち
		// -------------------------
		if (TKM::Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A)) {
			showUi_ = false;
			if (titleMenu_) { titleMenu_->SetVisible(false); }

			showMenuAfterVanish_ = true;
			ScheduleVanish_();
			flow_ = Flow::Vanishing;
			vanishTimer_ = 0.0f;
		}
		break;
	case Flow::Vanishing:
		vanishTimer_ += dt_;

		// 消えるまで敵は動いてOK
		for (auto& u : titleEnemies_) {
			if (!u.alive_ || !u.enemy_) { continue; }
			u.enemy_->Update(dt_);
		}

		// ランダム時差でぱぱぱ消す
		for (auto& u : titleEnemies_) {
			if (!u.alive_ || !u.enemy_) { continue; }

			if (vanishTimer_ >= u.vanishDelay_) {
				EmitTitleExplode_(u.enemy_->GetWorldPosition());
				u.alive_ = false;
			}
		}

		// 全滅したら「メニューへ」 or 「波紋へ」
		if (AllEnemiesGone_()) {

			if (showMenuAfterVanish_) {
				// A押しで消した場合：ここでメニュー表示
				showUi_ = true;
				if (titleMenu_) { titleMenu_->SetVisible(true); }

				// この後Startを押したら波紋へ行きたいので、ここで待機に戻す
				flow_ = Flow::Idle;
			} else {
				// 従来ルート：波紋 → IrisClose
				flow_ = Flow::Ripple;
				rippleTimer_ = 0.0f;

				if (rippleEffect_) {
					TKM::WaterRippleEffect::RippleDesc d{};
					d.duration_ = 1.0f;
					d.radiusMax_ = 0.857f;
					d.amplitude_ = 0.1f;
					d.frequency_ = 80.0f;
					d.width_ = 10.0f;
					d.color_ = { 1.0f, 1.0f, 1.0f };
					d.colorIntensity_ = 0.0f;
					rippleEffect_->Trigger({ 0.5f, 0.5f }, d);
				}
			}
		}
		break;
	case Flow::Ripple:
		rippleTimer_ += dt_;

		// 波紋を見せる待ち
		if (rippleTimer_ >= kRippleWaitSec_) {
			flow_ = Flow::IrisClose;
			irisClosing_ = true;

			irisTween_.Reset(
				0.0f,
				irisMax_,
				kIrisDurationSec_,
				Ease::Type::InBack
			);
		}
		break;
	case Flow::IrisClose:
		// irisClosing_ 更新は下の共通処理に任せる
		break;
	}

	// アイリス（閉）更新
	if (irisClosing_) {
		irisScale_ = UpdateIrisScale(iris_.get(), irisTween_, dt_);

		if (irisTween_.Finished()) {
			sceneManager_->SetNextScene(new GameScene(dxCommon_, srvManager_));
			return;
		}
	}

	// Yキーでゲームオーバーシーンへ
	if (TKM::Input::GetInstance()->TriggerKey(DIK_Y)) {
		sceneManager_->SetNextScene(new GameOverScene(dxCommon_, srvManager_));
		return;
	}

	constexpr float kTwoPi = 6.2831853f;
	skyPitch_ -= skyRotSpeedX_;
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

	TKM::ParticleManager::GetInstance()->Update(dt_);

#ifdef USE_IMGUI

	ImGui::Begin("タイトルシーン デバッグ");

	// =========================================================
	// ① 見つめ合い（Player/Boss）位置調整（折りたたみ）
	// =========================================================
	if (ImGui::CollapsingHeader("見つめ合い（位置調整）", ImGuiTreeNodeFlags_DefaultOpen)) {

		ImGui::Checkbox("自動で見つめ合う（Yaw/Pitch）", &titleAutoLookAt_); // デバッグ用：自動で見つめ合うかどうか（Yaw/Pitch計算して向きだけ合わせる）

		const bool 編集できる = (titlePlayer_ != nullptr && titleBoss_ != nullptr);
		if (!編集できる) {
			ImGui::TextDisabled("titlePlayer_ / titleBoss_ が null です");
		} else {

			bool 変更あり = false;
			変更あり |= ImGui::DragFloat3("プレイヤー位置", &titlePlayerPos_.x, 0.1f);
			変更あり |= ImGui::DragFloat3("ボス位置", &titleBossPos_.x, 0.1f);

			if (ImGui::Button("位置をリセット")) {
				titlePlayerPos_ = { -8.0f, -3.0f, 12.0f };
				titleBossPos_ = { 7.0f, -1.5f, 40.0f };
				変更あり = true;
			}

			if (変更あり) {
				titlePlayer_->SetPosition(titlePlayerPos_);
				titleBoss_->SetPosition(titleBossPos_);
			}

			const float py = LookAtYaw_(titlePlayerPos_, titleBossPos_);
			const float by = LookAtYaw_(titleBossPos_, titlePlayerPos_);
			ImGui::Text("Yaw（プレイヤー→ボス）: %.3f rad", py);
			ImGui::Text("Yaw（ボス→プレイヤー）: %.3f rad", by);
		}

		ImGui::Separator();
	}

	// =========================================================
	// ② タイトル敵情報（折りたたみ）
	// =========================================================
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

	// =========================================================
	// ③ Flow / タイマー（折りたたみ）
	// =========================================================
	if (ImGui::CollapsingHeader("状態 / タイマー", ImGuiTreeNodeFlags_DefaultOpen)) {

		const char* flowName = "";
		switch (flow_) {
		case Flow::IntroIrisOpen: flowName = "アイリスオープン中"; break;
		case Flow::Idle:          flowName = "待機中"; break;
		case Flow::Vanishing:     flowName = "消滅演出中"; break;
		case Flow::Ripple:        flowName = "波紋演出中"; break;
		case Flow::IrisClose:     flowName = "アイリスクローズ中"; break;
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
	DrawBack(); // 2D（背景）
	Draw3D(); // 3Dオブジェクト
	DrawSprite(); // UI（手前固定）
}

void TitleScene::Draw3D() {
	// 3D
	TKM::Object3dCommon::GetInstance()->DrawSetCommon();
	for (auto& u : titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; }
		u.enemy_->Draw(dxCommon_);
	}

	// メニュー中だけ：見つめ合い（Player/Boss）
	if (showUi_ && titleMenu_ && titleMenu_->IsVisible()) {
		DrawShowdownActors_();
	}

	// Particle
	TKM::ParticleManager::GetInstance()->Draw();
}

void TitleScene::DrawSprite() {
	// UI（手前固定：Swapchain側）
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	if (showUi_) {
		if (titleMenu_) { titleMenu_->Draw(); }
	}
	if (iris_ && (irisOpening_ || irisClosing_)) {
		iris_->Draw();
	}
}

void TitleScene::DrawBack() {
	if (skybox_) { skybox_->Draw(); } // スカイボックス（背景3D）

	// 2D（背景：3Dより先に描かれる＝奥になる）
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();

	if (showUi_) {
		if (sprite_) { sprite_->Draw(); } // タイトル画像（背景）
	}
}

void TitleScene::CreateTitleEnemies_() {
	titleEnemies_.clear();
	titleEnemies_.reserve(kEnemyCount);

	const float screenW = 1280.0f;
	const float screenH = 720.0f;
	const float aspect = screenW / screenH;

	const float fovY = DegToRad_(60.0f);      // タイトルは広め
	const float camZ = camera_ ? camera_->GetTranslate().z : -30.0f;

	// Zレンジ（君の近・奥の2層）
	std::uniform_real_distribution<float> nearZ(6.0f, 14.0f);
	std::uniform_real_distribution<float> farZ(14.0f, 28.0f);

	const float kNearRatio = 0.65f;
	const int nearCount = static_cast<int>(kEnemyCount * kNearRatio);

	for (int i = 0; i < kEnemyCount; ++i) {
		const bool isNear = (i < nearCount);

		float z = isNear ? nearZ(rng_) : farZ(rng_);

		float halfW = 0.0f, halfH = 0.0f;
		VisibleHalfExtentsAtZ_(halfW, halfH, z, camZ, fovY, aspect);

		// ★少し内側に寄せる（端ギリだと動いた瞬間はみ出るから）
		const float margin = isNear ? 6.0f : 10.0f;
		halfW = std::max(1.0f, halfW - margin);
		halfH = std::max(1.0f, halfH - margin);

		std::uniform_real_distribution<float> rx(-halfW, halfW);
		std::uniform_real_distribution<float> ry(-halfH, halfH);

		Vector3 pos = { rx(rng_), ry(rng_), z };

		TitleEnemyUnit u{};
		u.enemy_ = std::make_unique<Enemy>();
		u.enemy_->SetCamera(camera_.get());
		u.enemy_->SetParentScene(this);
		u.enemy_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);

		u.enemy_->SetModel("jerryfish.obj");
		u.enemy_->SetTentacleModel("tentacle.obj");
		u.enemy_->SetTentacleLocal({ 0,0,0 }, { 0,0,0 }, { 1,1,1 });

		u.enemy_->SetScale(isNear ? Vector3{ 1.35f, 1.35f, 1.35f } : Vector3{ 1.10f, 1.10f, 1.10f });
		u.enemy_->SetPosition(pos);

		u.enemy_->SetBehavior(EnemyBehavior::FreeRoam);

		// ★ローム範囲も「このZの画面内」に合わせてセット（=画面外へ行きにくい）
		// ただし “絶対に出ない” を保証するには Enemy の移動側でクランプが必要
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
	std::uniform_real_distribution<float> d(0.0f, kVanishDelayMaxSec_);
	for (auto& u : titleEnemies_) {
		u.vanishDelay_ = d(rng_);
	}
	vanishTimer_ = 0.0f;
}

void TitleScene::EmitTitleExplode_(const Vector3& pos) {
	Vector3 p = pos;
	auto* pm = TKM::ParticleManager::GetInstance();

	pm->Emit("titleExplode_core", p, 28);   // 白飛びコア（爽快感の核）
	pm->Emit("titleExplode_rays", p, 140);  // 放射光線（画像の“バァン”）
	pm->Emit("titleExplode_debris", p, 90);   // 火の粉/破片
	pm->Emit("titleExplode_ring", p, 2);    // 衝撃波
}
bool TitleScene::AllEnemiesGone_() const {
	for (const auto& u : titleEnemies_) {
		if (u.alive_) { return false; }
	}
	return true;
}

float TitleScene::LookAtYaw_(const Vector3& from, const Vector3& to) const {
	Vector3 d = { to.x - from.x, to.y - from.y, to.z - from.z };
	return std::atan2f(d.x, d.z); // ラジアン
}

void TitleScene::CreateShowdownActors_() {
	// Player（GameScene同様にクラスを使う）
	titlePlayer_ = std::make_unique<Player>();
	titlePlayer_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
	titlePlayer_->SetParentScene(this);
	titlePlayer_->SetCamera(camera_.get());
	titlePlayer_->SetPosition(titlePlayerPos_);

	// タイトルでは操作系全部OFF（事故防止）
	titlePlayer_->SetControlEnabled(false);
	titlePlayer_->SetShootingEnabled(false);
	titlePlayer_->SetReticleVisible(false);
	titlePlayer_->SetEnableJetSmoke(false);
	titlePlayer_->SetEnemy(nullptr);
	titlePlayer_->StopRumble();

	// Boss（BossEnemyクラスを使う）
	titleBoss_ = std::make_unique<BossEnemy>();
	titleBoss_->SetCamera(camera_.get());
	titleBoss_->SetParentScene(this);
	titleBoss_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
	titleBoss_->SetPosition(titleBossPos_);
	titleBoss_->SetTentacleCharge(true, 0.35f);

	// 見つめ合い（Yawだけ）
	const float py = LookAtYaw_(titlePlayerPos_, titleBossPos_);
	const float by = LookAtYaw_(titleBossPos_, titlePlayerPos_);
	// タイトル用の回転を別で持つ（GameSceneのとは別物）
	titlePlayerRot_.y = py;
	titleBossRot_.y = by;
	// プレイヤー（Yawだけ）
	titlePlayer_->SetYaw(titlePlayerRot_.y);
	// ボス（Euler回転を使う：EnemyにSetRotateを生やした前提）
	titleBoss_->SetRotate(titleBossRot_);
}

void TitleScene::UpdateShowdownActors_(float dt) {
	if (!titlePlayer_ || !titleBoss_) { return; }

	// 位置固定
	titlePlayer_->SetPosition(titlePlayerPos_);
	titleBoss_->SetPosition(titleBossPos_);

	// まず「狙うべき回転」を計算
	Vector3 pRotRad{ 0.0f, 0.0f, 0.0f };
	Vector3 bRotRad{ 0.0f, 0.0f, 0.0f };

	if (titleAutoLookAt_) {
		const float py = LookAtYaw_(titlePlayerPos_, titleBossPos_);
		const float by = LookAtYaw_(titleBossPos_, titlePlayerPos_);

		const float pp = LookAtPitch_(titlePlayerPos_, titleBossPos_);
		const float bp = LookAtPitch_(titleBossPos_, titlePlayerPos_);

		pRotRad = { pp, py, 0.0f };
		bRotRad = { bp, by, 0.0f };
	}

	// 手動オフセット（度→rad）を足す：モデル正面ズレ調整はここでやる
	pRotRad.x += DegToRad_(titlePlayerRotDeg_.x);
	pRotRad.y += DegToRad_(titlePlayerRotDeg_.y);
	pRotRad.z += DegToRad_(titlePlayerRotDeg_.z);

	bRotRad.x += DegToRad_(titleBossRotDeg_.x);
	bRotRad.y += DegToRad_(titleBossRotDeg_.y);
	bRotRad.z += DegToRad_(titleBossRotDeg_.z);

	// ★回転を先に適用（このフレームで反映させる）
	titlePlayer_->SetRotate(pRotRad);
	titleBoss_->SetRotate(bRotRad);

	// そのあと Update（内部で object_->Update() されて行列が確定する）
	titlePlayer_->UpdateTitleIdle(dt);

	titleBoss_->Update(dt);
	titleBoss_->SetPosition(titleBossPos_); // 保険
}

void TitleScene::DrawShowdownActors_() {
	if (!titlePlayer_ || !titleBoss_) { return; }
	titlePlayer_->Draw(dxCommon_);
	titleBoss_->Draw(dxCommon_);
}