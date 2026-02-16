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
#include "EnemyWaveConfig.h"

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
	/// 敵全体の初期化を行います。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	/// <param name="parent">所属する親シーン</param>
	/// <param name="player">参照対象となるプレイヤー</param>
	void Initialize(TKM::DirectXCommon* dx, TKM::Camera* camera, TKM::BaseScene* parent, Player* player);
	/// <summary>
	/// 敵全体の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt);
	/// <summary>
	/// 敵全体の描画処理を行います。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx);
	/// <summary>
	/// ImGui によるデバッグ情報を表示します。
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// プレイヤーに最も近い敵情報を更新します。
	/// </summary>
	void UpdateClosestEnemy();
	/// <summary>
	/// Wave を初期化します（GameScene::InitializeWaves 相当）。
	/// </summary>
	void InitializeWaves();
	/// <summary>
	/// 現在の wavePhase_ に応じて敵をスポーンします（GameScene::SpawnCurrentWave 相当）。
	/// </summary>
	void SpawnCurrentWave();
	/// <summary>
	/// wavePhase_ を次へ進めます（GameScene::GoToNextWave 相当）。
	/// </summary>
	void GoToNextWave();
	/// <summary>
	/// 生存している敵が存在するかどうかを判定します。
	/// </summary>
	/// <returns>生存している敵が1体以上いる場合は true、それ以外は false</returns>
	bool HasAliveEnemies() const { return !enemies_.empty(); }
	/// <summary>
	/// デバッグ用：即座にボス Wave（Done）へスキップします。
	/// </summary>
	void SkipToBossWave();
	/// <summary>
	/// 全ての Wave がクリアされているかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsAllWavesCleared() const {
		return (wavePhase_ == WavePhase::Done) && enemies_.empty();
	}
	/// <summary>
	/// Wave が完了状態（Done）かどうかを取得します。
	/// </summary>
	/// <returns>Wave が Done の場合 true、それ以外は false</returns>
	bool IsWaveDone() const { return wavePhase_ == WavePhase::Done; }
	/// <summary>
	/// Wave が初期化されているかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsWavesInitialized() const { return initializedWaves_; }
	// Getter==========================================================================
	/// <summary>
	/// 現在の WavePhase を取得します
	/// </summary>
	/// <returns></returns>
	WavePhase GetWavePhase() const { return wavePhase_; }
	/// <summary>
	/// 撃破した敵の数を取得します
	/// </summary>
	/// <returns></returns>
	int GetDefeatedEnemyCount() const { return defeatedEnemyCount_; }
	/// <summary>
	/// 最大敵数を取得します
	/// </summary>
	/// <returns></returns>
	int GetMaxEnemyCount() const { return maxEnemyCount_; }
	/// <summary>
	/// 敵リストを取得します
	/// </summary>
	/// <returns></returns>
	const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return enemies_; }
	// ================================================================================
	// Setter==========================================================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// 敵およびミッドボス核にも同じカメラを適用します。
	/// </summary>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera) {
		cam_ = camera;
		for (auto& e : enemies_) {
			if (e) e->SetCamera(cam_);
		}
		if (midBossCore_) { // 蘇生核にもカメラをセット
			midBossCore_->SetCamera(cam_);
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
	/// 敵をプレイヤーを対象とした挙動用にセットアップします。
	/// </summary>
	/// <param name="e">セットアップ対象となる敵</param>
	void SetupEnemyForPlayer(Enemy& e);
	/// <summary>
	/// プレイヤーに対して、これから敵が全滅することを通知します。
	/// </summary>
	void NotifyPlayerBeforeClearEnemies_();

	//======================================================================
	// 基本参照・共通情報
	//======================================================================
	TKM::DirectXCommon* dx_ = nullptr;
	TKM::Camera* cam_ = nullptr;
	TKM::BaseScene* parent_ = nullptr;
	Player* player_ = nullptr;

	// Wave 状態は EnemyManager が持つようにする
	WavePhase wavePhase_ = WavePhase::W1;

	const float dt_ = 1.0f / 60.0f; // 固定フレームレート想定
	//======================================================================
	// Wave1 関連
	//======================================================================
	// ───────── Wave1 用パラメータ ─────────
	float wave1SpawnTimer_ = 0.0f;   // 次の出現までのタイマー
	float wave1SpawnInterval_ = 1.5f;   // 出現間隔（秒相当）
	int   wave1MaxSimultaneous_ = 2;    // 同時に存在してよい敵の数
	int   wave1DefeatTarget_ = 5;    // このWaveで「倒すべき敵の数」
	/// <summary>
	/// Wave1 の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
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
	int   wave2SubWaveCount_ = 3; // サブウェーブ数
	/// <summary>
	/// Wave2 の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave2(float dt);
	/// <summary>
	/// Wave2 のサブウェーブをスポーンします。
	/// </summary>
	/// <param name="id">スポーンするサブウェーブの識別子</param>
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
	float wave3AngryDuration_ = 8.0f; // 中ボス怒り時間
	// 中ボスの定位置（左右 2 体）※必要ならあとで ImGui 化
	Vector3 wave3LeftPos_ = { -12.0f, 6.0f, 80.0f };
	Vector3 wave3RightPos_ = { 12.0f, 6.0f, 80.0f };
	/// <summary>
	/// Wave3 の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
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
		SpawnFn spawn_ = nullptr;
		UpdateFn update_ = nullptr;
	};

	static const WaveOps kWaveOps_[4]; // W1,W2,W3,Done(=nullptr)

	/// <summary>
	/// Wave1の開始
	/// </summary>
	void BeginWave1();
	/// <summary>
	/// Wave2の開始
	/// </summary>
	void BeginWave2();
	/// <summary>
	/// Wave3の開始
	/// </summary>
	void BeginWave3();
	// =====================================================================
	// 敵ウェーブ設定データ
	// =====================================================================
	EnemyWaveConfig waveConfig_{}; // 敵ウェーブ設定データ
	bool waveConfigLoaded_ = false; // 敵ウェーブ設定データが読み込まれたかどうか
	// =====================================================================
	// 敵リスト（直持ち版、BindEnemies 未使用時用）
	// =====================================================================
	std::vector<std::unique_ptr<Enemy>> enemies_; // 敵リスト（直持ち版）
	int defeatedEnemyCount_ = 0; // 撃破数カウンタ
	int maxEnemyCount_ = 0; // 最大敵数カウンタ
	bool initializedWaves_ = false; // Wave 初期化済みフラグ
};