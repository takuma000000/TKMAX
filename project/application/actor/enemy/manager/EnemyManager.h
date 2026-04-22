#pragma once

#include <memory>
#include <vector>
#include <array>

#include "Enemy.h"
#include "Player.h"
#include "Camera.h"
#include "Object3d.h"
#include "DirectXCommon.h"
#include "BaseScene.h"
#include "BattleActorManagerBase.h"
#include "EnemyBullet.h"
#include "EnemyBarrier.h"
#include "BarrierCoreManager.h"
#include "EnemyEncounterConfig.h"

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
	// 敵の出現段階管理
	//=============================================================
	enum class EnemyPhase {
		SmallEnemyBattle, // 雑魚敵戦
		BossReady         // ボス戦
	};

	//=============================================================
	// 雑魚敵戦の段階管理
	//=============================================================
	enum class MainSquadPhase {
		BarrierBattle, // バリア戦
		ExposedBattle, // コア露出戦
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

	/// <summary>
	/// 雑魚敵フェーズを開始します。
	/// </summary>
	void StartSmallEnemyPhase();
	/// <summary>
	/// 雑魚敵フェーズをリセットします。フェーズ開始前の状態に戻します。
	/// </summary>
	void ResetSmallEnemyPhase();
	/// <summary>
	/// 雑魚敵フェーズを終了します。フェーズ終了後は、次のWave段階へ進行します。
	/// </summary>
	void FinishSmallEnemyPhase();
	/// <summary>
	/// 雑魚敵フェーズが終了しているかどうかを判定します。
	/// </summary>
	/// <returns>終了している場合は true</returns>
	bool IsSmallEnemyPhaseFinished() const;
	/// <summary>
	/// 雑魚敵フェーズの初期化が完了しているかどうかを判定します。
	/// </summary>
	/// <returns>初期化済みなら true</returns>
	bool IsSmallEnemyPhaseDone() const;
	/// <summary>
	/// 雑魚敵フェーズが現在進行中かどうかを判定します。
	/// </summary>
	/// <returns>進行中の場合は true</returns>
	bool IsPhaseInitialized() const;
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
	/// バリアが有効かどうかを取得します。
	/// </summary>
	/// <returns>バリアが存在し、かつ有効なら true</returns>
	bool IsBarrierActive() const { return barrier_ && barrier_->IsActive(); }

	//=============================================================
	// Getter
	//=============================================================
	/// <summary>
	/// 現在の敵フェーズを取得します。
	/// </summary>
	/// <returns>現在の敵フェーズ</returns>
	EnemyPhase GetEnemyPhase() const { return enemyPhase_; }

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
	/// プレイヤーに最も近い敵を取得します。ロックオンや追尾対象の更新に使用します。
	/// </summary>
	/// <returns>最も近い敵のワールド座標</returns>
	Vector3 GetSpecialCorePosition_() const;

	/// <summary>
	/// バリア中心のワールド座標を取得します。
	/// </summary>
	/// <returns>バリア中心のワールド座標</returns>
	Vector3 GetBarrierCenter() const;

	/// <summary>
	/// バリアのサイズを取得します。
	/// AABBの半分サイズを返します。
	/// バリアが存在しない場合はゼロベクトルを返します。
	/// </summary>
	/// <returns>バリアの半サイズ</returns>
	Vector3 GetBarrierSize() const;

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
	/// 雑魚敵フェーズ本隊戦の開始処理を行います。
	/// </summary>
	void BeginMainSquadBattle_();
	/// <summary>
	/// 雑魚敵フェーズ本隊戦の進行処理を行います。敵の行動更新、特殊攻撃管理、バリア管理などを行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateMainSquadBattle_(float dt);

	//======================================================================
	// 雑魚敵フェーズ本隊
	//======================================================================
	/// <summary>
	/// 雑魚敵フェーズ本隊を生成します。円形隊列の敵を生成し、初期配置や状態を設定します。
	/// </summary>
	void SpawnMainSquad_();

	/// <summary>
	/// 雑魚敵フェーズ本隊の行動を更新します。円形隊列の移動、攻撃、特殊状態管理などを行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateMainSquadOrbit_(float dt);

	/// <summary>
	/// 雑魚敵フェーズ本隊の軌道目標をプレイヤーの位置に基づいて更新します。
	/// </summary>
	void ApplyMainSquadOrbitTargets_();

	/// <summary>
	/// 雑魚敵フェーズ本隊の無敵状態を切り替えます。trueで無敵、falseで通常状態になります。
	/// </summary>
	/// <param name="enable">trueで無敵、falseで通常状態</param>
	void SetMainSquadInvincible_(bool enable);

	/// <summary>
	/// 雑魚敵フェーズ本隊の生存している敵の数をカウントします。
	/// </summary>
	/// <returns>生存中の本隊数</returns>
	int CountAliveMainSquad_() const;

	/// <summary>
	/// 雑魚敵フェーズ本隊の位置を固定するかどうかを切り替えます。trueで位置固定、falseで通常移動になります。
	/// </summary>
	/// <param name="enable">trueで位置固定、falseで通常移動</param>
	void SetMainSquadFreeze_(bool enable);

	//======================================================================
	// 雑魚敵フェーズ特殊攻撃
	//======================================================================

	/// <summary>
	/// 雑魚敵フェーズの特殊攻撃サイクルを開始します。攻撃の準備、状態リセット、演出開始などを行います。
	/// </summary>
	/// <param name="attackType">特殊攻撃の種類</param>
	void UpdateScatterAttack_(float dt);

	/// <summary>
	/// 雑魚敵フェーズの特殊攻撃サイクルを更新します。攻撃の進行管理、状態更新、攻撃発動タイミングの判定などを行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateSpecialAttackCycle_(float dt);

	/// <summary>
	/// 雑魚敵フェーズの特殊攻撃コアの溜め開始処理を行います。コアの生成、初期配置、チャージ状態の設定などを行います。
	/// </summary>
	void BeginSpecialCoreCharge_();

	/// <summary>
	/// 雑魚敵フェーズの特殊攻撃コアの溜め状態を更新します。チャージの進行管理、演出更新、チャージ完了の判定などを行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void UpdateSpecialCoreCharge_(float dt);

	/// <summary>
	/// 雑魚敵フェーズの特殊攻撃コアの発射処理を行います。コアの発射、攻撃開始、演出終了などを行います。
	/// </summary>
	void FireSpecialCore_();

	/// <summary>
	/// 雑魚敵フェーズの特殊攻撃コアのチャージ中に発生させるパーティクルを更新・描画します。
	/// </summary>
	void EmitSpecialCoreChargeParticles_();

	//======================================================================
	// 雑魚敵フェーズバリア
	//======================================================================
	/// <summary>
	/// 雑魚敵フェーズ用バリアを初期化します。
	/// </summary>
	void InitializeBarrier_();

	/// <summary>
	/// 雑魚敵フェーズ用バリアの位置や状態を更新します。
	/// </summary>
	void UpdateBarrier_();

	/// <summary>
	/// 雑魚敵フェーズ用バリアの有効状態を切り替えます。
	/// </summary>
	/// <param name="active">trueで有効、falseで無効</param>
	void SetBarrierActive_(bool active);

	/// <summary>
	/// 雑魚敵フェーズバリア情報をプレイヤーへ同期します。
	/// プレイヤー弾判定や演出連携に使用します。
	/// </summary>
	void SyncBarrierInfoToPlayer_();

	/// <summary>
	/// 雑魚敵フェーズのバリア破壊処理を行います。
	/// </summary>
	void BreakBarrier_();

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
	EnemyEncounterConfig encounterConfig_;

	//======================================================================
	// 敵・敵弾管理
	//======================================================================
	std::vector<std::unique_ptr<Enemy>> enemies_;            // 敵本体リスト
	std::vector<std::unique_ptr<EnemyBullet>> enemyBullets_; // 敵弾リスト

	//======================================================================
	// フェーズ全体の進行状態
	//======================================================================
	EnemyPhase enemyPhase_;               // 現在のWave段階
	bool initializedWaves_ = false;       // Wave初期化済みフラグ
	int defeatedEnemyCount_ = 0;          // 累計撃破数
	int maxEnemyCount_ = 0;               // 想定最大敵数

	//======================================================================
	// 本隊戦の進行状態
	//======================================================================
	MainSquadPhase mainSquadPhase_;                     // Wave1本隊の段階
	float mainSquadPhaseTimer_ = 0.0f;                      // 現在フェーズ開始からの経過時間
	int mainSquadDefeatedCount_ = 0;                    // Wave1本隊の撃破数
	bool mainSquadStopped_ = false;                     // 本隊停止中かどうか

	//======================================================================
	// 本隊の配置・移動設定
	//======================================================================
	static constexpr int kMainSquadEnemyCount_ = 10;        // Wave1本隊の敵数
	float mainSquadMoveSpeed_ = 0.22f;             // 隊列移動速度
	float mainSquadOrbitRadius_ = 18.0f;                   // 円隊列の半径
	float mainSquadOrbitAngularSpeed_ = 0.75f;             // 円隊列の回転速度（rad/sec）
	float mainSquadOrbitAngle_ = 0.0f;                     // 現在の円回転角
	Vector3 mainSquadCenter_ = { 0.0f, -3.0f, 80.0f }; // 円隊列の中心座標

	//======================================================================
	// 特殊攻撃コア・通常射撃
	//======================================================================
	bool specialCoreCharging_ = false;          // 特殊攻撃コア溜め中フラグ
	Vector3 specialCoreOffset_ = { 0.0f, 0.0f, 0.0f }; // 隊列中心から見た特殊攻撃コア位置オフセット
	float specialCoreChargeDuration_ = 2.2f;    // 特殊攻撃コアの溜め時間
	float specialCoreStartScale_ = 0.55f;   // 特殊攻撃コアの初期スケール
	float specialCoreEndScale_ = 5.2f;      // 特殊攻撃コアの最大スケール
	float specialCoreShotSpeed_ = 1.6f;     // 特殊攻撃コア弾の発射速度
	float specialCoreRadius_ = 2.8f;        // 特殊攻撃コア弾の当たり判定半径
	int   specialCoreDamage_ = 2;           // 特殊攻撃コア弾のダメージ量
	EnemyBullet* specialCoreBullet_ = nullptr; // 特殊攻撃コア弾への参照

	float scatterShotInterval_ = 1.05f; // 通常攻撃の発射間隔
	float scatterShotTimer_ = 0.0f;     // 通常攻撃用タイマー
	float scatterShotBulletSpeed_ = 0.42f;  // 通常弾の移動速度

	//======================================================================
	// バリア状態・設定
	//======================================================================
	bool barrierBroken_ = false;                 // バリア破壊済みかどうか
	std::unique_ptr<EnemyBarrier> barrier_ = nullptr; // バリア本体
	Vector3 barrierOffset_ = { 0.0f, 0.0f, 0.0f };    // 隊列中心から見たバリア位置オフセット
	Vector3 barrierSize_ = { 23.0f, 23.0f, 11.0f };   // バリアサイズ
	bool barrierFollowCore_ = true;                   // バリアを中心対象へ追従させるか

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