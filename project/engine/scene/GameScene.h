#pragma once
#include "BaseScene.h"

#include <memory>
#include "engine/audio/AudioManager.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "srvManager.h"
#include "engine/2d/Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "Camera.h"
#include "Object3dCommon.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "DirectionalLight.h"
#include "engine/effect/ParticleManager.h"
#include "engine/effect/ParticlerEmitter.h"
#include "engine/func/math/Vector3.h"
#include <SkyBox.h>
#include "GameClearScene.h"
#include "SceneManager.h"

#include "Player.h"
#include "Enemy.h"
#include "EnemySpawner.h"

class GameScene : public BaseScene
{
public:
	GameScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}
	~GameScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

private:// ──────────────────── 初期化処理 ────────────────────

   // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   // ゲーム内のサウンドをロード＆再生
   // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeAudio();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// 必要なテクスチャをロード
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void LoadTextures();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// スプライトを作成し、初期化
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeSprite();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// 必要な3Dモデルをロード
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void LoadModels();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// 3Dオブジェクトを作成し、初期化
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeObjects();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// カメラを作成し、各オブジェクトに適用
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void InitializeCamera();

	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	// ImGui
	// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
	void ImGuiDebug();

private: // ──────────────────── 更新処理 ────────────────────

	/**
	* @brief 指定した Object3d の Transform を更新する
	* @param obj 更新対象の Object3d
	* @param translate 新しい座標
	* @param rotate 回転角の加算値
	* @param scale スケール値
	*
	* ───────────────────────────────────────────
	* obj の移動・回転・スケールをまとめて更新する
	* obj->SetTranslate(translate);
	* obj->SetRotate(obj->GetRotate() + rotate);
	* obj->SetScale(scale);
	* ───────────────────────────────────────────
	*/
	void UpdateObjectTransform(std::unique_ptr<Object3d>& obj, const Vector3& translate, const Vector3& rotate, const Vector3& scale);

	//メモリ使用量
	void UpdateMemory();

	void UpdateEnemies();
	void UpdateClosestEnemy();
	void InitializeEnemies();

	// Wave管理 ===
	enum class WavePhase { W1, W2, W3, Done };
	WavePhase wavePhase_ = WavePhase::W1;
	void SpawnCurrentWave();   // 現在のwavePhase_に応じてスポーン
	void GoToNextWave();       // wavePhase_を進める

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	std::unique_ptr<Sprite> sprite = nullptr;
	std::unique_ptr<Camera> camera = nullptr;

	//地面
	std::unique_ptr<Object3d> ground_ = nullptr;

	std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;// ディレクショナルライト

	static const int kMemoryHistorySize = 100;
	std::array<float, kMemoryHistorySize> memoryHistory_{}; // 過去のメモリ使用履歴（MB）
	int memoryHistoryIndex_ = 0;

	//パーティクル
	std::unique_ptr<ParticleEmitter> particleEmitter = nullptr;

	std::unique_ptr<Skybox> skybox_;// スカイボックス
	std::unique_ptr<Player> player_ = nullptr;
	std::vector<std::unique_ptr<Enemy>> enemies_;

	int defeatedEnemyCount_ = 0;// 倒した敵の数
	int maxEnemyCount_ = 0;// 最大敵数

	std::vector<std::unique_ptr<Object3d>> groundTiles_;
	float groundTileLen_ = 299.0f;   // ground.obj の奥行きに合わせて調整
	float groundScroll_ = 0.6f;     // 前進感の速さ
	float groundOffset_ = 0.0f;     // スクロール用オフセット
};

