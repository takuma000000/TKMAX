#define NOMINMAX
#include "GameScene.h"

#include "externals/imgui/imgui.h"
#include <limits>
#include <algorithm>
#include "MyMath.h"
#include <psapi.h>
#include <Input.h>

void GameScene::Initialize()
{
	// ──────────────── NULLチェック ────────────────
	assert(this != nullptr && "this is nullptr in GameScene::Initialize");
	assert(dxCommon != nullptr && "dxCommon is nullptr in GameScene::Initialize");

	// ──────────────── 各種初期化処理 ───────────────
	InitializeAudio();   // サウンドのロード＆再生
	LoadTextures();      // テクスチャのロード
	InitializeSprite();  // スプライトの作成＆初期化
	LoadModels();        // 3Dモデルのロード
	InitializeObjects(); // 3Dオブジェクトの作成＆初期化
	InitializeCamera();  // カメラの作成＆設定

	// ──────────────── ライトの初期化 ───────────────
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize({ 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);

	// ──────────────── パーティクルの初期化 ───────────────
	ParticleManager::GetInstance()->Initialize(dxCommon, srvManager, camera.get());
	ParticleManager::GetInstance()->CreateParticleGroup("uv", "./resources/circle.png", ParticleManager::ParticleType::NORMAL);
	particleEmitter = std::make_unique<ParticleEmitter>();
	particleEmitter->Initialize("uv", { 0.0f,2.5f,10.0f });

	// ──────────────── スカイボックスの初期化 ───────────────
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon, srvManager, "resources/rostock_laage_airport_4k.dds");
	skybox_->SetCamera(camera.get());
}

void GameScene::Finalize()
{
	// テクスチャマネージャーの終了
	TextureManager::GetInstance()->Finalize();

	// 終了処理
	AudioManager::GetInstance()->Finalize();

	// 3Dモデルマネージャーの終了
	ModelManager::GetInstance()->Finalize();
}

void GameScene::Update()
{
	Input::GetInstance()->Update();

	ResetDrawCallCount();
	UpdateMemory();

	camera->Update();
	ground_->Update();
	player_->Update();
	directionalLight_->Update();

	for (auto& enemy : enemies_) {
		enemy->Update();// 敵の更新
	}
	for (auto it = enemies_.begin(); it != enemies_.end(); ) {
		(*it)->Update();// 敵の更新

		if ((*it)->IsDead()) {
			player_->RemoveEnemyIfDead();
			it = enemies_.erase(it); // 死亡したら削除
		} else {
			++it;// 次の敵へ
		}
	}

	if (player_) {
		Enemy* closestEnemy = nullptr;
		float closestDistance = std::numeric_limits<float>::max();

		Vector3 playerPos = player_->GetPosition();

		for (auto& enemy : enemies_) {
			if (!enemy->IsDead()) {
				float dist = MyMath::Length(enemy->GetWorldPosition() - playerPos);
				if (dist < closestDistance) {
					closestDistance = dist;
					closestEnemy = enemy.get();
				}
			}
		}

		player_->SetEnemy(closestEnemy); // 生きてる最も近い敵を再設定
	}

	object3d->Update();
	ParticleManager::GetInstance()->Update();

	UpdatePerformanceInfo();
	ImGuiDebug();
}


