#pragma once

#include <memory>
#include <vector>

#include "Enemy.h"          // 敵そのもの
#include "EnemySpawner.h"   // 敵スポーンユーティリティ
#include "Player.h"        // プレイヤー
#include "camera/Camera.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "MidBossCore.h"
#include "CsvSpawnLoader.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

// =============================================================
// EnemyManagerクラス
// 敵全体の管理を行うクラス。
// =============================================================
class EnemyManager {
public:
	EnemyManager() = default;
	~EnemyManager() = default;

	// --- Wave 管理周りを追加 ---
	enum class WavePhase { W1, W2, W3, Done };

	/// <summary>
	/// 敵全体の初期化
	/// </summary>
	/// <param name="dx"></param>
	/// <param name="camera"></param>
	/// <param name="parent"></param>
	/// <param name="player"></param>
	void Initialize(DirectXCommon* dx, Camera* camera, BaseScene* parent, Player* player);

	/// <summary>
	/// 敵全体の更新
	/// </summary>
	/// <param name="dt"></param>
	void Update(float dt);

	/// <summary>
	/// 敵全体の描画
	/// </summary>
	/// <param name="dx"></param>
	void Draw(DirectXCommon* dx);

	/// <summary>
	/// デバッグ用ImGui表示
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// 敵リストをバインドします
	/// </summary>
	/// <param name="enemies"></param>
	/// <param name="defeatedEnemyCount"></param>
	/// <param name="maxEnemyCount"></param>
	void BindEnemies(std::vector<std::unique_ptr<Enemy>>* enemies,
		int* defeatedEnemyCount,
		int* maxEnemyCount);

	/// <summary>
	/// プレイヤーに最も近い敵を更新します
	/// </summary>
	void UpdateClosestEnemy();

	/// <summary>
	/// Wave 初期化（GameScene::InitializeWaves 相当）
	/// </summary>
	void InitializeWaves();

	/// <summary>
	/// 現在の wavePhase_ に応じて敵をスポーンします（GameScene::SpawnCurrentWave 相当）
	/// </summary>
	void SpawnCurrentWave();

	/// <summary>
	/// wavePhase_ を進めます（GameScene::GoToNextWave 相当）
	/// </summary>
	void GoToNextWave();

	/// <summary>
	/// 生存している敵が存在するかどうかを判定して返します。
	/// </summary>
	/// <returns>生存している敵が1体以上いる場合は true、そうでない場合は false を返します。</returns>
	bool HasAliveEnemies() const { return enemies_ && !enemies_->empty(); }

	/// <summary>
	/// 全Waveクリア済みかどうかを取得します
	/// </summary>
	/// <returns></returns>
	bool IsAllWavesCleared() const { return (wavePhase_ == WavePhase::Done) && (!enemies_ || enemies_->empty()); }

	/// <summary>
	/// 全Waveクリア済みかどうかを取得します
	/// </summary>
	/// <returns></returns>
	bool IsWaveDone() const { return wavePhase_ == WavePhase::Done; }

	/// <summary>
	/// デバッグ用：即座にボスWave（Done）へスキップします
	/// </summary>
	void SkipToBossWave();

	// Getter==========================================================================
	/// <summary>
	/// 敵リストを取得します
	/// </summary>
	/// <returns></returns>
	const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return *enemies_; }
	/// <summary>
	/// 現在の WavePhase を取得します
	/// </summary>
	/// <returns></returns>
	WavePhase GetWavePhase() const { return wavePhase_; }
	// ================================================================================
	// Setter==========================================================================
	/// <summary>
	/// カメラを設定します
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(Camera* camera) {
		cam_ = camera;
		for (auto& e : *enemies_) { // 敵全員にカメラをセット
			if (e) e->SetCamera(cam_); // 敵にもカメラをセット
		}
		if (midBossCore_) { // 蘇生核にもカメラをセット
			midBossCore_->SetCamera(cam_); // 蘇生核にもカメラをセット
		}
	}
	// ================================================================================

	/// <summary>
	/// Wave2の三角形編隊をスポーンします
	/// </summary>
	void SpawnWave2_Triangle();
	/// <summary>
	/// Wave2のライン編隊をスポーンします
	/// </summary>
	void SpawnWave2_Line();
	/// <summary>
	/// Wave2のファストカラム編隊をスポーンします
	/// </summary>
	void SpawnWave2_FastColumn();
