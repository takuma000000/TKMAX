#include "GameScene.h"

#include "externals/imgui/imgui.h"

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

	// 共通初期化
	object3dCommon_ = Object3dCommon::GetInstance();
	object3dCommon_->Initialize(dxCommon);

	spriteCommon_ = SpriteCommon::GetInstance(); // スプライト共通部
	spriteCommon_->Initialize(dxCommon);

	input_ = std::make_unique<Input>();
	input_->Initialize(nullptr); // ※必要なら WindowsAPI 渡す

	// プレイヤー初期化
	player_ = std::make_unique<Player>();
	player_->Initialize(object3dCommon_, dxCommon, camera.get(), input_.get(), spriteCommon_);
	player_->SetCamera(camera.get());
	player_->SetSpriteCommon(spriteCommon_);

	// 弾スプライト生成
	for (int i = 0; i < 15; i++) {
		auto sprite = std::make_unique<Sprite>();
		sprite->Initialize(spriteCommon_, dxCommon, "./resources/bullet.png");
		sprite->SetPosition({ 20.0f + i * 30.0f, 20.0f });
		sprite->SetSize({ 0.5f, 0.5f });
		bulletSprites_.push_back(std::move(sprite));
	}

	// HP表示
	for (int i = 0; i < 3; i++) {
		auto heart = std::make_unique<Sprite>();
		heart->Initialize(spriteCommon_, dxCommon, "./resources/heart_CR.png");
		heart->SetPosition({ 850.0f - i * 50.0f, 600.0f }); // 画面右下
		heartSprites_.push_back(std::move(heart));
	}

	// 敵の配置位置
	std::vector<Vector3> enemyPositions = {
		{ 20.0f, 0.0f, 50.0f },
		{ 0.0f, 8.0f, 50.0f },
		{ -20.0f, 0.0f, 50.0f },
		{ 0.0f, -8.0f, 50.0f },
		{ 20.0f, 8.0f, 50.0f },
		{ -20.0f, 8.0f, 50.0f },
		{ 20.0f, -8.0f, 50.0f },
		{ -20.0f, -8.0f, 50.0f }
	};

	for (const auto& pos : enemyPositions) {
		auto enemy = std::make_unique<Enemy>();
		enemy->Initialize(object3dCommon_, dxCommon, camera.get(), pos);
		enemy->SetCamera(camera.get());
		enemies_.push_back(std::move(enemy));
	}


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
	ResetDrawCallCount();
	UpdateMemory(); // メモリ使用量の更新

	// ────────────────────────────────────────
	// 各オブジェクトの更新処理
	// ────────────────────────────────────────
	camera->Update();
	
	// camera->ImGuiDebug();
	//object3d->Update();
	//ground_->Update();
	//anotherObject3d->Update();
	// ライトの更新
	directionalLight_->Update();

	UpdatePerformanceInfo(); // FPSの更新

	// Player更新
	if (player_) {
		std::vector<Enemy*> enemyPtrs;
		for (const auto& enemy : enemies_) {
			enemyPtrs.push_back(enemy.get());
		}

		player_->Update(enemyPtrs);

		// 弾スプライト更新
		int bulletCount = player_->GetBulletCount();
		for (int i = 0; i < bulletSprites_.size(); i++) {
			bulletSprites_[i]->SetVisible(i < bulletCount);
			bulletSprites_[i]->Update();
		}

		// HPスプライト更新
		int hp = player_->GetHP();
		for (int i = 0; i < heartSprites_.size(); i++) {
			heartSprites_[i]->SetVisible(i < hp);
			heartSprites_[i]->Update();
		}
	}


	// 敵の更新＆削除
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		auto& enemy = *it;
		if (enemy->IsDead()) {
			it = enemies_.erase(it);
		} else {
			enemy->Update(player_.get());
			++it;
		}
	}

	// 敵弾とプレイヤーの当たり判定
	for (const auto& enemy : enemies_) {
		for (const auto& bullet : enemy->GetBullets()) {
			float distance = MyMath::Distance(player_->GetTranslate(), bullet->GetPosition());
			float threshold = 1.0f;
			if (distance < threshold && !player_->IsInvincible()) {
				player_->OnCollision();
				bullet->Deactivate();
				break;
			}
		}
	}

	// プレイヤー弾と敵の当たり判定
	for (auto& enemy : enemies_) {
		for (const auto& bullet : player_->GetBullets()) {
			float enemySize = (enemy->GetScale().x + enemy->GetScale().y + enemy->GetScale().z) / 3.0f;
			float threshold = enemySize * 0.5f;
			float distance = MyMath::Distance(enemy->GetPosition(), bullet->GetPosition());
			if (distance < threshold) {
				enemy->OnCollision();
				bullet->Deactivate();
				break;
			}
		}
	}



	// ────────────────────────────────────────
	// 移動・回転（加算）・スケール更新
	// ────────────────────────────────────────

	//*-*-*-*-*-*-*-*-*-*-*
	// object3d
	//*-*-*-*-*-*-*-*-*-*-*
	//UpdateObjectTransform(object3d, { 0.0f, 0.0f, 00.0f }, { 0.0f,1.6f,0.0f }, { 1.0f, 1.0f, 1.0f });
								  ///   translate       ///     rotate      ///       scale       ///

	//UpdateObjectTransform(ground_, { 0.0f, 0.0f, 0.0f }, { 0.0f,1.6f,0.0f }, { 1.0f, 1.0f, 1.0f });

}

