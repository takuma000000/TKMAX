#define NOMINMAX
#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

using TKM::Camera;
using TKM::TextureManager;
using TKM::ModelManager;
using TKM::Sprite;

void TitleScene::Initialize(){
	camera_ = std::make_unique<Camera>();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, camY_, -30.0f });

	// ------------ テクスチャ読み込み -----------using TKM::Camera;---
	TextureManager::GetInstance()->LoadTexture("./resources/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./resources/title_kuraran.png");
	TextureManager::GetInstance()->LoadTexture("./resources/rostock_laage_airport_4k.dds");
	//--------------------------------------------
	// ------------ モデル読み込み --------------
	ModelManager::GetInstance()->LoadModel("jett.obj", dxCommon_);
	ModelManager::GetInstance()->LoadModel("enemy.obj", dxCommon_);
	//-----------------------------------------

	heli_ = std::make_unique<TKM::Object3d>();
	heli_->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
	heli_->SetModel("jett.obj");
	heli_->SetCamera(camera_.get());
	heli_->SetScale({ scale_, scale_, scale_ });
	heli_->SetTranslate({ 0.0f, baseY_, 0.0f });

	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon_, "./resources/title_kuraran.png");
	// 画面中央に表示
	sprite_->SetPosition({ 0.0f,0.0f });
	sprite_->SetSize({ 1.0f, 1.0f });

	dirLight_ = std::make_unique<TKM::DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f, -1.0f, 0.0f }, 1.0f);

	skybox_ = std::make_unique<TKM::Skybox>();
	skybox_->Initialize(dxCommon_, srvManager_, "resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera_.get());

	// === Iris sprite (白円) 共通ユーティリティ版 ===
	// 画面中央配置＋画面を覆う最大スケール irisMax_ をまとめて計算
	iris_ = CreateCenteredIrisSprite(dxCommon_, irisMax_, "./resources/circle2.png");
	// 色だけここで上書き（白・不透明）
	iris_->SetColor({ 1,1,1,1 });
	// 開始／終了スケールの設定
	irisStartScale_ = 10.0f;     // 最初は小さく
	irisEndScale_ = irisMax_;  // 最後に画面を完全に覆う
	// 現在スケールも開始値からスタート
	irisScale_ = irisStartScale_;
	// スプライトに反映
	iris_->SetSize({ irisScale_, irisScale_ });
	// 必要なら速度パラメータはそのまま保持（他で使ってるなら）
	irisSpeed_ = 3500.0f;
	// Tween の初期化（小さい → 大きい）
	irisTween_.Reset(
		irisStartScale_,          // start
		irisEndScale_,            // end
		kIrisDurationSec_,         // 所要時間
		Ease::Type::InBack        // ちょっと勢いつけて開く感じ
	);

	// タイトル敵を1体だけ置く
	titleEnemies_.clear();
	{
		auto e = std::make_unique<TKM::Object3d>();
		e->Initialize(TKM::Object3dCommon::GetInstance(), dxCommon_);
		e->SetModel("enemy.obj");
		e->SetCamera(camera_.get());
		e->SetScale({ enemyScale_, enemyScale_, enemyScale_ });
		e->SetTranslate({ enemyRadius_, enemyBaseY_, 0.0f }); // 右前方あたり
		titleEnemies_.push_back(std::move(e));
	}

	//---------------パーティクル----------------
	TKM::ParticleManager::GetInstance()->Initialize(dxCommon_, srvManager_, camera_.get());
	//-----------------------------------------

	// ---------------水面波紋エフェクト----------------
	rippleEffect_ = std::make_unique<TKM::WaterRippleEffect>();
	rippleEffect_->Initialize(dxCommon_);
	// DirectXCommon 側に「現在の ripple はこれだよ」と教える
	dxCommon_->SetWaterRippleEffect(rippleEffect_.get());

	// ---------------BGMロード・再生----------------
	// タイトルBGMロード
	//AudioManager::GetInstance()->LoadSound("title", "kuraran.wav");
	// タイトルBGM再生（1回だけ）
	//AudioManager::GetInstance()->PlaySound("title");
}

void TitleScene::Finalize(){}

