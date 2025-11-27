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
#include "TitleScene.h"
#include "SceneManager.h"
#include "application/enemy/manager/EnemyManager.h"

#include "application/player/Player.h"
#include "application/enemy/Enemy.h"
#include "application/enemy/EnemySpawner.h"
#include "application/boss/BossEnemy.h"
#include "application/boss/BossBullet.h"
#include <Easing.h>

//=============================================================
// GameSceneクラス
// ゲーム本編を管理するシーンクラス。
//=============================================================
class GameScene : public BaseScene
{
public:
	GameScene(DirectXCommon* dxCommon, SrvManager* srvManager) : dxCommon(dxCommon), srvManager(srvManager) {}
	~GameScene() = default;

	/// <summary>ゲーム本編シーンを初期化します。</summary>
	void Initialize() override;
	/// <summary>ゲーム本編シーンを終了します。</summary>
	void Finalize() override;
	/// <summary>ゲーム本編シーンを更新します。</summary>
	void Update() override;
	/// <summary>ゲーム本編シーンを描画します。</summary>
	void Draw() override;

	/// <summary>ボス弾を生成して管理リストへ追加します。</summary>
	void SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame);
	/// <summary>最も近い敵を取得します。</summary>
	Camera* GetCameraPtr() { return camera.get(); }
	/// <summary>DirectXCommonを取得します。</summary>
	DirectXCommon* GetDX() { return dxCommon; }
	/// <summary>プレイヤーのポインタを取得します。</summary>
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

	//メモリ使用量
	/// <summary>メモリ使用量を計測・履歴化します。</summary>
	void UpdateMemory();

	/// <summary>スカイボックスのX回転を更新します。</summary>
	void UpdateSkyboxRotationX(); // スカイボックスをX軸方向に回転
	/// <summary>地面タイルのスクロールを更新します。</summary>
	//void UpdateGroundScroll(); // 地面タイルのスクロール更新

	/// <summary>Iris演出の更新を行います。</summary>
	void StartClearSequence();
	/// <summary>Iris演出の更新を行います。</summary>
	/// <param name="dt">デルタタイム。</param>
	bool UpdateClearSequence(float dt);
	/// <summary>「ゲームスタート」スプライトの更新を行います。</summary>
	void SpawnFirework(const Vector3& center);

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

	std::unique_ptr<Camera> camera = nullptr;
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

	std::unique_ptr<EnemyManager> enemyManager_; // 敵管理クラス

	float skyPitch_ = 0.0f;        // X軸回転量
	float skyRotSpeedX_ = 0.002f;  // X軸回転速度

	bool bossBattle_ = false;         // ボス戦フラグ
	std::unique_ptr<BossEnemy> boss_; // ボス敵
	std::vector<std::unique_ptr<BossBullet>> bossBullets_;
	bool bossP2BgmPlayed_ = false; // P2でBGMを1回だけ再生したかどうか

	// Iris（開く）用
	std::unique_ptr<Sprite> iris_ = nullptr;
	bool   irisOpening_ = true;
	float  irisScale_ = 5.0f;
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

	std::unique_ptr<Sprite> startSprite_;  // 「ゲームスタート」スプライト
	float startT_ = 0.0f;                  // イージング進行度(0→1)
	bool startSlideIn_ = false;             // スライド中フラグ
	bool startVisible_ = false; // 表示も最初はしない（演出終了後に出す）
	bool startPlayed_ = false; // 一度だけ出すためのフラグ
	Vector2 startStartPos_ = { WindowsAPI::kClientWidth + 400.0f, WindowsAPI::kClientHeight * 0.5f }; // 右外
	Vector2 startEndPos_ = { WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight * 0.5f };   // 中央
	Ease::Tween startTween_;               // イージング
	float startDuration_ = 1.0f;           // アニメ時間
	float startHoldSec_ = 1.0f;     // 中央で静止して見せる時間(秒)
	float startHoldElapsed_ = 0.0f; // 経過
	bool  startFadeOut_ = false;    // フェードアウト中か
	float startFadeSec_ = 0.6f;     // フェード時間(秒)
	float startAlpha_ = 1.0f;       // 現在アルファ

	float startGlowAmp_ = 0.8f;  // どれだけ明るくオーバーシュートするか（0.3～0.8目安）
	float startGlowSpeed_ = 10.0f; // 中央到達後の“呼吸”スピード
	bool  startGlowOn_ = true;  // ON/OFF

	// ゲーム開始ロック：true の間は敵/プレイヤー/弾など一切更新しない
	bool gameplayLocked_ = true;
	// 敵初期化フラグ
	bool enemiesInitialized_ = false;

	// --- カメラインロ用 ---
	bool  camIntroActive_ = false;   // いま回転中か
	bool  camIntroDone_ = false;   // 一度やったら終了
	Ease::Tween camYawTween_;        // ヨー回転用ツイーン(スカラー)
	float camIntroDuration_ = 1.2f;  // かけたい時間(秒)

	// 始点/終点角度（お好みで調整）
	float camYawStart_ = -1.2f;    // 開始時に横を向かせる（-約69度）
	float camYawEnd_ = 0.0f;     // 最終的に+Zを向く前提(=0)

	// ピッチを少しだけ変化させたいなら
	float camPitchStart_ = 0.12f;    // ほんのり俯瞰で始める
	float camPitchEnd_ = 0.05f;    // 少しだけ水平へ

	// Iris閉じ（タイトル戻り用）
	bool irisClosing_ = false; // Iris閉じ中か
	Ease::Tween irisCloseTween_; // Iris閉じ用イージング
	float irisCloseScale_ = 0.0f; // 閉じる最終スケール

	float playerDeathElapsed_ = 0.0f; // プレイヤー死亡からの経過時間
	bool playerDeathStarted_ = false; // プレイヤー死亡処理開始フラグ

	// --- ゲームクリア演出用 ---
	bool clearSequence_ = false; // クリア演出中か

	enum class ClearPhase { None, CamZoom, PlayerFly, IrisClose }; // 演出フェーズ
	ClearPhase clearPhase_ = ClearPhase::None; // 現在のフェーズ

	float clearTimer_ = 0.0f; // フェーズ内タイマー

	// カメラ寄り用
	Vector3 clearCamStartPos_{};   // 開始位置
	Vector3 clearCamTargetPos_{};  // 目標位置

	// プレイヤー飛ばし用
	Vector3 clearPlayerStartPos_{}; // 開始位置
	float   clearPlayerSpeed_ = 10.0f; // 奥に進むスピード
	float   clearPlayerFlyMinTime_ = 1.8f; // プレイヤーを飛ばして見せる最低時間（秒）
	float   clearPlayerFlyDistance_ = 80.0f; // Z方向に飛ばす距離目安

	// 風エフェクト用
	void UpdateAirStreak(float dt);
	float airStreakTimer_ = 0.0f;

	bool requestInitEnemies_ = false;

	// アイリス演出時間
	static constexpr float kIrisDurationSec = 0.8f;

	// スカイボックス回転
	static constexpr float kSkyRotSpeedX = 0.002f;

	// 「ゲームスタート」演出
	static constexpr float kStartSlideInSec = 1.0f;
	static constexpr float kStartHoldSec = 1.0f;
	static constexpr float kStartFadeSec = 0.6f;

	// クリア演出
	static constexpr float kPlayerFlyMinTime = 1.8f;
	static constexpr float kPlayerFlyDistance = 80.0f;

	// 花火
	static constexpr int   kFireworkBurstCount = 60;

	// airStreak（風エフェクト）
	static constexpr float kAirBoxHalfWidth = 40.0f;
	static constexpr float kAirBoxHalfHeight = 25.0f;
};