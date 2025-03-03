#include "GameScene.h"

#include "imgui/imgui.h"

#include "MyMath.h"

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
	directionalLight_->Initialize({ 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, 1.0f);
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
	// ────────────────────────────────────────
	// 各オブジェクトの更新処理
	// ────────────────────────────────────────
	camera->Update();
	
	// camera->ImGuiDebug();
	sprite->Update();
	object3d->Update();
	//anotherObject3d->Update();
	// ライトの更新
	directionalLight_->Update();


	// ────────────────────────────────────────
	// 移動・回転（加算）・スケール更新
	// ────────────────────────────────────────

	//*-*-*-*-*-*-*-*-*-*-*
	// object3d
	//*-*-*-*-*-*-*-*-*-*-*
	//UpdateObjectTransform(object3d, { 0.0f, 0.0f, 0.0f }, { 0.0f,0.0f,0.0f }, { 5.0f, 5.0f, 5.0f });

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
	if (ImGui::DragFloat3("LightDir", &direction.x, 0.1f))
	{
		directionalLight_->SetDirection(MyMath::Normalize(direction));
	}
	ImGui::End();
}

void GameScene::Draw()
{

	//Draw
	SpriteCommon::GetInstance()->DrawSetCommon();
	Object3dCommon::GetInstance()->DrawSetCommon();

	sprite->Draw();  // textureSrvHandleGPU は必要に応じて設定
	object3d->Draw(dxCommon);
	//anotherObject3d->Draw(dxCommon);

	// ライト情報をシェーダーなどに適用（必要に応じて実装）
	Vector4 lightColor = directionalLight_->GetColor();
	Vector3 lightDirection = directionalLight_->GetDirection();
	float lightIntensity = directionalLight_->GetIntensity();
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// ゲーム内のサウンドをロード＆再生する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeAudio()
{
	AudioManager::GetInstance()->LoadSound("fanfare", "fanfare.wav");
	AudioManager::GetInstance()->PlaySound("fanfare");
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
	sprite = std::make_unique<Sprite>();
	sprite->Initialize(SpriteCommon::GetInstance(), dxCommon, "./resources/uvChecker.png");

	sprite->SetPosition({ -1000.0f, 0.0f });
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 必要な3Dモデルをロードする
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::LoadModels()
{
	ModelManager::GetInstance()->LoadModel("axis.obj", dxCommon);
	ModelManager::GetInstance()->LoadModel("sphere.obj", dxCommon);
}

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
// 3Dオブジェクトを作成し、初期化する
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
void GameScene::InitializeObjects()
{
	object3d = std::make_unique<Object3d>();
	object3d->Initialize(Object3dCommon::GetInstance(), dxCommon);
	object3d->SetModel("sphere.obj");

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
	camera->SetTranslate({ 0.0f,0.0f,-30.0f });

	object3d->SetCamera(camera.get());
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
