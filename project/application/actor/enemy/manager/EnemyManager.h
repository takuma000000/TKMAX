#pragma once

#include <memory>
#include <vector>
#include <array>

#include "Enemy.h"
#include "EnemyFactory.h"
#include "Player.h"
#include "Camera.h"
#include "Object3d.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "EnemyWaveConfig.h"
#include "BattleActorManagerBase.h"
#include "EnemyBullet.h"
#include "EnemyBarrier.h"
#include "BarrierCoreManager.h"

//=============================================================
// EnemyManagerクラス
// 敵全体の管理を担当するクラス。
// 敵の生成、更新、描画、Wave進行、敵弾管理、
// バリア戦や中ボス戦の進行管理などをまとめて扱います。
//=============================================================
class EnemyManager : public BattleActorManagerBase {
public:
	EnemyManager() = default;
	~EnemyManager() = default;

	//=============================================================
	// Waveの大まかな進行段階
	//=============================================================
	enum class WavePhase {
		W1,   // Wave1
		Done  // 全Wave終了
	};

	//=============================================================
	// Wave1の詳細段階
	//=============================================================
	// Wave1はさらに段階を分けて進行します。
	// BarrierBattle : 本隊が無敵の状態で、バリア戦を行う段階
	// ExposedBattle : バリア破壊後、本隊を撃破する段階
	//=============================================================
	enum class Wave1Phase {
		BarrierBattle, // 本隊は無敵、増援やコア対応を行う段階
		ExposedBattle, // バリア破壊後、本隊を倒す段階
	};

	//=============================================================
	// 基本処理
	//=============================================================
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
	/// 敵本体、敵弾、Wave進行、バリア、コア管理もここで更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt) override;

	/// <summary>
	/// 敵全体の描画処理を行います。
	/// 敵本体、敵弾、バリア、コア類を描画します。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx) override;

	/// <summary>
	/// デバッグ用ImGuiを表示します。
	/// </summary>
	void ImGuiDebug();

	//=============================================================
	// Wave制御
	//=============================================================
	/// <summary>
	/// Wave全体の初期化を行います。
	/// 各Waveの進行状態や管理値をセットアップします。
	/// </summary>
	void InitializeWaves();

	/// <summary>
	/// 現在のWave段階に応じた敵をスポーンします。
	/// </summary>
	void SpawnCurrentWave();

	/// <summary>
	/// Wave段階を次へ進めます。
	/// </summary>
	void GoToNextWave();

	/// <summary>
	/// デバッグ用：Waveを即座に完了状態へスキップします。
	/// </summary>
	void SkipToBossWave();

	/// <summary>
	/// プレイヤーに最も近い敵情報を更新します。
	/// ロックオンや追尾対象更新の補助に使用します。
	/// </summary>
	void UpdateClosestEnemy();

	//=============================================================
	// 状態確認
	//=============================================================
	/// <summary>
	/// 生存している敵が存在するかどうかを判定します。
	/// </summary>
	/// <returns>生存している敵が1体以上いる場合 true</returns>
	bool HasAliveEnemies() const { return !enemies_.empty(); }

	/// <summary>
	/// 全Waveがクリア済みかどうかを取得します。
	/// </summary>
	/// <returns>最終Waveまで完了し、かつ敵が存在しない場合 true</returns>
	bool IsAllWavesCleared() const {
		return (wavePhase_ == WavePhase::Done) && enemies_.empty();
	}

	/// <summary>
	/// Waveが完了状態かどうかを取得します。
	/// </summary>
	/// <returns>WaveがDoneなら true</returns>
	bool IsWaveDone() const { return wavePhase_ == WavePhase::Done; }

	/// <summary>
	/// Wave初期化が完了しているかどうかを取得します。
	/// </summary>
	/// <returns>初期化済みなら true</returns>
	bool IsWavesInitialized() const { return initializedWaves_; }

	/// <summary>
	/// Wave1バリアが有効かどうかを取得します。
	/// </summary>
	/// <returns>バリアが存在し、かつ有効なら true</returns>
	bool IsWave1BarrierActive() const { return wave1Barrier_ && wave1Barrier_->IsActive(); }

