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
#include "MidBossCore.h"
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
		W2,   // Wave2
		W3,   // Wave3（中ボスステージ）
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
		ExposedBattle, // バリア破壊後。本隊を倒す段階
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
	/// 敵本体、敵弾、Wave進行、中ボス核などもここで更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt) override;
	/// <summary>
	/// 敵全体の描画処理を行います。
	/// 敵本体、敵弾、バリア、コア類などの描画を行います。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dx) override;
	/// <summary>
	/// ImGui によるデバッグ情報を表示します。
	/// </summary>
	void ImGuiDebug();

	/// <summary>
	/// プレイヤーに最も近い敵情報を更新します。
	/// ロックオンや追尾系の補助情報更新に使用します。
	/// </summary>
	void UpdateClosestEnemy();
	/// <summary>
	/// Wave を初期化します。
	/// 各Waveの初期状態や管理値をセットアップします。
	/// </summary>
	void InitializeWaves();
	/// <summary>
	/// 現在の wavePhase_ に応じて敵をスポーンします。
	/// </summary>
	void SpawnCurrentWave();
	/// <summary>
	/// wavePhase_ を次へ進めます。
	/// </summary>
	void GoToNextWave();
	/// <summary>
	/// 生存している敵が存在するかどうかを判定します。
	/// </summary>
	/// <returns>生存している敵が1体以上いる場合は true、それ以外は false</returns>
	bool HasAliveEnemies() const { return !enemies_.empty(); }
	/// <summary>
	/// デバッグ用：即座にボスWave相当の完了状態へスキップします。
	/// </summary>
	void SkipToBossWave();
	/// <summary>
	/// 全ての Wave がクリアされているかどうかを取得します。
	/// </summary>
	/// <returns>最終Waveまで完了し、かつ敵が残っていなければ true</returns>
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
	/// <returns>初期化済みなら true</returns>
	bool IsWavesInitialized() const { return initializedWaves_; }
	/// <summary>
	/// Wave1のバリアがアクティブかどうかを取得します。
	/// </summary>
	/// <returns>バリアが存在し、かつ有効なら true</returns>
	bool IsWave1BarrierActive() const { return wave1Barrier_ && wave1Barrier_->IsActive(); }

	// Getter==========================================================================
	/// <summary>
	/// 現在の WavePhase を取得します。
	/// </summary>
	/// <returns>現在のWave段階</returns>
	WavePhase GetWavePhase() const { return wavePhase_; }
	/// <summary>
	/// 撃破した敵の数を取得します。
	/// </summary>
	/// <returns>現在の撃破数</returns>
	int GetDefeatedEnemyCount() const { return defeatedEnemyCount_; }
	/// <summary>
	/// 最大敵数を取得します。
	/// </summary>
	/// <returns>この管理対象で想定している最大敵数</returns>
	int GetMaxEnemyCount() const { return maxEnemyCount_; }
	/// <summary>
	/// 敵リストを取得します。
	/// </summary>
	/// <returns>敵オブジェクトの配列</returns>
	const std::vector<std::unique_ptr<Enemy>>& GetEnemies() const { return enemies_; }
	/// <summary>
	/// Wave1の特殊攻撃コア位置を取得します。
	/// </summary>
	/// <returns>Wave1特殊コアのワールド座標</returns>
	Vector3 GetWave1SpecialCorePosition_() const;
	/// <summary>
	/// Wave1のバリア中心位置を取得します。
	/// </summary>
	/// <returns>バリア中心のワールド座標</returns>
	Vector3 GetWave1BarrierCenter() const;
	/// <summary>
	/// Wave1のバリアのサイズを取得します。
	/// AABBの半分サイズを返します。
	/// バリアが存在しない場合はゼロベクトルを返します。
	/// </summary>
	/// <returns>バリアの半サイズ</returns>
	Vector3 GetWave1BarrierSize() const;
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
	/// Wave2の三角形編隊をスポーンします。
	/// </summary>
	void SpawnWave2_Triangle();
	/// <summary>
	/// Wave2のライン編隊をスポーンします。
	/// </summary>
	void SpawnWave2_Line();
	/// <summary>
	/// Wave2の高速カラム編隊をスポーンします。
	/// </summary>
	void SpawnWave2_FastColumn();