void GameScene::Draw()
{

	// === DSVを明示的にバインド ===
	/*D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = dxCommon->GetCurrentRTVHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon->GetDSVHandle();
	dxCommon->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);*/

	//Draw
	SpriteCommon::GetInstance()->DrawSetCommon();
	Object3dCommon::GetInstance()->DrawSetCommon();

	if (player_) {
		player_->Draw();

		for (auto& sprite : bulletSprites_) {
			sprite->Draw();
		}

		for (auto& sprite : heartSprites_) {
			sprite->Draw();
		}
	}

	for (const auto& enemy : enemies_) {
		enemy->Draw();
	}


	//object3d->Draw(dxCommon);
	//ground_->Draw(dxCommon);
	//anotherObject3d->Draw(dxCommon);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// ゲーム内のサウンドをロード＆再生する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeAudio()
{
	//AudioManager::GetInstance()->LoadSound("fanfare", "fanfare.wav");
	//AudioManager::GetInstance()->PlaySound("fanfare");
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
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// スプライトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeSprite()
{
	
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

	object3d = std::make_unique<Object3d>();
	object3d->Initialize(Object3dCommon::GetInstance(), dxCommon);
	object3d->SetModel("sphere.obj");
	object3d->SetParentScene(this);

	//地面
	ground_ = std::make_unique<Object3d>();
	ground_->Initialize(Object3dCommon::GetInstance(), dxCommon);
	ground_->SetModel("terrain.obj");
	ground_->SetParentScene(this);

	/*anotherObject3d = std::make_unique<Object3d>();
	anotherObject3d->Initialize(Object3dCommon::GetInstance(), dxCommon);
	anotherObject3d->SetModel("plane.obj");*/
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// カメラを作成し、各オブジェクトに適用する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeCamera()
{
	//Object3d共通部の初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ 0.0f,0.0f,0.0f });
	camera->SetTranslate({ 0.0f,2.5f,-25.0f });

	object3d->SetCamera(camera.get());
	ground_->SetCamera(camera.get());
	//anotherObject3d->SetCamera(camera.get());
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

void GameScene::ImGui()
{
	Vector3 object3dRotate = object3d->GetRotate();
	Vector3 object3dTranslate = object3d->GetTranslate();
	Vector3 object3dScale = object3d->GetScale();

	Vector3 direction = directionalLight_->GetDirection();


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
	ImGui::Separator();
	ImGui::Text("Memory Usage Graph (MB)");
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

	ImGui::End();
}
