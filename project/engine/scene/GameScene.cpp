#include "GameScene.h"

#include "externals/imgui/imgui.h"
#include <psapi.h>
#include <Input.h>

// シーンの初期化処理。各種リソースやオブジェクトの準備を行う
void GameScene::Initialize()
{
	assert(this != nullptr && "this is nullptr in GameScene::Initialize");
	assert(dxCommon != nullptr && "dxCommon is nullptr in GameScene::Initialize");

	InitializeAudio();      // サウンドの読み込み
	LoadTextures();         // テクスチャの読み込み
	InitializeSprite();     // スプライトの初期化
	LoadModels();           // 3Dモデルの読み込み
	InitializeObjects();    // 3Dオブジェクトの生成と設定
	InitializeCamera();     // カメラの生成と各オブジェクトへの設定

	// ディレクショナルライトの生成と初期設定
	directionalLight_ = std::make_unique<DirectionalLight>();
	directionalLight_->Initialize({ 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, 1.0f);

	// パーティクルの初期化とエミッターの設定
	ParticleManager::GetInstance()->Initialize(dxCommon, srvManager, camera.get());
	ParticleManager::GetInstance()->CreateParticleGroup("uv", "./resources/gradationLine.png", ParticleManager::ParticleType::CYLINDER);
	particleEmitter = std::make_unique<ParticleEmitter>();
	particleEmitter->Initialize("uv", { 0.0f, 2.5f, 10.0f });

	// スカイボックスの生成と初期化
	skybox_ = std::make_unique<Skybox>();
	skybox_->Initialize(dxCommon, srvManager, "resources/rostock_laage_airport_4k.dds");
}

// シーン終了処理。使用していたリソースを開放する
void GameScene::Finalize()
{
	TextureManager::GetInstance()->Finalize();
	AudioManager::GetInstance()->Finalize();
	ModelManager::GetInstance()->Finalize();
}

// 毎フレーム実行される更新処理。入力やカメラ、パーティクルの更新などを行う
void GameScene::Update()
{
	Input::GetInstance()->Update();           // 入力情報更新
	ResetDrawCallCount();                     // ドローコール数リセット
	UpdateMemory();                           // メモリ使用量更新

	particleEmitter->Update();                // パーティクルエミッター更新
	camera->Update();                         // カメラの更新
	sprite->Update();                         // スプライトの更新
	directionalLight_->Update();              // ライトの更新
	ParticleManager::GetInstance()->Update(); // パーティクル全体の更新
	UpdatePerformanceInfo();                  // FPS・描画回数などの更新
	ImGuiDebug();                             // ImGuiによるデバッグ表示
}

// 描画処理。スプライト、パーティクル、スカイボックスを描画する
void GameScene::Draw()
{
	SpriteCommon::GetInstance()->DrawSetCommon();    // スプライト共通描画準備
	Object3dCommon::GetInstance()->DrawSetCommon();  // 3Dオブジェクト共通描画準備

	sprite->Draw();                                  // スプライト描画
	ParticleManager::GetInstance()->Draw();          // パーティクル描画
	skybox_->Draw(camera->GetViewMatrix(), camera->GetProjectionMatrix()); // スカイボックス描画

	// （将来的なシェーダー適用のためにライト情報を取得）
	Vector4 lightColor = directionalLight_->GetColor();
	Vector3 lightDirection = directionalLight_->GetDirection();
	float lightIntensity = directionalLight_->GetIntensity();
}

// サウンドの読み込み処理。現在は未使用だが拡張可能
void GameScene::InitializeAudio()
{
}

// 必要なテクスチャをすべてロードする
void GameScene::LoadTextures()
{
	TextureManager::GetInstance()->LoadTexture("./resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./resources/pokemon.png");
	TextureManager::GetInstance()->LoadTexture("./resources/circle.png");
	TextureManager::GetInstance()->LoadTexture("./resources/sphere.png");
	TextureManager::GetInstance()->LoadTexture("./resources/gradationLine.png");
}

// スプライトの生成と初期化
void GameScene::InitializeSprite()
{
	sprite = std::make_unique<Sprite>();
	sprite->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/circle.png");
	sprite->SetPosition({ -1000.0f, 0.0f });
	sprite->SetParentScene(this);
}

// 使用する3Dモデルを読み込む
void GameScene::LoadModels()
{
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("terrain.obj", dxCommon);
}

// オブジェクトを生成してモデルやカメラを設定する
void GameScene::InitializeObjects()
{
	object3d = std::make_unique<Object3d>();
	object3d->Initialize(Object3dCommon::GetInstance(), dxCommon);
	object3d->SetModel("sphere.obj");
	object3d->SetParentScene(this);

	ground_ = std::make_unique<Object3d>();
	ground_->Initialize(Object3dCommon::GetInstance(), dxCommon);
	ground_->SetModel("terrain.obj");
	ground_->SetParentScene(this);
}

// カメラを生成し、オブジェクトに設定する
void GameScene::InitializeCamera()
{
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera->SetTranslate({ 0.0f, 0.0f, -0.0f });

	object3d->SetCamera(camera.get());
	ground_->SetCamera(camera.get());
}

// ImGuiを使ったデバッグ用UI表示処理
void GameScene::ImGuiDebug()
{
	// object3dのTransform調整
	Vector3 rotate = object3d->GetRotate();
	Vector3 pos = object3d->GetTranslate();
	Vector3 scale = object3d->GetScale();

	ImGui::Begin("Object3d");
	if (ImGui::DragFloat3("Rotate", &rotate.x, 0.01f)) object3d->SetRotate(rotate);
	if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) object3d->SetTranslate(pos);
	if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) object3d->SetScale(scale);
	ImGui::End();

	// パフォーマンス・メモリ情報の表示
	ImGui::Begin("Info");
	ImGui::Text("FPS : %.2f", fps_);
	ImGui::Text("FrameTime : %.2f ms", frameTimeMs_);
	ImGui::Text("DrawCall : %d", drawCallCount_);
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		size_t kb = pmc.WorkingSetSize / 1024;
		size_t mb = kb / 1024;
		ImGui::Text("Memory : %zu KB / %zu MB", kb, mb);
	}
	ImGui::End();

	// カメラのTransform調整
	ImGui::Begin("Camera");
	Vector3 camPos = camera->GetTranslate();
	if (ImGui::DragFloat3("Position", &camPos.x, 0.1f)) camera->SetTranslate(camPos);
	Vector3 camRot = camera->GetRotate();
	if (ImGui::DragFloat3("Rotation", &camRot.x, 0.1f)) camera->SetRotate(camRot);
	ImGui::End();

	// スカイボックス専用のImGui表示
	skybox_->ImGuiUpdate();
}

// メモリ使用履歴を更新する（ImGuiグラフ用）
void GameScene::UpdateMemory()
{
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
		float memMB = static_cast<float>(pmc.WorkingSetSize) / (1024.0f * 1024.0f);
		memoryHistory_[memoryHistoryIndex_] = memMB;
		memoryHistoryIndex_ = (memoryHistoryIndex_ + 1) % kMemoryHistorySize;
	}
}