private:
	/// <summary>
	/// 敵をプレイヤー対象の挙動向けにセットアップします。
	/// 追尾・攻撃対象・参照先などの設定に使います。
	/// </summary>
	/// <param name="e">セットアップ対象となる敵</param>
	void SetupEnemyForPlayer(Enemy& e);
	/// <summary>
	/// プレイヤーに対して、
	/// これから敵が全滅することを通知します。
	/// </summary>
	void NotifyPlayerBeforeClearEnemies_();

	//=============================================================
	// 基本参照・共通情報
	//=============================================================
	WavePhase wavePhase_ = WavePhase::W1; // 現在のWave進行状態
	//=============================================================
	// Wave1 関連
	//=============================================================
	int wave1DefeatTarget_ = 10; // Wave1で撃破対象となる敵数
	/// <summary>
	/// Wave1 の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1(float dt);
	//=============================================================
	// Wave1（新仕様：バリア戦 + 本隊戦）
	//=============================================================
	Wave1Phase wave1Phase_ = Wave1Phase::BarrierBattle; // Wave1内の現在フェーズ
	float wave1PhaseTimer_ = 0.0f; // 現在フェーズに入ってからの経過時間

	float wave1FormationMoveSpeed_ = 0.22f; // 隊列移動速度

	Vector3 wave1SpecialCoreOffset_ = { 0.0f, 0.0f, 0.0f }; // 隊列中心から見た特殊攻撃コアの位置オフセット
	float wave1SpecialChargeDuration_ = 2.2f;                // 特殊攻撃コアの溜め時間
	float wave1SpecialCoreStartScale_ = 0.55f;               // 特殊攻撃コアの初期スケール
	float wave1SpecialCoreEndScale_ = 5.2f;                  // 特殊攻撃コアの最大スケール
	float wave1SpecialCoreShotSpeed_ = 1.6f;                 // 特殊攻撃コア弾の発射速度
	float wave1SpecialCoreRadius_ = 2.8f;                    // 特殊攻撃コア弾の当たり判定半径
	int   wave1SpecialCoreDamage_ = 2;                       // 特殊攻撃コア弾のダメージ量

	float wave1NormalShotInterval_ = 1.05f; // 通常攻撃の発射間隔
	float wave1NormalShotTimer_ = 0.0f;     // 通常攻撃の経過タイマー
	float wave1NormalBulletSpeed_ = 0.42f;  // 通常弾の速度

	float playerHitRadius_ = 2.2f;            // プレイヤーの簡易当たり判定半径
	float playerHitCooldown_ = 0.0f;          // 被弾直後の連続ヒット防止タイマー
	float playerHitCooldownDuration_ = 0.45f; // 被弾後の猶予時間
	int   enemyBulletDamage_ = 1;             // 通常敵弾のダメージ量

	EnemyBullet* wave1SpecialCoreBullet_ = nullptr; // Wave1特殊攻撃コア弾への参照

	/// <summary>
	/// Wave1の本隊グループをスポーンします。
	/// </summary>
	void SpawnWave1Group();
	/// <summary>
	/// 敵弾全体の更新を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateEnemyBullets_(float dt);
	/// <summary>
	/// Wave1の散開攻撃を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1ScatterAttack_(float dt);
	/// <summary>
	/// Wave1の特殊攻撃コアの溜め開始処理を行います。
	/// </summary>
	void BeginWave1SpecialCharge_();
	/// <summary>
	/// Wave1の特殊攻撃コアの溜め更新を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1SpecialCharge_(float dt);
	/// <summary>
	/// Wave1の特殊攻撃コアを発射します。
	/// </summary>
	void FireWave1SpecialCore_();
	/// <summary>
	/// Wave1の特殊攻撃コア溜め中パーティクルを発生させます。
	/// </summary>
	void EmitWave1SpecialChargeParticles_();

	float wave1CircleRadius_ = 18.0f;          // 円隊列の半径
	float wave1CircleAngularSpeed_ = 0.75f;    // 円隊列の回転速度（rad/sec）
	float wave1CircleAngle_ = 0.0f;            // 現在の円回転角
	Vector3 wave1CircleCenter_ = { 0.0f, -3.0f, 80.0f }; // 円隊列の中心座標

	/// <summary>
	/// Wave1の円形隊列を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1CircleFormation_(float dt);
	/// <summary>
	/// Wave1本隊に対して円隊列の目標位置を適用します。
	/// </summary>
	void ApplyWave1CircleTargets_();
	/// <summary>
	/// Wave1本隊全体の無敵状態を切り替えます。
	/// </summary>
	/// <param name="enable">trueで無敵、falseで解除</param>
	void SetWave1AllInvincible_(bool enable);

	std::unique_ptr<EnemyBarrier> wave1Barrier_ = nullptr; // Wave1用バリア本体
	Vector3 wave1BarrierOffset_ = { 0.0f, 0.0f, 0.0f };   // 隊列中心から見たバリア位置オフセット
	Vector3 wave1BarrierSize_ = { 23.0f, 23.0f, 11.0f };  // バリアのサイズ
	bool wave1BarrierFollowCore_ = true;                  // バリアを中心対象へ追従させるかどうか

	float wave1BarrierShaderFresnelPower_ = 2.0f;  // バリアのフレネル強度
	float wave1BarrierShaderBaseStrength_ = 0.55f; // バリア中心付近の明るさ
	float wave1BarrierShaderRimStrength_ = 1.35f;  // バリア縁の明るさ
	float wave1BarrierShaderAlphaBase_ = 0.42f;    // バリア中心付近の透明度
	float wave1BarrierShaderAlphaRim_ = 0.95f;     // バリア縁の透明度
	Vector3 wave1BarrierShaderTint_ = { 1.0f, 1.0f, 1.0f }; // バリアの色味

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
	/// Wave1バリア情報をプレイヤー側へ同期します。
	/// プレイヤー弾との判定や演出連携に使います。
	/// </summary>
	void SyncWave1BarrierInfoToPlayer_();

	static constexpr int kWave1EnemyCount_ = 10; // Wave1本隊の敵数
	static constexpr int kBarrierCoreCount_ = 5; // バリア破壊用コア数

	bool wave1BarrierBroken_ = false; // バリア破壊済みかどうか
	bool wave1MainStopped_ = false;   // 本隊を停止中かどうか

	int wave1MainDefeatedCount_ = 0;  // Wave1本隊の撃破数

	std::unique_ptr<BarrierCoreManager> barrierCoreManager_ = nullptr; // バリア破壊用コア群の管理クラス

	/// <summary>
	/// Wave1のバリア破壊処理を行います。
	/// </summary>
	void BreakWave1Barrier_();
	/// <summary>
	/// Wave1の特殊攻撃サイクル全体を更新します。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave1SpecialAttackCycle_(float dt);
	/// <summary>
	/// Wave1本隊の生存数を数えます。
	/// </summary>
	/// <returns>生存中の本隊数</returns>
	int CountAliveWave1Main_() const;
	/// <summary>
	/// Wave1本隊の行動停止状態を切り替えます。
	/// </summary>
	/// <param name="enable">trueで停止、falseで解除</param>
	void SetWave1MainFreeze_(bool enable);
	/// <summary>
	/// バリア破壊用コア管理クラスを初期化します。
	/// </summary>
	void InitializeBarrierCoreManager_();
	/// <summary>
	/// バリア破壊用コアをスポーンします。
	/// </summary>
	void SpawnBarrierCores_();
	/// <summary>
	/// バリア破壊用コアを全消去します。
	/// </summary>
	void ClearBarrierCores_();
	/// <summary>
	/// バリア破壊用コアが全て破壊されたかどうかを判定します。
	/// </summary>
	/// <returns>全破壊済みなら true</returns>
	bool AreAllBarrierCoresDestroyed_() const;
	/// <summary>
	/// 敵弾を描画します。
	/// プレイヤーに近いほど明るく、遠いほど暗くなるような距離補正も加えます。
	/// </summary>
	/// <param name="dx">DirectX 共通管理クラス</param>
	void DrawEnemyBullets_(TKM::DirectXCommon* dx);
	/// <summary>
	/// 敵弾とプレイヤーの当たり判定を行います。
	/// 衝突時はダメージ処理と無敵時間処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void CheckEnemyBulletPlayerCollision_(float dt);

	//=============================================================
	// Wave2 関連
	//=============================================================
	int   wave2SubWave_ = 0;         // 現在のサブWave番号
	float wave2WaitTimer_ = 0.0f;    // サブWave間の待機タイマー
	float wave2WaitDuration_ = 1.5f; // 待機時間
	bool  wave2Waiting_ = false;     // 待機中フラグ
	int   wave2SubWaveCount_ = 3;    // サブWave総数

	/// <summary>
	/// Wave2 の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave2(float dt);
	/// <summary>
	/// Wave2 の指定サブWaveをスポーンします。
	/// </summary>
	/// <param name="id">スポーンするサブWaveの識別子</param>
	void SpawnWave2SubWave(int id);

