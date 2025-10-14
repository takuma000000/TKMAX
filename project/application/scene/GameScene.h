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
#include "engine/effect/light/DirectionalLight.h"
#include "engine/effect/particle/ParticleManager.h"
#include "engine/effect/particle/ParticlerEmitter.h"
#include "engine/func/math/Vector3.h"
#include <SkyBox.h>
#include "GameClearScene.h"
#include "SceneManager.h"

#include "application/player/Player.h"
#include "application/enemy/Enemy.h"
#include "application/enemy/EnemySpawner.h"
#include "application/boss/BossEnemy.h"
#include "application/boss/BossBullet.h"
#include <Easing.h>

class GameScene : public BaseScene
{
public:
	GameScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}
	~GameScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Update() override;
	void Draw() override;

	void SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame);
	Camera* GetCameraPtr() { return camera.get(); }
	DirectXCommon* GetDX() { return dxCommon; }
	Player* GetPlayerPtr() { return player_.get(); }

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

	void UpdateSkyboxRotationX(); // スカイボックスをX軸方向に回転
	void UpdateGroundScroll(); // 地面タイルのスクロール更新

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

	float skyPitch_ = 0.0f;        // X軸回転量
	float skyRotSpeedX_ = 0.002f;  // X軸回転速度

	bool bossBattle_ = false;         // ボス戦フラグ
	std::unique_ptr<BossEnemy> boss_; // ボス敵
	std::vector<std::unique_ptr<BossBullet>> bossBullets_;

	// Iris（開く）用
	std::unique_ptr<Sprite> iris_ = nullptr;
	bool   irisOpening_ = true;
	float  irisScale_ = 5.0f;
	float  irisSpeed_ = 3.2f;
	float  irisMin_ = 0.0f;

	// ★追加
	float  irisT_ = 0.0f;            // 進行度(0→1)
	float  irisDuration_ = 0.8f;     // アニメ全体の長さ(秒)
	float  irisStartScale_ = 0.0f;   // 開始スケール（覆った状態）
	float  irisEndScale_ = 0.0f;   // 最終スケール（Initializeでセット）
	float  irisMaxScale_ = 0.0f;   // 画面対角ベース

	Ease::Tween irisTween_; // Iris用イージング
	bool emitOpenBurst_ = true; // 開いた瞬間にエフェクトを出すか
	std::unique_ptr<Sprite> irisShadow_ = nullptr; // Irisの影
	float emitOpenDelaySec_ = 0.7f;  // 開始から何秒遅らせるか（お好み）
	float emitOpenElapsed_ = 0.0f;   // 経過時間
	const float dt = 0.016f; // 可変なら実測のdeltaTimeを使ってOK

	// 花火用
	bool  emitFireworkPending_ = false; // 花火を出すか
	float emitFireworkDelaySec_ = 0.7f; // 開始から何秒遅らせるか（お好み）
	float emitFireworkElapsed_ = 0.0f; // 経過時間
	Vector3 lastEmitPos_ = { 0.0f, 0.0f, 0.0f }; // 最後にエフェクトを出した位置

};