	//=============================================================
	// Getter
	//=============================================================
	/// <summary>
	/// 現在のWave段階を取得します。
	/// </summary>
	/// <returns>現在のWavePhase</returns>
	WavePhase GetWavePhase() const { return wavePhase_; }

	/// <summary>
	/// 撃破した敵の累計数を取得します。
	/// </summary>
	/// <returns>累計撃破数</returns>
	int GetDefeatedEnemyCount() const { return defeatedEnemyCount_; }

	/// <summary>
	/// 管理対象の最大敵数を取得します。
	/// </summary>
	/// <returns>想定している最大敵数</returns>
	int GetMaxEnemyCount() const { return maxEnemyCount_; }

	/// <summary>
	/// 敵リストを取得します。
	/// </summary>
	/// <returns>敵オブジェクトの配列</returns>
	const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return enemies_; }

	/// <summary>
	/// Wave1の特殊攻撃コア位置を取得します。
	/// </summary>
	/// <returns>特殊攻撃コアのワールド座標</returns>
	Vector3 GetWave1SpecialCorePosition_() const;

	/// <summary>
	/// Wave1のバリア中心位置を取得します。
	/// </summary>
	/// <returns>バリア中心のワールド座標</returns>
	Vector3 GetWave1BarrierCenter() const;

	/// <summary>
	/// Wave1のバリアサイズを取得します。
	/// AABBの半分サイズを返します。
	/// バリアが存在しない場合はゼロベクトルを返します。
	/// </summary>
	/// <returns>バリアの半サイズ</returns>
	Vector3 GetWave1BarrierSize() const;

	//=============================================================
	// Setter
	//=============================================================
	/// <summary>
	/// 使用するカメラを設定します。
	/// 敵本体や関連オブジェクトにも同じカメラを適用します。
	/// </summary>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera) override;

protected:
	/// <summary>
	/// カメラ変更時の処理を行います。
	/// EnemyManager配下の敵や関連オブジェクトにも新しいカメラを適用します。
	/// </summary>
	void OnCameraChanged() override;

