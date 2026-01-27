#pragma once
#include "BaseScene.h"

#include "camera/DebugCamera.h"
#include "GameClearScene.h"
#include "TitleScene.h"
#include "manager/EnemyManager.h"
#include "UIController.h"
#include "MyMath.h"
#include "PostEffectController.h"
#include "ClearSequenceController.h"

//=============================================================
// GameSceneクラス
// ゲーム本編を管理するシーンクラス。
//=============================================================
class GameScene : public TKM::BaseScene {
public:
	GameScene(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager) : dxCommon_(dxCommon), srvManager_(srvManager) {}
	~GameScene() = default;

	/// <summary>
	/// ゲーム本編シーンを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// ゲーム本編シーンを終了します。
	/// </summary>
	void Finalize() override;
	/// <summary>
	/// ゲーム本編シーンを更新します。
	/// </summary>
	void Update() override;
	/// <summary>
	/// ゲーム本編シーンを描画します。
	/// </summary>
	void Draw() override;
	/// <summary>
	/// ImGuiでデバッグ表示を行います。
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// オーディオを初期化します。
	/// </summary>
	void InitializeAudio();
	/// <summary>
	/// スプライトを初期化します。
	/// </summary>
	void InitializeSprite();
	/// <summary>
	/// オブジェクトを初期化します。
	/// </summary>
	void InitializeObjects();
	/// <summary>
	/// カメラを初期化します。
	/// </summary>
	void InitializeCamera();

	/// <summary>
	/// モデルを読み込みます。
	/// </summary>
	void LoadModels();
	/// <summary>
	/// テクスチャを読み込みます。
	/// </summary>
	void LoadTextures();
	/// <summary>
	/// 敵弾をスポーンします。
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="dir"></param>
	/// <param name="speed"></param>
	/// <param name="damage"></param>
	/// <param name="lifeFrame"></param>
	void SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame);
	/// <summary>
	/// アクティブなカメラを更新します。
	/// </summary>
	/// <returns></returns>
	TKM::Camera* UpdateActiveCamera();
	/// <summary>
	/// クリア演出シーケンスを開始します。
	/// </summary>
	void StartClearSequence();
	// Getter==================================
	/// <summary>
	/// カメラのポインタを取得します。
	/// </summary>
	/// <returns></returns>
	TKM::Camera* GetCameraPtr() {
		if (useDebugCamera_ && debugCamera_) { return debugCamera_.get(); }
		return camera_.get();
	}
	/// <summary>
	/// DirectXCommonのポインタを取得します。
	/// </summary>
	/// <returns></returns>
	TKM::DirectXCommon* GetDX() { return dxCommon_; }
	/// <summary>
	/// プレイヤーのポインタを取得します。
	/// </summary>
	/// <returns></returns>
	Player* GetPlayerPtr() { return player_.get(); }
	// ========================================
private:
	//======================================================================
	// 基本システム
	//======================================================================
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::SrvManager* srvManager_ = nullptr;
	//======================================================================
	// カメラ / ライト / スカイボックス
	//======================================================================
	std::unique_ptr<TKM::Camera> camera_ = nullptr;

	std::unique_ptr<TKM::DebugCamera> debugCamera_ = nullptr; // デバッグカメラ
	bool useDebugCamera_ = false;                        // デバッグカメラ使用フラグ

	std::unique_ptr<TKM::DirectionalLight> directionalLight_ = nullptr;// ディレクショナルライト

	std::unique_ptr<TKM::Skybox> skybox_; // スカイボックス
	//======================================================================
	// プレイヤー / 敵 / マネージャ
	//======================================================================
	std::unique_ptr<Player> player_ = nullptr;

	std::vector<std::unique_ptr<Enemy>> enemies_;
	int defeatedEnemyCount_ = 0;// 倒した敵の数
	int maxEnemyCount_ = 0;// 最大敵数

	std::unique_ptr<EnemyManager> enemyManager_; // 敵管理クラス
	std::unique_ptr<BossManager>  bossManager_;  // ボス管理クラス

	bool requestInitEnemies_ = false; // 敵再初期化リクエスト

	// 敵初期化フラグ
	bool enemiesInitialized_ = false;
	//======================================================================
	// パーティクル / 風エフェクト
	//======================================================================
	//パーティクル
	std::unique_ptr<ParticleEmitter> particleEmitter_ = nullptr;

	// 風エフェクト用
	void  UpdateAirStreak(float dt);
	float airStreakTimer_ = 0.0f;
	//======================================================================
	// 時間制御
	//======================================================================
	TKM::TimeScaleController timeScale_; // 時間制御クラス

	std::unique_ptr<TKM::FireworkController> fireworkController_;

	static constexpr float dt_ = 0.016f;

	std::unique_ptr<TKM::GameFlowController> flow_ = nullptr;
	std::unique_ptr<TKM::UIController> ui_ = nullptr;
	std::unique_ptr<TKM::PostEffectController> postFx_ = nullptr;
	std::unique_ptr<TKM::ClearSequenceController> clearSeq_ = nullptr;
};