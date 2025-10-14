#define NOMINMAX
#include "TitleScene.h"
#include "Input.h"
#include "SceneManager.h"
#include <algorithm>
#include <cmath>
#include "externals/imgui/imgui.h"

void TitleScene::Initialize()
{
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera->SetTranslate({ 0.0f, camY_, -30.0f });

	TextureManager::GetInstance()->LoadTexture("./resources/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/circle2.png");
	TextureManager::GetInstance()->LoadTexture("./resources/title_kuraran.png");
	TextureManager::GetInstance()->LoadTexture("./resources/rostock_laage_airport_4k.dds");

	ModelManager::GetInstance()->LoadModel("jett.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("enemy.obj", dxCommon);

	heli_ = std::make_unique<Object3d>();
	heli_->Initialize(Object3dCommon::GetInstance(), dxCommon);
	heli_->SetModel("jett.obj");
	heli_->SetCamera(camera.get());
	heli_->SetScale({ scale_, scale_, scale_ });
	heli_->SetTranslate({ 0.0f, baseY_, 0.0f });

	sprite = std::make_unique<Sprite>();
	sprite->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/title_kuraran.png");
	// 画面中央に表示
	sprite->SetPosition({ 0.0f,0.0f });
	sprite->SetSize({ 1.0f, 1.0f });

	dirLight_ = std::make_unique<DirectionalLight>();
	dirLight_->Initialize({ 1,1,1,1 }, { 0.0f, -1.0f, 0.0f }, 1.0f);

	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon, srvManager, "resources/kloofendal_48d_partly_cloudy_puresky_1k.dds");
	skybox_->SetCamera(camera.get());

	// === Iris sprite (白円) ===
	iris_ = std::make_unique<Sprite>();
	// circle2.png が好みならこっちの行を使う： "./resources/circle2.png"
	iris_->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/circle2.png");
	iris_->SetPosition({ 0.0f, 0.0f });     // 画面中央
	iris_->SetSize({ irisScale_, irisScale_ }); // 最初は小さく
	iris_->SetColor({ 1,1,1,1 });           // 白・不透明

	// 画面中央に配置 & 対角長を最大に
	iris_->SetAnchorPoint({ 0.5f, 0.5f });
	iris_->SetPosition({ WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight * 0.5f });

	// 画面を覆うための最大スケール（対角）
	const float diag = std::sqrt(
		float(WindowsAPI::kClientWidth) * float(WindowsAPI::kClientWidth) +
		float(WindowsAPI::kClientHeight) * float(WindowsAPI::kClientHeight)
	);

	// ★余白に絶対負けない“核オプション”
	//   基本の対角に 1.8〜2.0 倍をかける。これで端がチラ見えする余地を潰す。
	irisMax_ = diag * 2.0f;   // ← まずは 2.0f。まだなら 2.2f に

	irisStartScale_ = irisScale_; // 最初のスケール（小さめ）
	irisEndScale_ = irisMax_;     // 最後に覆うサイズ


	// 開始サイズ & 速度（お好みで）
	irisScale_ = 10.0f;
	irisSpeed_ = 3500.0f;  // 速め

	iris_->SetSize({ irisScale_, irisScale_ });
	// 開始時にセット（覆い切るサイズを irisMax_ とする）
	irisTween_.Reset(/*start*/ irisScale_, /*end*/ irisMax_, /*sec*/ 0.8f, Ease::Type::InBack);

	// タイトル敵を1体だけ置く
	titleEnemies_.clear();
	{
		auto e = std::make_unique<Object3d>();
		e->Initialize(Object3dCommon::GetInstance(), dxCommon);
		e->SetModel("enemy.obj");
		e->SetCamera(camera.get());
		e->SetScale({ enemyScale_, enemyScale_, enemyScale_ });
		e->SetTranslate({ enemyRadius_, enemyBaseY_, 0.0f }); // 右前方あたり
		titleEnemies_.push_back(std::move(e));
	}

}

void TitleScene::Finalize()
{
	TextureManager::GetInstance()->Finalize();
}

void TitleScene::Update()
{


	ResetDrawCallCount();
	UpdatePerformanceInfo();

	Input::GetInstance()->Update();

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
			float phase = std::fmod(enemyTime_, 6.2831853f);
			// 0→1→0 の台形イージング
			float w = std::clamp(1.0f - std::abs(std::fmod(phase, 3.1415926f) - 1.5707963f) / 1.5707963f, 0.0f, 1.0f);
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

	dirLight_->Update();

	camera->Update();
	sprite->Update();

	// SPACE / A でアイリス（閉）開始
	if (!irisClosing_ && (Input::GetInstance()->TriggerKey(DIK_SPACE) ||
		Input::GetInstance()->TriggerButton(XINPUT_GAMEPAD_A))) {
		irisClosing_ = true;
	}

	if (irisClosing_) {
		irisScale_ = irisTween_.Update(0.016f); // 1フレーム分の進行
		iris_->SetSize({ irisScale_, irisScale_ });
		iris_->Update();

		if (irisTween_.Finished()) {
			sceneManager_->SetNextScene(new GameScene(dxCommon, srvManager));
			return;
		}
	}

	constexpr float kTwoPi = 6.2831853f;
	skyPitch_ -= skyRotSpeedX_;
	if (skyPitch_ > kTwoPi)  skyPitch_ -= kTwoPi;
	if (skyPitch_ < 0.0f)    skyPitch_ += kTwoPi;
	skybox_->SetRotation({ skyPitch_, 0.0f, 0.0f });

#ifdef _DEBUG
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
		camera->SetTranslate({ 0.0f, camY_, -camDist_ });
		camera->Update();

	}
	ImGui::End();
#endif // _DEBUG
}

void TitleScene::Draw()
{
	// 3Dは3Dでまとめて
	Object3dCommon::GetInstance()->DrawSetCommon();
	if (heli_) heli_->Draw(dxCommon);
	for (auto& e : titleEnemies_) e->Draw(dxCommon);
	if (skybox_) skybox_->Draw();

	// ---- ここで Sprite パイプラインに戻す ----
	SpriteCommon::GetInstance()->DrawSetCommon();
	if (sprite) sprite->Draw();     // タイトル画像
	if (iris_)  iris_->Draw();      // 白円(アイリス)
}
