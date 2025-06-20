#include "GameScene.h"

#include "externals/imgui/imgui.h"

#include <algorithm>
#include "MyMath.h"
#include <psapi.h>
#include <Input.h>
#include "externals/nlohmann/json.hpp"
#include <fstream>

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

	struct LevelData {
		// オブジェクト1個分のデータ
		struct ObjectData {
			std::string fileName; // ファイル名
			Vector3 translation;
			Vector3 rotation;
			Vector3 scaling;
		};
		// オブジェクトのコンテナ
		std::vector<ObjectData> objects;
	};

	// 連結してフルパスを得る
	const std::string fullPath = std::string("resources/levels/") + "untitled.json";
	// ファイルストリーム
	std::ifstream file;
	// ファイルを開く
	file.open(fullPath);
	// ファイルオープン失敗をチェック
	if (file.fail()) {
		assert(0);
	}

	// JSON文字から解凍したデータ
	nlohmann::json deserialized;
	// 解凍
	file >> deserialized;
	// 正しいレベルエディタファイルかチェック
	assert(deserialized.isObject());
	assert(deserialized.contains("name"));
	assert(deserialized["name"].is_string());
	// "name"を文字列として取得
	std::string name = deserialized["name"].get<std::string>();
	// 正しいレベルデータファイルかチェック
	assert(name.compare("scene") == 0);

	// レベルデータ格納用インスタンスを生成
	LevelData* levelData = new LevelData();
	// "objects"の全オブジェクトを走査
	for (nlohmann::json& object : deserialized["objects"]) {
		assert(object.contains("type"));
		// 種別を取得
		std::string type = object["type"].get<std::string>();
		// 種類ごとの処理
		// MESH
		if (type.compare("MESH") == 0) {
			// 要素追加
			levelData->objects.emplace_back(LevelData::ObjectData());
			// 今追加した要素の参照を得る
			LevelData::ObjectData& objectData = levelData->objects.back();
			if (object.contains("file_name")) {
				// ファイル名
				objectData.fileName = object["file_name"];
			}
			// トランスフォームのパラメータ読み込み
			nlohmann::json transform = object["transform"];
			// 平行移動
			objectData.translation.x = (float)transform["translation"][0];
			objectData.translation.y = (float)transform["translation"][2];
			objectData.translation.z = (float)transform["translation"][1];
			// 回転角
			objectData.rotation.x = (float)transform["rotation"][0];
			objectData.rotation.y = (float)transform["rotation"][2];
			objectData.rotation.z = (float)transform["rotation"][1];
			// スケーリング
			objectData.scaling.x = (float)transform["scaling"][0];
			objectData.scaling.y = (float)transform["scaling"][2];
			objectData.scaling.z = (float)transform["scaling"][1];
		}
		// 再帰処理
			// TODO: オブジェクト走査を再帰関数にまとめ、再帰呼出で枝を走査する
		if (object.contains("children")) {

		}
	}
	// レベルエディタからオブジェクトを生成、配置
	for (auto& objectData : levelData->objects) {
		// ファイル名から登録済みモデルを検索
		Model* model = nullptr;
		decltype(models)::iterator it = models.find(objectData.fileName);
		if (it != models.end()) { model = it->second; }
		// モデルを指定して3Dオブジェクトを生成
		Object3d* newObject = new Object3d();
		newObject->SetModel(objectData.fileName);
		newObject->SetTranslate(objectData.translation);
		newObject->SetRotate(objectData.rotation);
		newObject->SetScale(objectData.scaling);

		newObject->Initialize(Object3dCommon::GetInstance(), dxCommon);

		objects.push_back(newObject);
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
	for (Object3d* object : objects) {
		object->
	}

	ResetDrawCallCount();
	UpdateMemory(); // メモリ使用量の更新

	// ────────────────────────────────────────
	// 各オブジェクトの更新処理
	// ────────────────────────────────────────
	camera->Update();

	// camera->ImGuiDebug();
	object3d->Update();
	ground_->Update();
	//anotherObject3d->Update();
	// ライトの更新
	directionalLight_->Update();

	UpdatePerformanceInfo(); // FPSの更新

	// ────────────────────────────────────────
	// 移動・回転（加算）・スケール更新
	// ────────────────────────────────────────

	//*-*-*-*-*-*-*-*-*-*-*
	// object3d
	//*-*-*-*-*-*-*-*-*-*-*
	ImGuiDebug();
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

	object3d->Draw(dxCommon);
	ground_->Draw(dxCommon);
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

void GameScene::ImGuiDebug()
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