//=============================================================
	// Wave3（中ボスステージ） 関連
	//=============================================================
	bool  wave3ReviveInProgress_ = false; // 蘇生フェーズ中かどうか
	float wave3CoreTimer_ = 0.0f;         // 蘇生核の経過時間
	float wave3CoreLifetime_ = 5.0f;      // 蘇生核の存続時間
	int   wave3CoreHP_ = 5;               // 蘇生核のHP
	int   wave3PrevAliveMidBossCount_ = 0; // 前フレームの中ボス生存数
	float wave3AngryDuration_ = 8.0f;     // 中ボス怒り状態の継続時間

	Vector3 wave3LeftPos_ = { -12.0f, 6.0f, 80.0f };  // 左側中ボスの定位置
	Vector3 wave3RightPos_ = { 12.0f, 6.0f, 80.0f };  // 右側中ボスの定位置

	/// <summary>
	/// Wave3 の更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateWave3(float dt);

	/// <summary>
	/// Wave3の中ボスステージ用の中ボスをスポーンします。
	/// </summary>
	void SpawnWave3MidBossStage();

	/// <summary>
	/// Wave3の蘇生核をスポーンします。
	/// </summary>
	void SpawnWave3Core();

	/// <summary>
	/// Wave3の追加中ボスをスポーンします。
	/// </summary>
	void SpawnWave3ExtraMidBoss();

	std::unique_ptr<MidBossCore> midBossCore_ = nullptr; // Wave3の蘇生核

	//=============================================================
	// デバッグ系フラグ
	//=============================================================
	bool freezeEnemies_ = false; // デバッグ用：敵移動停止フラグ

	//=============================================================
	// Wave処理の関数テーブル
	//=============================================================
	using SpawnFn = void (EnemyManager::*)();        // Wave開始時のスポーン関数型
	using UpdateFn = void (EnemyManager::*)(float);  // Wave更新関数型

	/// <summary>
	/// 各Waveに対応するスポーン関数と更新関数をまとめた構造体です。
	/// </summary>
	struct WaveOps {
		SpawnFn spawn_ = nullptr;   // スポーン関数
		UpdateFn update_ = nullptr; // 更新関数
	};

	// WaveOps配列のインデックスはWavePhaseと対応
	// [0]=W1, [1]=W2, [2]=W3, [3]=Done
	static const WaveOps kWaveOps_[4];

	/// <summary>
	/// Wave1を開始します。
	/// </summary>
	void BeginWave1();

	/// <summary>
	/// Wave2を開始します。
	/// </summary>
	void BeginWave2();

	/// <summary>
	/// Wave3を開始します。
	/// </summary>
	void BeginWave3();

	//=============================================================
	// 敵ウェーブ設定データ
	//=============================================================
	EnemyWaveConfig waveConfig_{}; // 敵Wave設定データ

	//=============================================================
	// 敵リスト・敵弾リスト
	//=============================================================
	std::vector<std::unique_ptr<Enemy>> enemies_;             // 敵本体のリスト
	std::vector<std::unique_ptr<EnemyBullet>> enemyBullets_;  // 敵弾のリスト

	bool wave1SpecialCharging_ = false; // Wave1特殊攻撃コアの溜め中フラグ
	int defeatedEnemyCount_ = 0;        // 累計撃破数
	int maxEnemyCount_ = 0;             // 想定最大敵数
	bool initializedWaves_ = false;     // Wave初期化済みフラグ

	//=============================================================
	// デルタタイム
	//=============================================================
	float dt_ = 0.016f; // 前回更新時の経過時間保持用

protected:
	/// <summary>
	/// カメラが変更されたときの処理を行います。
	/// EnemyManager配下の敵や核にも新しいカメラを適用します。
	/// </summary>
	void OnCameraChanged() override;
};