#pragma once
#include "BaseScene.h"
#include "SystemIncludes.h"
#include "engine/effect/ParticleManager.h"
#include "engine/effect/ParticlerEmitter.h"

#include <memory>
#include "engine/2d/Sprite.h"
#include "Object3d.h"
#include "Camera.h"
#include "SkyBox.h"

class GameScene : public BaseScene
{
public:
	GameScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}
	~GameScene() = default;

	// -------------------- 初期化・終了 --------------------
	void Initialize() override;
	void Finalize() override;

	// -------------------- 更新・描画 --------------------
	void Update() override;
	void Draw() override;

private:
	// -------------------- 初期化内部処理 --------------------
	void InitializeAudio();     // サウンド
	void LoadTextures();        // テクスチャ
	void InitializeSprite();    // スプライト
	void LoadModels();          // モデル
	void InitializeObjects();   // 3Dオブジェクト
	void InitializeCamera();    // カメラ

	// -------------------- デバッグUI・状態更新 --------------------
	void ImGuiDebug();          // ImGui表示
	void UpdateMemory();        // メモリ使用量更新

private:
	// -------------------- システム参照 --------------------
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	// -------------------- ゲームオブジェクト --------------------
	std::unique_ptr<Sprite> sprite = nullptr;// スプライト
	std::unique_ptr<Camera> camera = nullptr;// カメラ
	std::unique_ptr<Object3d> object3d = nullptr;// 3Dオブジェクト
	std::unique_ptr<Object3d> ground_ = nullptr;// 地面オブジェクト
	std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;// ディレクショナルライト
	std::unique_ptr<ParticleEmitter> particleEmitter = nullptr;// パーティクルエミッター
	std::unique_ptr<Skybox> skybox_ = nullptr;// スカイボックス

	// -------------------- メモリ情報 --------------------
	static const int kMemoryHistorySize = 100;// メモリ履歴のサイズ
	std::array<float, kMemoryHistorySize> memoryHistory_{};// メモリ使用量履歴
	int memoryHistoryIndex_ = 0;// メモリ履歴のインデックス
};