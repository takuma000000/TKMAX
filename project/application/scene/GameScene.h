#pragma once
#include "BaseScene.h"

#include <memory>
#include "AudioManager.h"
#include "TextureManager.h"
#include "DirectXCommon.h"
#include "srvManager.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Object3d.h"
#include "camera/Camera.h"
#include "camera/DebugCamera.h"
#include "Object3dCommon.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "DirectionalLight.h"
#include "ParticleManager.h"
#include "ParticlerEmitter.h"
#include "Vector3.h"
#include <SkyBox.h>
#include "GameClearScene.h"
#include "TitleScene.h"
#include "SceneManager.h"
#include "manager/EnemyManager.h"
#include "manager/BossManager.h"
#include "LineRenderer.h"
#include "RadialBlurEffect.h"
#include "VignettingEffect.h"
#include "FogEffect.h"
#include "AuraEffect.h"
#include "IrisUtil.h"
#include "Player.h"
#include "Enemy.h"
#include "EnemySpawner.h"
#include "BossEnemy.h"
#include "BossBullet.h"
#include <Easing.h>
#include "TimeScaleController.h"
#include "WaterRippleEffect.h"
#include "FogVolume3D.h"
#include "SmokeVolume3D.h"
#include "RBGaugeUI.h"
#include "IntroSequence.h"
#include "FireworkController.h"

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
	/// <summary>
	/// クリア演出シーケンスの更新を行います。
	/// </summary>
	/// <param name="dt"></param>
	/// <returns></returns>
	bool UpdateClearSequence(float dt);
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

	// ゲーム開始ロック：true の間は敵/プレイヤー/弾など一切更新しない
	bool gameplayLocked_ = true;
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

	///
	// アイリス演出時間
	static constexpr float kIrisDurationSec_ = 0.8f;
	///
	//======================================================================
	// アイリス閉じ演出（タイトル戻り）
	//======================================================================
	// Iris閉じ（タイトル戻り用）
	bool        irisClosing_ = false;   // Iris閉じ中か
	Ease::Tween irisCloseTween_;             // Iris閉じ用イージング
	float       irisCloseScale_ = 0.0f;    // 閉じる最終スケール
	//======================================================================
	// プレイヤー死亡・ゲームオーバー遷移
	//======================================================================
	float playerDeathElapsed_ = 0.0f;   // プレイヤー死亡からの経過時間
	bool  playerDeathStarted_ = false;  // プレイヤー死亡処理開始フラグ
	//======================================================================
	// ゲームクリア演出
	//======================================================================
	// --- ゲームクリア演出用 ---
	bool clearSequence_ = false; // クリア演出中か

	enum class ClearPhase { None, CamZoom, PlayerFly, IrisClose }; // 演出フェーズ
	ClearPhase clearPhase_ = ClearPhase::None; // 現在のフェーズ

	float clearTimer_ = 0.0f; // フェーズ内タイマー

	// カメラ寄り用
	Vector3 clearCamStartPos_{};   // 開始位置
	Vector3 clearCamTargetPos_{};  // 目標位置

	// プレイヤー飛ばし用
	Vector3 clearPlayerStartPos_{};    // 開始位置
	float   clearPlayerSpeed_ = 10.0f;   // 奥に進むスピード
	float   clearPlayerFlyMinTime_ = 1.8f;    // プレイヤーを飛ばして見せる最低時間（秒）
	float   clearPlayerFlyDistance_ = 80.0f;   // Z方向に飛ばす距離目安
	//======================================================================
	// ポストエフェクト（RadialBlur）
	//======================================================================
	// RadialBlur エフェクト
	std::unique_ptr<TKM::RadialBlurEffect> radialBlur_ = nullptr;
	// Vignetting エフェクト
	std::unique_ptr<TKM::VignettingEffect> vignetting_ = nullptr;
	// Fog エフェクト
	std::unique_ptr<TKM::FogEffect> fog_ = nullptr;
	// Aura エフェクト
	std::unique_ptr<TKM::AuraEffect> aura_ = nullptr;
	// WaterRipple エフェクト（波紋）
	std::unique_ptr<TKM::WaterRippleEffect> waterRipple_ = nullptr;
	// FogVolume3D エフェクト
	std::unique_ptr<TKM::FogVolume3D> fogVolume3D_ = nullptr;
	// SmokeVolume3D エフェクト
	std::unique_ptr<TKM::SmokeVolume3D> smokeVolume3D_ = nullptr;
	//======================================================================
	// 時間制御
	//======================================================================
	TKM::TimeScaleController timeScale_; // 時間制御クラス
	bool clearSlowRequested_ = false; // クリアスロー要求フラグ

	// 操作ガイドUI
	std::unique_ptr<TKM::Sprite> uiLT_;
	std::unique_ptr<TKM::Sprite> uiLB_;
	std::unique_ptr<TKM::Sprite> uiRB_;

	std::unique_ptr<TKM::IntroSequence> intro_ = nullptr;

	std::unique_ptr<TKM::RBGaugeUI> rbGaugeUI_;

	std::unique_ptr<TKM::FireworkController> fireworkController_;

	static constexpr float dt_ = 0.016f;
};