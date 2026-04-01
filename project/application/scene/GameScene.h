#pragma once
#include "BaseScene.h"

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
#include "CameraManager.h"
#include "BossEntranceSequence.h"

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
	/// アクティブなカメラのポインタを取得します。
	/// </summary>
	/// <returns></returns>
	TKM::Camera* GetCameraPtr() {
		return TKM::CameraManager::GetInstance()->GetActiveCamera();
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
	bool clearSequenceTriggered_ = false; // ボス撃破後、クリア開始を一度でも要求したら true
	//======================================================================
	// パーティクル / 風エフェクト
	//======================================================================
	std::unique_ptr<ParticleEmitter> particleEmitter_ = nullptr; // パーティクルエミッタ（デバッグ/汎用）
	float airStreakTimer_ = 0.0f; // 風エフェクト用タイマー
	//======================================================================
	// 時間制御
	//======================================================================
	TKM::TimeScaleController timeScale_; // タイムスケール制御
	static constexpr float kFixedDeltaTime_ = 0.016f; // 固定デルタタイム（秒） - タイムスケールの影響を受けない更新に使用
	//======================================================================
	// 演出 / 画面制御（シーン内サブコントローラ）
	//======================================================================
	std::unique_ptr<TKM::FireworkController>      fireworkController_ = nullptr; // 花火演出
	std::unique_ptr<TKM::GameFlowController>      flow_ = nullptr; // ゲーム開始/遷移/ロック制御
	std::unique_ptr<TKM::UIController>            ui_ = nullptr; // UI制御
	std::unique_ptr<TKM::PostEffectController>    postFx_ = nullptr; // ポストエフェクト制御
	std::unique_ptr<TKM::ClearSequenceController> clearSeq_ = nullptr; // クリア演出シーケンス
	std::unique_ptr<TKM::PauseMenuController>     pause_ = nullptr;
	std::unique_ptr<BossEntranceSequence>         bossEntranceSeq_ = nullptr; // WAVE3後のボス登場演出
	//======================================================================
	// 内部処理
	//======================================================================
	/// <summary>
	/// 風エフェクトを更新します。
	/// </summary>
	/// <param name="rawDeltaTime">前フレームからの経過時間（秒）</param>
	void UpdateAirStreak(float rawDeltaTime); // 風エフェクト更新
	/// <summary>
	/// フレーム更新の開始処理を行います。
	/// </summary>
	/// <param name="outRawDeltaTime">生のデルタタイム（補間・スケール未適用、秒）</param>
	/// <param name="outScaledDeltaTime">タイムスケール適用後のデルタタイム（参照で更新される、秒）</param>
	void BeginFrameUpdate(float& outRawDeltaTime, float& outScaledDeltaTime);
	/// <summary>
	/// ゲームフローの更新処理を行います。
	/// </summary>
	void UpdateFlow();
	/// <summary>
	/// 敵とウェーブのロジック更新を行います。
	/// </summary>
	/// <param name="scaledDeltaTime">タイムスケール適用後のデルタタイム（秒）</param>
	void UpdateEnemyAndWaveLogic(float scaledDeltaTime);
	/// <summary>
	/// ゲームプレイシステムの更新処理を行います。
	/// </summary>
	/// <param name="rawDeltaTime">生のデルタタイム（タイムスケール未適用、秒）</param>
	/// <param name="scaledDeltaTime">タイムスケール適用後のデルタタイム（秒）</param>
	void UpdateGameplaySystems(float rawDeltaTime, float scaledDeltaTime);
	/// <summary>
	/// トランジションとシーンチェンジの更新処理を行います。
	/// </summary>
	/// <param name="rawDeltaTime">前フレームからの経過時間（秒）</param>
	void UpdateTransitionsAndSceneChange(float rawDeltaTime);
	/// <summary>
	/// デバッグキーとリクエストの処理を行います。
	/// </summary>
	void HandleDebugKeysAndRequests();
	/// <summary>
	/// フレーム更新の終了処理を行います。
	/// </summary>
	void EndFrameUpdate();
	/// <summary>
	/// ポーズメニューを更新し、必要ならこのフレームの更新を早期終了します。
	/// </summary>
	/// <param name="rawDeltaTime">生のデルタタイム（秒）</param>
	/// <param name="allowPauseOpen">ロック中でないならtrue（ポーズを開ける）</param>
	/// <returns>このフレームを終了するならtrue</returns>
	bool TryUpdatePauseAndMaybeEarlyReturn_(float rawDeltaTime, bool allowPauseOpen);
	/// <summary>
	/// ポーズ中に動かすものだけ更新します（UI/遷移/デバッグ）。
	/// </summary>
	/// <param name="rawDeltaTime">生のデルタタイム（秒）</param>
	void UpdatePausedOnly_(float rawDeltaTime);
	/// <summary>
	/// 通常時のゲーム本体更新をまとめて行います。
	/// </summary>
	/// <param name="rawDeltaTime">生のデルタタイム（秒）</param>
	/// <param name="scaledDeltaTime">タイムスケール適用後のデルタタイム（秒）</param>
	void UpdateNormalGameplay_(float rawDeltaTime, float scaledDeltaTime);
};