void TitleScene::Update(){
	ResetDrawCallCount();
	UpdatePerformanceInfo();

	TKM::Input::GetInstance()->Update();

	// === ヘリの旋回＋上下動（上下幅拡大版） ===
	t_ += 0.01f * std::max(0.0f, speed_);

	float R = radius_;
	Vector3 pos;
	pos.x = std::cos(t_) * R;
	pos.z = std::sin(t_) * R;
	// 上下動をより大きく（*2.0fで増幅）
	pos.y = baseY_ + std::sin(t_ * 0.5f) * (bobAmp_ * 2.0f);

	float dirX = -std::sin(t_);
	float dirZ = std::cos(t_);
	float yaw = std::atan2(dirX, dirZ) + yawOffset_;

	heli_->SetTranslate(pos);
	heli_->SetRotate({ 0.0f, yaw, 0.0f });
	heli_->SetScale({ scale_, scale_, scale_ });
	heli_->Update();

	enemyTime_ += 0.01f * std::max(0.0f, enemySpeed_);

	if (!titleEnemies_.empty()) {
		auto& e = titleEnemies_.front();

		Vector3 pos{};
		float yaw = 0.0f;

		switch (enemyMotion_) {
		case EnemyMotion::EightXZ: {
			// XZ 平面の 8 の字（リサージュ）
			float ax = std::cos(enemyTime_);
			float az = std::sin(2.0f * enemyTime_);    // 2倍速で位相差
			pos.x = ax * enemyRadius_;
			pos.z = az * enemyRadius_ * 0.8f;         // 縦横比を少し潰す
			pos.y = enemyBaseY_ + std::sin(enemyTime_ * 1.3f) * enemyBobAmp_;

			// 進行方向でヨー計算
			float dx = -std::sin(enemyTime_);
			float dz = 2.0f * std::cos(2.0f * enemyTime_);
			yaw = std::atan2(dx, dz) + enemyYawOffset_;
			break;
		}
		case EnemyMotion::SineStrafe: {
			// 横振り＋手前/奥スラローム（ジグザグ寄り）
			pos.x = std::sin(enemyTime_ * 1.4f) * enemyRadius_;
			pos.z = std::cos(enemyTime_ * 0.7f) * (enemyRadius_ * 0.7f);
			pos.y = enemyBaseY_ + std::sin(enemyTime_ * 2.2f) * (enemyBobAmp_ * 0.6f);

			float dx = 1.4f * std::cos(enemyTime_ * 1.4f);
			float dz = -0.7f * std::sin(enemyTime_ * 0.7f);
			yaw = std::atan2(dx, dz) + enemyYawOffset_;
			break;
		}
		case EnemyMotion::Swoop: {
			// 周期的にカメラへスッと寄って戻る
			float phase = std::fmod(enemyTime_, kTwoPi_);
			// 0→1→0 の台形イージング
			float w = std::clamp(
				1.0f - std::abs(std::fmod(phase, kPi_) - kHalfPi_) / kHalfPi_,
				0.0f, 1.0f);
			float swoop = -enemyRadius_ * 0.6f * w; // 手前(−Z)に引き寄せ

			pos.x = std::cos(enemyTime_) * (enemyRadius_ * 0.6f);
			pos.z = std::sin(enemyTime_) * (enemyRadius_ * 0.6f) + swoop;
			pos.y = enemyBaseY_ + std::sin(enemyTime_ * 1.1f) * enemyBobAmp_;

			float dx = -std::sin(enemyTime_);
			float dz = std::cos(enemyTime_) + (swoop != 0.0f ? -0.6f * (w > 0.0f) : 0.0f);
			yaw = std::atan2(dx, dz) + enemyYawOffset_;
			break;
		}
		}

		e->SetTranslate(pos);
		e->SetRotate({ 0.0f, yaw, 0.0f });
		e->SetScale({ enemyScale_, enemyScale_, enemyScale_ });
		e->Update();
	}

	dirLight_->Update(); // 平行光源更新
	camera_->Update(); // カメラ更新
	sprite_->Update(); // タイトル画像更新

	if (rippleEffect_) {
		rippleEffect_->Update(dt_);
	}

	// SPACE / A でアイリス（閉）開始＋波紋
	if (!irisClosing_ && (TKM::Input::GetInstance()->TriggerKey(DIK_SPACE) ||
		TKM::Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A))) {

		irisClosing_ = true;

		// 画面中心から波紋。UV(0.5, 0.5)
		if (rippleEffect_) {
			TKM::WaterRippleEffect::RippleDesc d{};
			d.duration = 1.0f;

			// タイトル用（今のタイトル目線値があるならここに入れる）
			d.radiusMax = 0.857f;
			d.amplitude = 0.1f;
			d.frequency = 80.0f;
			d.width = 10.0f;
			d.color = { 1.0f, 1.0f, 1.0f };
			d.colorIntensity = 0.0f;
			/// 中心から波紋を発生
			rippleEffect_->Trigger({ 0.5f, 0.5f }, d); // 中心から波紋
		}
	}

	// アイリス（閉）更新
	if (irisClosing_) {
		irisScale_ = UpdateIrisScale(iris_.get(), irisTween_, 0.016f);

		if (irisTween_.Finished()) {
			irisClosing_ = false; // 状態を戻しておく（お好み）
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

	// === ImGui ===
	ImGui::Begin("Title Heli (Background)");
	// position
	ImGui::Text("Heli Position: (%.2f, %.2f, %.2f)", heli_->GetTranslate().x, heli_->GetTranslate().y, heli_->GetTranslate().z);
	ImGui::SliderFloat("Radius", &radius_, 0.0f, 30.0f);
	ImGui::SliderFloat("BaseY", &baseY_, -5.0f, 10.0f);
	ImGui::SliderFloat("Bob Amp", &bobAmp_, 0.0f, 5.0f);
	ImGui::SliderFloat("Speed", &speed_, 0.0f, 5.0f);
	ImGui::SliderFloat("Yaw Offset", &yawOffset_, -3.14f, 3.14f);
	ImGui::SliderFloat("Scale", &scale_, 0.1f, 5.0f);
	ImGui::Separator();
	ImGui::SliderFloat("Cam Dist", &camDist_, 2.0f, 60.0f);
	ImGui::SliderFloat("Cam Y", &camY_, -5.0f, 20.0f);
	if (ImGui::Button("Apply Camera")) {
		camera_->SetTranslate({ 0.0f, camY_, -camDist_ });
		camera_->Update();
	}
	ImGui::End();

#endif // USE_IMGUI
}

void TitleScene::Draw(){
	// 3Dは3Dでまとめて
	TKM::Object3dCommon::GetInstance()->DrawSetCommon();
	if (heli_) heli_->Draw(dxCommon_);
	for (auto& e : titleEnemies_) e->Draw(dxCommon_);
	if (skybox_) skybox_->Draw();

	// ---- ここで Sprite パイプラインに戻す ----
	TKM::SpriteCommon::GetInstance()->DrawSetCommon();
	if (sprite_) sprite_->Draw();     // タイトル画像
	if (iris_)  iris_->Draw();      // 白円(アイリス)

	TKM::ParticleManager::GetInstance()->Draw();
}