private:
	//======================================================================
	// 内部メソッド
	//======================================================================
	/// <summary>
	/// 敵をプレイヤー対象の挙動向けにセットアップします。
	/// 追尾対象、攻撃対象、各種参照設定に使用します。
	/// </summary>
	/// <param name="e">セットアップ対象の敵</param>
	void SetupEnemyForPlayer(Enemy& e);

	/// <summary>
	/// これから敵を全消去することをプレイヤーへ通知します。
	/// ターゲット解除や参照切り替えの前処理に使用します。
	/// </summary>
	void NotifyPlayerBeforeClearEnemies_();

	//======================================================================
	// Wave開始・進行
	//======================================================================
	/// <summary>
	/// Wave1を開始します。
	/// </summary>
	void BeginWave1();

	/// <summary>
	/// Wave1の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1(float dt);

	//======================================================================
	// Wave1本隊
	//======================================================================
	/// <summary>
	/// Wave1本隊グループを生成します。
	/// </summary>
	void SpawnWave1Group();

	/// <summary>
	/// Wave1の円形隊列を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1CircleFormation_(float dt);

	/// <summary>
	/// Wave1本隊へ円隊列の目標位置を適用します。
	/// </summary>
	void ApplyWave1CircleTargets_();

	/// <summary>
	/// Wave1本隊全体の無敵状態を切り替えます。
	/// </summary>
	/// <param name="enable">trueで無敵化、falseで解除</param>
	void SetWave1AllInvincible_(bool enable);

	/// <summary>
	/// Wave1本隊の生存数を取得します。
	/// </summary>
	/// <returns>生存中の本隊数</returns>
	int CountAliveWave1Main_() const;

	/// <summary>
	/// Wave1本隊の停止状態を切り替えます。
	/// </summary>
	/// <param name="enable">trueで停止、falseで解除</param>
	void SetWave1MainFreeze_(bool enable);

	//======================================================================
	// Wave1特殊攻撃
	//======================================================================
	/// <summary>
	/// Wave1の散開攻撃を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1ScatterAttack_(float dt);

	/// <summary>
	/// Wave1の特殊攻撃サイクル全体を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1SpecialAttackCycle_(float dt);

	/// <summary>
	/// Wave1特殊攻撃コアの溜め処理を開始します。
	/// </summary>
	void BeginWave1SpecialCharge_();

	/// <summary>
	/// Wave1特殊攻撃コアの溜め更新を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1SpecialCharge_(float dt);

	/// <summary>
	/// Wave1特殊攻撃コア弾を発射します。
	/// </summary>
	void FireWave1SpecialCore_();

	/// <summary>
	/// Wave1特殊攻撃コアの溜め演出パーティクルを発生させます。
	/// </summary>
	void EmitWave1SpecialChargeParticles_();

	//======================================================================
	// Wave1バリア
	//======================================================================
	/// <summary>
	/// Wave1用バリアを初期化します。
	/// </summary>
	void InitializeWave1Barrier_();

	/// <summary>
	/// Wave1用バリアの位置や状態を更新します。
	/// </summary>
	void UpdateWave1Barrier_();

	/// <summary>
	/// Wave1用バリアの有効状態を切り替えます。
	/// </summary>
	/// <param name="active">trueで有効、falseで無効</param>
	void SetWave1BarrierActive_(bool active);

	/// <summary>
	/// Wave1バリア情報をプレイヤーへ同期します。
	/// プレイヤー弾判定や演出連携に使用します。
	/// </summary>
	void SyncWave1BarrierInfoToPlayer_();

	/// <summary>
	/// Wave1のバリア破壊処理を行います。
	/// </summary>
	void BreakWave1Barrier_();

	//======================================================================
	// バリアコア
	//======================================================================
	/// <summary>
	/// バリアコア管理クラスを初期化します。
	/// </summary>
	void InitializeBarrierCoreManager_();

	/// <summary>
	/// バリア破壊用コアを生成します。
	/// </summary>
	void SpawnBarrierCores_();

	/// <summary>
	/// バリア破壊用コアを全削除します。
	/// </summary>
	void ClearBarrierCores_();

	/// <summary>
	/// バリア破壊用コアが全て破壊されたかどうかを判定します。
	/// </summary>
	/// <returns>全破壊済みなら true</returns>
	bool AreAllBarrierCoresDestroyed_() const;

	//======================================================================
	// 敵弾
	//======================================================================
	/// <summary>
	/// 敵弾全体の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateEnemyBullets_(float dt);

	/// <summary>
	/// 敵弾を描画します。
	/// プレイヤーとの距離に応じた見た目補正もここで行います。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void DrawEnemyBullets_(TKM::DirectXCommon* dx);

	/// <summary>
	/// 敵弾とプレイヤーの当たり判定を行います。
	/// 衝突時はダメージ処理と無敵時間処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void CheckEnemyBulletPlayerCollision_(float dt);

	//======================================================================
	// 基本参照・設定データ
	//======================================================================
	EnemyWaveConfig waveConfig_{}; // 敵Wave設定データ

	//======================================================================
	// 敵・敵弾管理
	//======================================================================
	std::vector<std::unique_ptr<Enemy>> enemies_;            // 敵本体リスト
	std::vector<std::unique_ptr<EnemyBullet>> enemyBullets_; // 敵弾リスト

	//======================================================================
	// Wave全体状態
	//======================================================================
	WavePhase wavePhase_ = WavePhase::W1; // 現在のWave進行状態
	bool initializedWaves_ = false;       // Wave初期化済みフラグ
	int defeatedEnemyCount_ = 0;          // 累計撃破数
	int maxEnemyCount_ = 0;               // 想定最大敵数

	//======================================================================
	// Wave1進行状態
	//======================================================================
	Wave1Phase wave1Phase_ = Wave1Phase::BarrierBattle; // Wave1内の現在フェーズ
	float wave1PhaseTimer_ = 0.0f;                      // 現在フェーズ開始からの経過時間
	int wave1DefeatTarget_ = 10;                        // Wave1で撃破対象となる敵数
	int wave1MainDefeatedCount_ = 0;                    // Wave1本隊の撃破数
	bool wave1MainStopped_ = false;                     // 本隊停止中かどうか

	//======================================================================
	// Wave1本隊・隊列
	//======================================================================
	static constexpr int kWave1EnemyCount_ = 10;        // Wave1本隊の敵数
	float wave1FormationMoveSpeed_ = 0.22f;             // 隊列移動速度
	float wave1CircleRadius_ = 18.0f;                   // 円隊列の半径
	float wave1CircleAngularSpeed_ = 0.75f;             // 円隊列の回転速度（rad/sec）
	float wave1CircleAngle_ = 0.0f;                     // 現在の円回転角
	Vector3 wave1CircleCenter_ = { 0.0f, -3.0f, 80.0f }; // 円隊列の中心座標

	//======================================================================
	// Wave1特殊攻撃
	//======================================================================
	bool wave1SpecialCharging_ = false;          // 特殊攻撃コア溜め中フラグ
	Vector3 wave1SpecialCoreOffset_ = { 0.0f, 0.0f, 0.0f }; // 隊列中心から見た特殊攻撃コア位置オフセット
	float wave1SpecialChargeDuration_ = 2.2f;    // 特殊攻撃コアの溜め時間
	float wave1SpecialCoreStartScale_ = 0.55f;   // 特殊攻撃コアの初期スケール
	float wave1SpecialCoreEndScale_ = 5.2f;      // 特殊攻撃コアの最大スケール
	float wave1SpecialCoreShotSpeed_ = 1.6f;     // 特殊攻撃コア弾の発射速度
	float wave1SpecialCoreRadius_ = 2.8f;        // 特殊攻撃コア弾の当たり判定半径
	int   wave1SpecialCoreDamage_ = 2;           // 特殊攻撃コア弾のダメージ量
	EnemyBullet* wave1SpecialCoreBullet_ = nullptr; // 特殊攻撃コア弾への参照

	float wave1NormalShotInterval_ = 1.05f; // 通常攻撃の発射間隔
	float wave1NormalShotTimer_ = 0.0f;     // 通常攻撃用タイマー
	float wave1NormalBulletSpeed_ = 0.42f;  // 通常弾の移動速度

	//======================================================================
	// Wave1バリア
	//======================================================================
	bool wave1BarrierBroken_ = false;                 // バリア破壊済みかどうか
	std::unique_ptr<EnemyBarrier> wave1Barrier_ = nullptr; // Wave1用バリア本体
	Vector3 wave1BarrierOffset_ = { 0.0f, 0.0f, 0.0f };    // 隊列中心から見たバリア位置オフセット
	Vector3 wave1BarrierSize_ = { 23.0f, 23.0f, 11.0f };   // バリアサイズ
	bool wave1BarrierFollowCore_ = true;                   // バリアを中心対象へ追従させるか

	float wave1BarrierShaderFresnelPower_ = 2.0f;         // フレネル強度
	float wave1BarrierShaderBaseStrength_ = 0.55f;        // 中心付近の明るさ
	float wave1BarrierShaderRimStrength_ = 1.35f;         // 縁の明るさ
	float wave1BarrierShaderAlphaBase_ = 0.42f;           // 中心付近の透明度
	float wave1BarrierShaderAlphaRim_ = 0.95f;            // 縁の透明度
	Vector3 wave1BarrierShaderTint_ = { 1.0f, 1.0f, 1.0f }; // バリアの色味

	//======================================================================
	// バリアコア
	//======================================================================
	static constexpr int kBarrierCoreCount_ = 5; // バリア破壊用コア数
	std::unique_ptr<BarrierCoreManager> barrierCoreManager_ = nullptr; // バリアコア管理クラス

	//======================================================================
	// プレイヤー当たり判定
	//======================================================================
	float playerHitRadius_ = 2.2f;            // プレイヤーの簡易当たり判定半径
	float playerHitCooldown_ = 0.0f;          // 被弾後の連続ヒット防止タイマー
	float playerHitCooldownDuration_ = 0.45f; // 被弾後の猶予時間
	int enemyBulletDamage_ = 1;               // 通常敵弾のダメージ量

	//======================================================================
	// デバッグ
	//======================================================================
	bool freezeEnemies_ = false; // デバッグ用：敵停止フラグ

	//======================================================================
	// デルタタイム保持
	//======================================================================
	float dt_ = 0.016f; // 前回更新時の経過時間
};