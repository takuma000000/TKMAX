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
#include "ParticleGroupsCatalog.h"
#include "Input.h"
#include "PauseMenuController.h"
#include "TextureCatalog.h"
#include "ModelCatalog.h"

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
	/// ゲーム本編シーンの3Dオブジェクトを描画します。
	/// </summary>
	void Draw3D() override;
	/// <summary>
	/// ゲーム本編シーンのSpriteを描画します。
	/// </summary>
	void DrawSprite() override;
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
	/// 敵弾をスポーンします。
	/// </summary>
	/// <param name="pos">弾を生成するワールド座標位置</param>
	/// <param name="dir">弾が進む正規化された方向ベクトル</param>
	/// <param name="speed">弾の移動速度</param>
	/// <param name="damage">プレイヤーに与えるダメージ量</param>
	/// <param name="lifeFrame">弾が消滅するまでのフレーム数</param>
	void SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame);
	/// <summary>
	/// アクティブなカメラを更新します。
	/// </summary>
	/// <returns></returns>
	TKM::Camera* UpdateActiveCamera();
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
	TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX共通（デバイス/コマンド等）
	TKM::SrvManager* srvManager_ = nullptr; // SRV管理
	//======================================================================
	// カメラ / ライト / スカイボックス
	//======================================================================
	std::unique_ptr<TKM::Camera> camera_ = nullptr;           // メインカメラ
	std::unique_ptr<TKM::DebugCamera> debugCamera_ = nullptr; // デバッグカメラ
	bool useDebugCamera_ = false;                              // デバッグカメラ使用フラグ
	std::unique_ptr<TKM::DirectionalLight> directionalLight_ = nullptr; // ディレクショナルライト
	std::unique_ptr<TKM::Skybox> skybox_ = nullptr;                      // スカイボックス
	//======================================================================
	// プレイヤー / 敵 / マネージャ
	//======================================================================
	std::unique_ptr<Player> player_ = nullptr; // プレイヤー
	std::unique_ptr<EnemyManager> enemyManager_ = nullptr; // 敵管理
	std::unique_ptr<BossManager>  bossManager_ = nullptr; // ボス管理
	bool enemiesInitialized_ = false; // 敵初期化済みフラグ
	bool requestInitEnemies_ = false; // 敵再初期化リクエスト
	//======================================================================
	// パーティクル / 風エフェクト
	//======================================================================
	std::unique_ptr<ParticleEmitter> particleEmitter_ = nullptr; // パーティクルエミッタ（デバッグ/汎用）
	float airStreakTimer_ = 0.0f; // 風エフェクト用タイマー
	//======================================================================
	// 時間制御
	//======================================================================
	TKM::TimeScaleController timeScale_; // タイムスケール制御
	static constexpr float dt_ = 0.016f; // 固定デルタタイム（仮）
	//======================================================================
	// 演出 / 画面制御（シーン内サブコントローラ）
	//======================================================================
	std::unique_ptr<TKM::FireworkController>      fireworkController_ = nullptr; // 花火演出
	std::unique_ptr<TKM::GameFlowController>      flow_ = nullptr; // ゲーム開始/遷移/ロック制御
	std::unique_ptr<TKM::UIController>            ui_ = nullptr; // UI制御
	std::unique_ptr<TKM::PostEffectController>    postFx_ = nullptr; // ポストエフェクト制御
	std::unique_ptr<TKM::ClearSequenceController> clearSeq_ = nullptr; // クリア演出シーケンス
	std::unique_ptr<TKM::PauseMenuController>     pause_ = nullptr;
	//======================================================================
	// 内部処理
	//======================================================================
	/// <summary>
	/// 風エフェクトを更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateAirStreak(float dt); // 風エフェクト更新
	/// <summary>
	/// フレーム更新の開始処理を行います。
	/// </summary>
	/// <param name="rawDt">生のデルタタイム（補間・スケール未適用、秒）</param>
	/// <param name="scaledDt">タイムスケール適用後のデルタタイム（参照で更新される、秒）</param>
	void BeginFrameUpdate(float& rawDt, float& scaledDt);
	/// <summary>
	/// ゲームフローの更新処理を行います。
	/// </summary>
	void UpdateFlow();
	/// <summary>
	/// 敵とウェーブのロジック更新を行います。
	/// </summary>
	/// <param name="scaledDt">タイムスケール適用後のデルタタイム（秒）</param>
	void UpdateEnemyAndWaveLogic(float scaledDt);
	/// <summary>
	/// ゲームプレイシステムの更新処理を行います。
	/// </summary>
	/// <param name="dt">生のデルタタイム（タイムスケール未適用、秒）</param>
	/// <param name="scaledDt">タイムスケール適用後のデルタタイム（秒）</param>
	void UpdateGameplaySystems(float dt, float scaledDt);
	/// <summary>
	/// トランジションとシーンチェンジの更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateTransitionsAndSceneChange(float dt);
	/// <summary>
	/// デバッグキーとリクエストの処理を行います。
	/// </summary>
	void HandleDebugKeysAndRequests();
	/// <summary>
	/// フレーム更新の終了処理を行います。
	/// </summary>
	void EndFrameUpdate();
};