private:
	/// <summary>
	/// 敵をプレイヤー向けにセットアップします
	/// </summary>
	/// <param name="e"></param>
	void SetupEnemyForPlayer(Enemy& e);

	//======================================================================
	// 基本参照・共通情報
	//======================================================================
	DirectXCommon* dx_ = nullptr;
	Camera* cam_ = nullptr;
	BaseScene* parent_ = nullptr;
	Player* player_ = nullptr;

	// GameScene 側の実体を「参照」するだけ
	std::vector<std::unique_ptr<Enemy>>* enemies_ = nullptr;
	int* defeatedEnemyCount_ = nullptr;
	int* maxEnemyCount_ = nullptr;

	// Wave 状態は EnemyManager が持つようにする
	WavePhase wavePhase_ = WavePhase::W1;

	const float dt = 1.0f / 60.0f; // 固定フレームレート想定
	//======================================================================
	// Wave1 関連
	//======================================================================
	// ───────── Wave1 用パラメータ ─────────
	float wave1SpawnTimer_ = 0.0f;   // 次の出現までのタイマー
	float wave1SpawnInterval_ = 1.5f;   // 出現間隔（秒相当）
	int   wave1MaxSimultaneous_ = 2;    // 同時に存在してよい敵の数
	int   wave1DefeatTarget_ = 5;    // このWaveで「倒すべき敵の数」
	/// <summary>
	/// Wave1の更新
	/// </summary>
	/// <param name="dt"></param>
	void UpdateWave1(float dt);
	/// <summary>
	/// Wave1の敵を1体スポーンします
	/// </summary>
	void SpawnWave1Enemy();     // Wave1敵1体スポーン
	//======================================================================
	// Wave2 関連
	//======================================================================
	// ───────── Wave2 用パラメータ ─────────
	int   wave2SubWave_ = 0;      // 0,1,2... の隊列番号
	float wave2WaitTimer_ = 0.0f;   // 待機タイマー
	float wave2WaitDuration_ = 1.5f;   // 好きな秒数にできる
	bool  wave2Waiting_ = false;  // 待機中フラグ
	/// <summary>
	/// Wave2の更新
	/// </summary>
	void UpdateWave2(float dt);
	/// <summary>
	/// Wave2のサブウェーブをスポーンします
	/// </summary>
	/// <param name="id"></param>
	void SpawnWave2SubWave(int id);
	//======================================================================
	// Wave3（中ボスステージ） 関連
	//======================================================================
	// ───────── Wave3（中ボスステージ） 用パラメータ ─────────
	// 中ボスが片方落ちたときに「蘇生核」を出して 5 秒間猶予を与える
	bool  wave3ReviveInProgress_ = false; // 蘇生フェーズ中かどうか
	float wave3CoreTimer_ = 0.0f;  // 核の経過時間
	float wave3CoreLifetime_ = 5.0f;  // 核が生きていれば蘇生成立（秒）
	int   wave3CoreHP_ = 5;     // 核のHP（あとで調整用）
	int   wave3PrevAliveMidBossCount_ = 0; // 前フレームの生存中中ボス数
	// 中ボスの定位置（左右 2 体）※必要ならあとで ImGui 化
	Vector3 wave3LeftPos_ = { -12.0f, 6.0f, 80.0f };
	Vector3 wave3RightPos_ = { 12.0f, 6.0f, 80.0f };
	/// <summary>
	/// Wave3の更新
	/// </summary>
	/// <param name="dt"></param>
	void UpdateWave3(float dt);
	/// <summary>
	/// Wave3の中ボスステージ用の中ボスをスポーンします
	/// </summary>
	void SpawnWave3MidBossStage();
	/// <summary>
	/// Wave3の蘇生核をスポーンします
	/// </summary>
	void SpawnWave3Core();
	/// <summary>
	/// Wave3の追加中ボス（左右どちらか）をスポーンします
	/// </summary>
	void SpawnWave3ExtraMidBoss();

	std::unique_ptr<MidBossCore> midBossCore_ = nullptr; // 蘇生核
	//======================================================================
	// デバッグ系フラグ
	//======================================================================
	bool freezeEnemies_ = false; // デバッグ用：敵移動停止フラグ



	using SpawnFn = void (EnemyManager::*)();
	using UpdateFn = void (EnemyManager::*)(float);

	struct WaveOps {
		SpawnFn spawn = nullptr;
		UpdateFn update = nullptr;
	};

	static const WaveOps kWaveOps_[4]; // W1,W2,W3,Done(=nullptr)

	// 各Waveの「開始処理」（今 switch の case に書いてた中身を移す）
	void BeginWave1();
	void BeginWave2();
	void BeginWave3();



	// csv_spawn 用
	bool useCsvSpawn_ = false;            // CSV駆動モード
	std::vector<SpawnEvent> spawnEvents_;
	size_t spawnCursor_ = 0;             // spawnEvents_ の現在位置
	float csvWaveTime_ = 0.0f;           // 現在Wave開始からの経過秒

	void LoadSpawnCsv();
	void ResetCsvForCurrentWave();
	void UpdateCsvSpawn(float dt);
	int  CurrentWaveIndex() const;      // W1->1, W2->2, W3->3
	void SpawnFromEvent(const SpawnEvent& e);
};