void GameScene::Draw()
{
	SpriteCommon::GetInstance()->DrawSetCommon();
	Object3dCommon::GetInstance()->DrawSetCommon();

	ground_->Draw(dxCommon);
	player_->Draw(dxCommon);

	for (auto& enemy : enemies_) {
		enemy->Draw(dxCommon);
	}

	object3d->Draw(dxCommon);
	ParticleManager::GetInstance()->Draw();
	skybox_->Draw();
}


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// ゲーム内のサウンドをロード＆再生する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeAudio()
{
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要なテクスチャをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadTextures()
{
	//ファイルパス
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./resources/pokemon.png");
	TextureManager::GetInstance()->LoadTexture("./resources/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/sphere.png");
	TextureManager::GetInstance()->LoadTexture("./resources/gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("./resources/rostock_laage_airport_4k.dds");
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite()
{
	sprite = std::make_unique<Sprite>();
	sprite->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/circle.png");
	sprite->SetPosition({ -1000.0f, 0.0f });
	sprite->SetParentScene(this);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要な3Dモデルをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadModels()
{
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("terrain.obj", dxCommon);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeObjects()
{
	// object3d
	object3d = std::make_unique<Object3d>();
	object3d->Initialize(Object3dCommon::GetInstance(), dxCommon);
	object3d->SetModel("sphere.obj");
	object3d->SetParentScene(this);
	object3d->SetEnvironment("./resources/rostock_laage_airport_4k.dds");

	// ground
	ground_ = std::make_unique<Object3d>();
	ground_->Initialize(Object3dCommon::GetInstance(), dxCommon);
	ground_->SetModel("terrain.obj");
	ground_->SetParentScene(this);

	// player
	player_ = std::make_unique<Player>();
	player_->Initialize(Object3dCommon::GetInstance(), dxCommon);
	player_->SetPosition({ 0.0f, 0.0f, 0.0f });
	player_->SetParentScene(this);

	// enemies（5体をランダムに配置）
	const int enemyCount = 5;
	for (int i = 0; i < enemyCount; ++i) {
		auto enemy = std::make_unique<Enemy>();
		enemy->Initialize(Object3dCommon::GetInstance(), dxCommon);

		Vector3 pos = {
			static_cast<float>(rand() % 20 - 10), // X: -10〜10
			5.0f,                                 // Y: 地面から少し浮かせる
			20.0f + static_cast<float>(i * 5)     // Z: プレイヤーより奥へ（20, 25, 30...）
		};

		enemy->SetPosition(pos);
		enemy->SetParentScene(this);
		enemy->SetCamera(camera.get());

		enemies_.push_back(std::move(enemy));
	}

	// 1体だけプレイヤーに設定（必要なら複数対応へ）
	if (!enemies_.empty()) {
		player_->SetEnemy(enemies_.front().get());
	}
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを作成し、各オブジェクトに適用する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeCamera()
{
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,0.0f,-25.0f });

	object3d->SetCamera(camera.get());
	ground_->SetCamera(camera.get());
	player_->SetCamera(camera.get());

	for (auto& enemy : enemies_) {
		enemy->SetCamera(camera.get());
	}
}


void GameScene::ImGuiDebug()
{
	Vector3 object3dRotate = object3d->GetRotate();
	Vector3 object3dTranslate = object3d->GetTranslate();
	Vector3 object3dScale = object3d->GetScale();

	Vector3 direction = directionalLight_->GetDirection();

	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("Object3d");
	if (ImGui::DragFloat3("Object3dRotate", &object3dRotate.x, 0.01f))
	{
		object3d->SetRotate(object3dRotate);
	}
	if (ImGui::DragFloat3("Object3dTranslate", &object3dTranslate.x, 0.01f))
	{
		object3d->SetTranslate(object3dTranslate);
	}
	if (ImGui::DragFloat3("Object3dScale", &object3dScale.x, 0.01f))
	{
		object3d->SetScale(object3dScale);
	}
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("Info");
	ImGui::Text("FPS : %.2f", fps_);
	ImGui::Separator();
	ImGui::Text("FrameTime : %.2f ms", frameTimeMs_);
	ImGui::Separator();
	ImGui::Text("DrawCall : %d", drawCallCount_);
	ImGui::Separator();
	// メモリ使用量取得
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		// WorkingSetSize = 実際にメモリ上に展開されているサイズ
		size_t memoryUsageKB = pmc.WorkingSetSize / 1024; // KB
		size_t memoryUsageMB = memoryUsageKB / 1024; // MB
		ImGui::Text("Memory Usage : %zu KB / %zu MB", memoryUsageKB, memoryUsageMB);
	}
	ImGui::Text("MB");
	ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤色
	ImGui::PlotLines(
		"##MemoryPlot",
		memoryHistory_.data(),
		kMemoryHistorySize,
		memoryHistoryIndex_,
		nullptr,
		0.0f,
		500.0f,
		ImVec2(0, 150)
	);
	ImGui::PopStyleColor();
	ImGui::Separator();
	ImGui::Text("Active Sprite : %d", Sprite::GetActiveCount());
	ImGui::Text("Active Object3D : %d", Object3d::GetActiveCount());
	ImGui::Separator();
	int totalParticles = 0;
	for (const auto& pair : ParticleManager::GetInstance()->GetParticleGroups()) {
		totalParticles += static_cast<int>(pair.second.particles.size());
	}
	ImGui::Text("Active Particles: %d", totalParticles);
	ImGui::Text("ParticleGroup Count: %d", ParticleManager::GetInstance()->GetParticleGroups().size());
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	player_->ImGuiDebug();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	for (size_t i = 0; i < enemies_.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		enemies_[i]->ImGuiDebug();
		ImGui::PopID();
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("Bullet Debug");
	for (const auto& bullet : player_->GetBullets()) {
		ImGui::Text(bullet->IsHit() ? "true" : "false");
	}
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("ground");
	Vector3 groundTranslate = ground_->GetTranslate();
	Vector3 groundRotate = ground_->GetRotate();
	Vector3 groundScale = ground_->GetScale();
	if (ImGui::DragFloat3("Translate", &groundTranslate.x, 0.01f)) {
		ground_->SetTranslate(groundTranslate);
	}
	if (ImGui::DragFloat3("Rotate", &groundRotate.x, 0.01f)) {
		ground_->SetRotate(groundRotate);
	}
	if (ImGui::DragFloat3("Scale", &groundScale.x, 0.01f)) {
		ground_->SetScale(groundScale);
	}
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("Camera");
	/*Vector3 camPos = camera->GetTranslate();
	if (ImGui::DragFloat3("Position", &camPos.x, 0.1f, -50, 50.0f)) {
		camera->SetTranslate(camPos);
	}
	Vector3 camRot = camera->GetRotate();
	if (ImGui::DragFloat3("Rotation", &camRot.x, 0.1f, -30.0f, 30.0f)) {
		camera->SetRotate(camRot);
	}*/

	camera->ImGuiDebug();

	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
	ImGui::Begin("GamePad");
	auto DrawButtonBar = [](const char* label, bool isPressed, const ImVec4& color) {
		float value = isPressed ? 1.0f : 0.0f;
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
		ImGui::ProgressBar(value, ImVec2(200, 0), label);
		ImGui::PopStyleColor();
		};
	XINPUT_STATE state;
	DWORD result = XInputGetState(0, &state);
	if (result == ERROR_SUCCESS) {
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Controller Connected");
	} else {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Controller Not Found");
	}
	ImGui::Separator();
	DrawButtonBar("A", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A), ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // 緑
	DrawButtonBar("B", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_B), ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // 赤
	DrawButtonBar("X", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_X), ImVec4(0.0f, 0.4f, 1.0f, 1.0f)); // 青
	DrawButtonBar("Y", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_Y), ImVec4(1.0f, 0.4f, 0.7f, 1.0f)); // ピンク
	DrawButtonBar("Start", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_START), ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // グレー
	DrawButtonBar("Back", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_BACK), ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 白
	DrawButtonBar("LB", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_LEFT_SHOULDER), ImVec4(0.6f, 0.2f, 0.8f, 1.0f)); // 紫
	DrawButtonBar("RB", Input::GetInstance()->PushButton(XINPUT_GAMEPAD_RIGHT_SHOULDER), ImVec4(1.0f, 0.6f, 0.0f, 1.0f)); // オレンジ
	ImGui::End();
	///////////////////////////////////////////////////////////////////////////////////////////////////////
}

// ──────────────────────────────────────────────
// ● 指定した Object3d の Transform を更新する
// ──────────────────────────────────────────────
// - 更新対象の Object3d の座標・回転・スケールを設定）
// ──────────────────────────────────────────────
/**
 * @param obj       [in/out] 更新対象の Object3d
 * @param translate [in]     新しい座標
 * @param rotate    [in]     新しい回転角（加算処理なし）
 * @param scale     [in]     スケール値（直接上書き）
 */
void GameScene::UpdateObjectTransform(std::unique_ptr<Object3d>& obj, const Vector3& translate, const Vector3& rotate, const Vector3& scale)
{
	obj->SetTranslate(translate);
	obj->SetRotate(rotate);
	obj->SetScale(scale);
}

void GameScene::UpdateMemory()
{
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		// 現在のメモリ使用量（MB）
		float memoryUsageMB = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);

		// リングバッファで履歴を更新
		memoryHistory_[memoryHistoryIndex_] = memoryUsageMB;
		memoryHistoryIndex_ = (memoryHistoryIndex_ + 1) % kMemoryHistorySize;
	}
}
