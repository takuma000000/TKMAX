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

//=============================================================
// GameSceneクラス
// ゲーム本編を管理するシーンクラス。
//=============================================================
class GameScene : public BaseScene {
public:
	GameScene(TKM::DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}
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
	/// <summary>
	/// Iris（開く）演出の更新を行います。
	/// </summary>
	/// <param name="center"></param>
	void SpawnFirework(const Vector3& center);
	// Getter==================================
	/// <summary>
	/// カメラのポインタを取得します。
	/// </summary>
	/// <returns></returns>
	TKM::Camera* GetCameraPtr() {
		if (useDebugCamera_ && debugCamera_) { return debugCamera_.get(); }
		return camera.get();
	}
	/// <summary>
	/// DirectXCommonのポインタを取得します。
	/// </summary>
	/// <returns></returns>
	TKM::DirectXCommon* GetDX() { return dxCommon; }
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
	TKM::DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;
	//======================================================================
	// カメラ / ライト / スカイボックス
	//======================================================================
	std::unique_ptr<TKM::Camera> camera = nullptr;

	std::unique_ptr<TKM::DebugCamera> debugCamera_ = nullptr; // デバッグカメラ
	bool useDebugCamera_ = false;                        // デバッグカメラ使用フラグ

	std::unique_ptr<DirectionalLight> directionalLight_ = nullptr;// ディレクショナルライト

	std::unique_ptr<TKM::Skybox> skybox_;// スカイボックス

	// --- カメラインロ用 ---
	bool  camIntroActive_ = false;   // いま回転中か
	bool  camIntroDone_ = false;   // 一度やったら終了
	Ease::Tween camYawTween_;        // ヨー回転用ツイーン(スカラー)
	float camIntroDuration_ = 1.2f;  // かけたい時間(秒)

	// 始点/終点角度（お好みで調整）
	float camYawStart_ = -1.2f;      // 開始時に横を向かせる（-約69度）
	float camYawEnd_ = 0.0f;       // 最終的に+Zを向く前提(=0)

	// ピッチを少しだけ変化させたいなら
	float camPitchStart_ = 0.12f;    // ほんのり俯瞰で始める
	float camPitchEnd_ = 0.05f;    // 少しだけ水平へ

	// スカイボックス回転
	static constexpr float kSkyRotSpeedX = 0.002f;
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
	std::unique_ptr<ParticleEmitter> particleEmitter = nullptr;

	// 風エフェクト用
	void  UpdateAirStreak(float dt);
	float airStreakTimer_ = 0.0f;

	// airStreak（風エフェクト）
	static constexpr float kAirBoxHalfWidth = 40.0f;
	static constexpr float kAirBoxHalfHeight = 25.0f;
	//======================================================================
	// アイリス開き演出（ゲーム開始）
	//======================================================================
	std::unique_ptr<TKM::Sprite> iris_ = nullptr;
	// Iris（開く）用
	bool   irisOpening_ = true;
	float  irisScale_ = 5.0f;
	float  irisStartScale_ = 0.0f;   // 開始スケール（覆った状態）
	float  irisEndScale_ = 0.0f;   // 最終スケール（Initializeでセット）
	float  irisMaxScale_ = 0.0f;   // 画面対角ベース

	Ease::Tween irisTween_;          // Iris用イージング
	bool        emitOpenBurst_ = true;  // 開いた瞬間にエフェクトを出すか
	std::unique_ptr<TKM::Sprite> irisShadow_ = nullptr; // Irisの影
	float emitOpenDelaySec_ = 0.7f;  // 開始から何秒遅らせるか（お好み）
	float emitOpenElapsed_ = 0.0f;  // 経過時間
	const float dt = 0.016f;           // 可変なら実測のdeltaTimeを使ってOK

	// アイリス演出時間
	static constexpr float kIrisDurationSec = 0.8f;
	//======================================================================
	// アイリス閉じ演出（タイトル戻り）
	//======================================================================
	// Iris閉じ（タイトル戻り用）
	bool        irisClosing_ = false;   // Iris閉じ中か
	Ease::Tween irisCloseTween_;             // Iris閉じ用イージング
	float       irisCloseScale_ = 0.0f;    // 閉じる最終スケール
	//======================================================================
	// 「ゲームスタート」スライドイン演出
	//======================================================================
	std::unique_ptr<TKM::Sprite> startSprite_;  // 「ゲームスタート」スプライト
	float startT_ = 0.0f;         // イージング進行度(0→1)
	bool  startSlideIn_ = false;        // スライド中フラグ
	bool  startVisible_ = false;        // 表示も最初はしない（演出終了後に出す）
	bool  startPlayed_ = false;        // 一度だけ出すためのフラグ

	Vector2 startStartPos_ = { WindowsAPI::kClientWidth + 400.0f, WindowsAPI::kClientHeight * 0.5f }; // 右外
	Vector2 startEndPos_ = { WindowsAPI::kClientWidth * 0.5f,  WindowsAPI::kClientHeight * 0.5f };  // 中央

	Ease::Tween startTween_;        // イージング
	float       startDuration_ = 1.0f;   // アニメ時間
	float       startHoldSec_ = 1.0f;   // 中央で静止して見せる時間(秒)
	float       startHoldElapsed_ = 0.0f;  // 経過
	bool        startFadeOut_ = false;  // フェードアウト中か
	float       startFadeSec_ = 0.6f;   // フェード時間(秒)
	float       startAlpha_ = 1.0f;   // 現在アルファ

	float startGlowAmp_ = 0.8f;   // どれだけ明るくオーバーシュートするか（0.3～0.8目安）
	float startGlowSpeed_ = 10.0f;  // 中央到達後の“呼吸”スピード
	bool  startGlowOn_ = true;   // ON/OFF

	// 「ゲームスタート」演出
	static constexpr float kStartSlideInSec = 1.0f;
	static constexpr float kStartHoldSec = 1.0f;
	static constexpr float kStartFadeSec = 0.6f;
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

	// クリア演出
	static constexpr float kPlayerFlyMinTime = 1.8f;
	static constexpr float kPlayerFlyDistance = 80.0f;
	//======================================================================
	// 花火演出
	//======================================================================
	// 花火用
	bool  emitFireworkPending_ = false; // 花火を出すか
	float emitFireworkDelaySec_ = 0.7f;  // 開始から何秒遅らせるか（お好み）
	float emitFireworkElapsed_ = 0.0f;  // 経過時間
	Vector3 lastEmitPos_ = { 0.0f, 0.0f, 0.0f }; // 最後にエフェクトを出した位置

	// 花火
	static constexpr int kFireworkBurstCount = 60;
	//======================================================================
	// ポストエフェクト（RadialBlur）
	//======================================================================
	// RadialBlur エフェクト
	std::unique_ptr<RadialBlurEffect> radialBlur_ = nullptr;
	// Vignetting エフェクト
	std::unique_ptr<VignettingEffect> vignetting_ = nullptr;
	// Fog エフェクト
	std::unique_ptr<FogEffect> fog_ = nullptr;
	// Aura エフェクト
	std::unique_ptr<AuraEffect> aura_ = nullptr;
	// WaterRipple エフェクト（波紋）
	std::unique_ptr<WaterRippleEffect> waterRipple_ = nullptr;
	//======================================================================
	// 時間制御
	//======================================================================
	TimeScaleController timeScale_; // 時間制御クラス
	bool clearSlowRequested_ = false; // クリアスロー要求フラグ
};