#pragma once

#include <memory>
#include <vector>

#include "Enemy.h"
#include "EnemyFactory.h"
#include "Player.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "MidBossCore.h"
#include "EnemyWaveConfig.h"
#include "BattleActorManagerBase.h"
#include <array>
#include "EnemyBullet.h"

// =============================================================
// EnemyManagerクラス
// 敵全体の管理を行うクラス。
// =============================================================
class EnemyManager : public BattleActorManagerBase {
public:
	EnemyManager() = default;
	~EnemyManager() = default;

	// --- Wave 管理周りを追加 ---
	enum class WavePhase { W1, W2, W3, Done };
	// Wave1 の中の細かいフェーズ（編隊行動の段階）を定義
	enum class Wave1Phase {
		Scatter, // 散開して待つ
		FormUp,  // 隊列位置へ移動
		Hold,    // 隊列完成後に少し止まる
		Attack,  // 隊列状態で一斉攻撃
		Break,   // 解散して散開位置へ戻る
	};

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
	void Update(float dt) override;
	/// <summary>
	/// 敵全体の描画処理を行います。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx) override;
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
	/// <summary>
	/// Wave1の三角隊列の中心位置を取得します
	/// </summary>
	/// <returns></returns>
	Vector3 GetWave1SpecialCorePosition_() const;
	// ================================================================================
	// Setter==========================================================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// 敵およびミッドボス核にも同じカメラを適用します。
	/// </summary>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera) override;
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
	// Wave 状態は EnemyManager が持つようにする
	WavePhase wavePhase_ = WavePhase::W1;
	//======================================================================
	// Wave1 関連
	//======================================================================
	// ───────── Wave1 用パラメータ ─────────
	int   wave1DefeatTarget_ = 5;    // このWaveで「倒すべき敵の数」
	/// <summary>
	/// Wave1 の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1(float dt);
	//==============================================================
	// Wave1（新仕様：散開 → 隊列 → ホールド → 解散）
	//==============================================================
	static constexpr int kWave1EnemyCount_ = 5;

	Wave1Phase wave1Phase_ = Wave1Phase::Scatter;
	float wave1PhaseTimer_ = 0.0f;

	float wave1ScatterDuration_ = 2.0f;     // 散開している時間
	float wave1HoldDuration_ = 1.0f;        // 隊列完成後の静止時間
	float wave1BreakDuration_ = 1.5f;       // 解散移動の猶予時間
	float wave1FormationMoveSpeed_ = 0.22f; // 隊列移動速度
	float wave1BreakMoveSpeed_ = 0.18f;     // 解散時の移動速度

	Vector3 wave1FormationCenter_ = { 0.0f, 6.0f, 62.0f };

	Vector3 wave1SpecialCoreOffset_ = { 0.0f, 8.5f, -2.0f }; // 三角隊列のさらに上
	float wave1SpecialChargeDuration_ = 1.15f;               // 溜め時間
	float wave1SpecialCoreStartScale_ = 0.8f;                // 生成時の小ささ
	float wave1SpecialCoreEndScale_ = 4.6f;                  // 最大サイズ
	float wave1SpecialCoreShotSpeed_ = 1.6f;                // 発射速度
	float wave1SpecialCoreRadius_ = 2.8f;                    // 当たり判定半径
	int   wave1SpecialCoreDamage_ = 2;                       // SP弾ダメージ

	float wave1NormalShotInterval_ = 1.05f;                  // 散開中の通常攻撃間隔
	float wave1NormalShotTimer_ = 0.0f;                      // 散開中通常攻撃タイマー
	float wave1NormalBulletSpeed_ = 0.42f;                   // 散開中通常弾速度

	std::array<Vector3, kWave1EnemyCount_> wave1ScatterPositions_{};
	std::array<Vector3, kWave1EnemyCount_> wave1FormationPositions_{};

	float playerHitRadius_ = 2.2f;             // プレイヤーの簡易当たり判定半径
	float playerHitCooldown_ = 0.0f;           // 連続ヒット防止タイマー
	float playerHitCooldownDuration_ = 0.45f;  // 被弾後の猶予時間
	int   enemyBulletDamage_ = 1;              // 敵弾ダメージ

	EnemyBullet* wave1SpecialCoreBullet_ = nullptr;

	void SpawnWave1Group();
	void BuildWave1ScatterPositions_();
	void BuildWave1FormationPositions_();
	void ApplyWave1FormationTargets_();
	void ApplyWave1ScatterTargets_();
	bool AreAllWave1EnemiesInFormation_() const;
	void UpdateEnemyBullets_(float dt);
	void UpdateWave1ScatterAttack_(float dt);
	void BeginWave1SpecialCharge_();
	void UpdateWave1SpecialCharge_(float dt);
	void FireWave1SpecialCore_();
	void EmitWave1SpecialChargeParticles_();
	/// <summary>
	/// 敵弾を描画します。プレイヤーに近いほど明るく、遠いほど暗くなるように、距離に応じた色変化も加えます。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void DrawEnemyBullets_(TKM::DirectXCommon* dx);
	/// <summary>
	/// 敵弾とプレイヤーの当たり判定を行います。衝突していたらプレイヤーにダメージを与え、必要なら無敵時間も開始します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void CheckEnemyBulletPlayerCollision_(float dt);
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
	Vector3 wave3LeftPos_ = { -12.0f, 6.0f, 80.0f }; // 中ボスの定位置（右）
	Vector3 wave3RightPos_ = { 12.0f, 6.0f, 80.0f }; // 中ボスの定位置（左）
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

	using SpawnFn = void (EnemyManager::*)(); // スポーン関数のメンバ関数ポインタ型
	using UpdateFn = void (EnemyManager::*)(float); // 更新関数のメンバ関数ポインタ型

	struct WaveOps { // 各 Wave のスポーン関数と更新関数をまとめた構造体
		SpawnFn spawn_ = nullptr; // スポーン関数
		UpdateFn update_ = nullptr; // 更新関数
	};
	// WaveOps 配列のインデックスは WavePhase と対応させる（例: kWaveOps_[0] は WavePhase::W1 用）
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
	// =====================================================================
	// 敵リスト（直持ち版、BindEnemies 未使用時用）
	// =====================================================================
	std::vector<std::unique_ptr<Enemy>> enemies_; // 敵リスト（直持ち版）
	std::vector<std::unique_ptr<EnemyBullet>> enemyBullets_; // 敵弾リスト
	bool wave1SpecialCharging_ = false;             // 現在SP溜め中か
	int defeatedEnemyCount_ = 0; // 撃破数カウンタ
	int maxEnemyCount_ = 0; // 最大敵数カウンタ
	bool initializedWaves_ = false; // Wave 初期化済みフラグ

protected:
	/// <summary>
	/// カメラが変更されたときの処理を行います。EnemyManager と MidBossCore に新しいカメラを適用します。
	/// </summary>
	void OnCameraChanged() override;
};