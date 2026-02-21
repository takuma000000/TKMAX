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
	ModelManager::GetInstance()->LoadModel("jerryfish.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("tentacle.obj", dxCommon_);
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
	// シーケンス開始
	flow_ = Flow::IntroIrisOpen;
	showUi_ = true;
	seqTimer_ = 0.0f;
	vanishTimer_ = 0.0f;
	if (titleMenu_) { titleMenu_->SetVisible(true); }
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
		// 敵うようよ更新
		for (auto& u : titleEnemies_) {
			if (!u.alive_ || !u.enemy_) { continue; }
			u.enemy_->Update(dt_);
		}

		// UI更新
		if (titleMenu_) {
			const auto cmd = titleMenu_->Update(dt_);
			if (cmd == TitleMenuController::Command::Start) {
				flow_ = Flow::StartSequence;
				seqTimer_ = 0.0f;
				vanishTimer_ = 0.0f;
				rippleTimer_ = 0.0f;
			} else if (cmd == TitleMenuController::Command::Exit) {
				sceneManager_->RequestQuit();
				PostQuitMessage(0);
				return;
			}
		}
		break;

	case Flow::StartSequence:
		// 敵は動かしてOK
		for (auto& u : titleEnemies_) {
			if (!u.alive_ || !u.enemy_) { continue; }
			u.enemy_->Update(dt_);
		}

		seqTimer_ += dt_;

		// UI消し
		if (seqTimer_ >= kHideUiDelaySec_) {
			showUi_ = false;
			if (titleMenu_) { titleMenu_->SetVisible(false); }
		}

		// 消滅スケジュール開始
		if (seqTimer_ >= kStartVanishDelaySec_) {
			ScheduleVanish_();
			flow_ = Flow::Vanishing;
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

		// 全滅したら波紋 → Iris close
		if (AllEnemiesGone_()) {
			flow_ = Flow::Ripple;
			rippleTimer_ = 0.0f;

			// ここで波紋
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

	//// === ImGui ===
	//ImGui::Begin("Title Heli (Background)");
	//// position
	/*ImGui::Text("Heli Position: (%.2f, %.2f, %.2f)", heli_->GetTranslate().x, heli_->GetTranslate().y, heli_->GetTranslate().z);
	//ImGui::SliderFloat("Radius", &radius_, 0.0f, 30.0f);
	//ImGui::SliderFloat("BaseY", &baseY_, -5.0f, 10.0f);
	//ImGui::SliderFloat("Bob Amp", &bobAmp_, 0.0f, 5.0f);
	//ImGui::SliderFloat("Speed", &speed_, 0.0f, 5.0f);
	//ImGui::SliderFloat("Yaw Offset", &yawOffset_, -3.14f, 3.14f);
	//ImGui::SliderFloat("Scale", &scale_, 0.1f, 5.0f);
	//ImGui::Separator();
	//ImGui::SliderFloat("Cam Dist", &camDist_, 2.0f, 60.0f);
	//ImGui::SliderFloat("Cam Y", &camY_, -5.0f, 20.0f);
	//if (ImGui::Button("Apply Camera")) {
	//	camera_->SetTranslate({ 0.0f, camY_, -camDist_ });
	//	camera_->Update();
	}*/
	//ImGui::End();

#endif // USE_IMGUI
}

void TitleScene::Draw() {
	if (skybox_) { skybox_->Draw(); } // スカイボックスは一番最初に描画

	// 3D
	TKM::Object3dCommon::GetInstance()->DrawSetCommon();
	for (auto& u : titleEnemies_) {
		if (!u.alive_ || !u.enemy_) { continue; }
		u.enemy_->Draw(dxCommon_);
	}

	// 2D
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	if (showUi_) {
		if (sprite_) { sprite_->Draw(); }
		if (titleMenu_) { titleMenu_->Draw(); }
	}
	if (iris_ && (irisOpening_ || irisClosing_)) {
		iris_->Draw();
	}

	TKM::ParticleManager::GetInstance()->Draw();
}
void TitleScene::CreateTitleEnemies_() {
	titleEnemies_.clear(); // 念のためクリア
	titleEnemies_.reserve(15); // 15体くらいは出したい（多すぎるとごちゃごちゃするのでほどほどに）

	const Vector3 roamMin = { -22.0f, 0.8f, 48.0f }; // 敵のうろうろ範囲の最小値（X: -22～18, Y: 0.8～13, Z: 48～96あたり）※適宜調整
	const Vector3 roamMax = { 18.0f, 13.0f, 96.0f }; // 敵のうろうろ範囲（X: -22～18, Y: 0.8～13, Z: 48～96あたり）※適宜調整

	std::uniform_real_distribution<float> rx(roamMin.x, roamMax.x); // 敵の初期配置用の乱数分布（X座標）
	std::uniform_real_distribution<float> ry(roamMin.y, roamMax.y); // 敵の初期配置用の乱数分布（Y座標）
	std::uniform_real_distribution<float> rz(roamMin.z, roamMax.z); // 敵の初期配置用の乱数分布（Z座標）

	for (int i = 0; i < 15; ++i) {
		TitleEnemyUnit u{};
		u.enemy_ = std::make_unique<Enemy>();
		u.enemy_->SetCamera(camera_.get());
		u.enemy_->SetParentScene(this);
		u.enemy_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);

		u.enemy_->SetModel("jerryfish.obj");
		u.enemy_->SetTentacleModel("tentacle.obj");
		u.enemy_->SetTentacleLocal({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });

		u.enemy_->SetScale({ 1.0f, 1.0f, 1.0f }); // 少し大きめにして存在感アップ
		u.enemy_->SetPosition({ rx(rng_), ry(rng_), rz(rng_) }); // ランダムな位置に配置

		u.enemy_->SetBehavior(EnemyBehavior::FreeRoam); // 自由にうろうろする動き
		u.enemy_->SetRoamArea(roamMin, roamMax);
		u.enemy_->SetRoamSpeed(0.08f, 0.08f); // ゆっくり目の移動速度（0.05～0.15くらい��見栄え良い）

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