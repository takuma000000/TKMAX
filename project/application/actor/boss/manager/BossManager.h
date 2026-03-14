#pragma once
#include <memory>
#include <vector>
#include "DirectXCommon.h"
#include "Camera.h"
#include "MyMath.h"
#include "Object3dCommon.h"
#include "AudioManager.h"
#include "BossEnemy.h"
#include "BossBullet.h"
#include "Player.h"
#include "BaseScene.h"
#include "BossController.h"
#include "TimeScaleController.h"
#include "WaterRippleEffect.h"
#include "BossHpBarUI.h"
#include "BattleActorManagerBase.h"
#include "BossConfig.h"

//=============================================================
// BossManagerクラス
// ボス本体＋ボス弾の管理を行うクラス。
//=============================================================
class BossManager : public BattleActorManagerBase {
public:
	BossManager() = default;
	~BossManager() = default;

	// Boss戦設定構造体
	struct BossBattleConfig {
		Vector3 spawnPos_; // ボス出現位置（ワールド座標）
		Vector3 arenaMin_; // アリーナ最小座標（ワールド座標）
		Vector3 arenaMax_; // アリーナ最大座標（ワールド座標）

		TKM::WaterRippleEffect::RippleDesc killRipple_; // 撃破時波紋エフェクト設定

		float killSlowScale_ = 1.0f; // 撃破時スローモーション倍率
		float killSlowDuration_ = 0.0f; // 撃破時スローモーション継続時間（秒）
	};
	// 撃破シーケンス状態構造体
	struct KillSequenceState {
		bool zoomStarted_ = false; // ズーム開始フラグ
		bool slowTriggered_ = false; // スローモーション発動済みフラグ
		bool rippleTriggered_ = false; // スローモーション/波紋エフェクト発動済みフラグ
		bool attacksStopped_ = false; // 撃破中に攻撃を止めたか

		/// <summary>
		/// リセット。
		/// </summary>
		void Reset();
	};

	/// <summary>
	/// 初期化処理を行います。
	/// </summary>
	/// <param name="dxCommon">DirectX共通管理クラス</param>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	/// <param name="parent">所属する親シーン</param>
	/// <param name="player">レーザー発射元となるプレイヤー</param>
	void Initialize(
		TKM::DirectXCommon* dxCommon,
		TKM::Camera* camera,
		TKM::BaseScene* parent,
		Player* player
	);
	/// <summary>
	/// ボス戦開始。
	/// </summary>
	void StartBattle();
	/// <summary>
	/// 毎フレームの更新処理を行います。
	/// </summary>
	/// <param name="dt">前フレームからの経過時間（秒）</param>
	void Update(float dt) override;
	/// <summary>
	/// 描画処理を行います。
	/// </summary>
	/// <param name="dxCommon">DirectX共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon) override;
	/// <summary>
	/// UI描画。
	/// </summary>
	void DrawUI();

	/// <summary>
	/// ボスが使用する弾をスポーンさせます。
	/// </summary>
	/// <param name="pos">弾の生成位置（ワールド座標）</param>
	/// <param name="dir">弾の進行方向（正規化ベクトル）</param>
	/// <param name="speed">弾の移動速度</param>
	/// <param name="damage">ヒット時に与えるダメージ量</param>
	/// <param name="lifeFrame">弾が消滅するまでの生存フレーム数</param>
	void SpawnEnemyBullet(
		const Vector3& pos,
		const Vector3& dir,
		float speed,
		int damage,
		int lifeFrame
	);
	/// <summary>
	/// ボス戦がアクティブか？
	/// </summary>
	/// <returns></returns>
	bool IsBattleActive() const;
	/// <summary>
	/// ボスが生存しているか？
	/// </summary>
	/// <returns></returns>
	bool IsBossAlive() const;
	/// <summary>
	/// ボスが死亡しているか？
	/// </summary>
	/// <returns></returns>
	bool IsBossDead() const;
	/// <summary>
	/// クリアシーケンス開始時の処理。
	/// </summary>
	void OnClearSequenceStart();

	// Getter===================================
	/// <summary>
	/// ボス本体を取得。
	/// </summary>
	/// <returns></returns>
	BossEnemy* GetBoss() { return boss_.get(); }
	/// <summary>
	/// ボス本体を取得（const版）。
	/// </summary>
	/// <returns></returns>
	const BossEnemy* GetBoss() const { return boss_.get(); }
	// =========================================
	// Setter===================================
	/// <summary>
	/// タイムスケールコントローラーを設定します。
	/// </summary>
	/// <param name="t">使用するタイムスケールコントローラー</param>
	void SetTimeScaleController(TKM::TimeScaleController* t);
	/// <summary>
	/// ウォーターリップルエフェクトを設定します。
	/// </summary>
	/// <param name="r">使用するウォーターリップルエフェクト</param>
	void SetWaterRippleEffect(TKM::WaterRippleEffect* r);
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera) override;
	// =========================================

private:
	//==============================
	// 外部参照（システム系）
	//==============================
	// タイムスケールコントローラー参照
	TKM::TimeScaleController* timeScale_ = nullptr;
	// ウォーターリップルエフェクト参照
	TKM::WaterRippleEffect* waterRipple_ = nullptr;
	//==============================
	// 状態フラグ
	//==============================
	bool bossBattle_ = false; // ボス戦フラグ
	bool bossP2BgmPlayed_ = false; // P2BGMを1回だけ再生したか
	KillSequenceState killSeq_; // 撃破シーケンス状態
	//==============================
	// ボス本体・制御
	//==============================
	std::unique_ptr<BossEnemy> boss_; // ボス本体
	std::unique_ptr<BossController> bossController_; // ボスコントローラー
	std::vector<std::unique_ptr<BossBullet>> bossBullets_; // ボス弾リスト
	//==============================
	// UI
	//==============================
	std::unique_ptr<TKM::BossHpBarUI> hpUI_; // ボスHPバーUI
	//==============================
	// 内部処理
	//==============================
	/// <summary>
	/// ボス弾を更新します。
	/// </summary>
	void UpdateBossBullets();
	//==============================
	// Slash hit anti-multi（encapsulated）
	//==============================
	int slashAttackId_ = 0; // スラッシュ攻撃IDカウンタ
	int currentSlashId_ = -1; // 現在処理中のスラッシュ攻撃ID
	float slashIdHoldT_ = 0.0f; // 現在のスラッシュ攻撃IDの保持時間
	//==============================
	// 設定
	//==============================
	BossConfig bossConfig_;

protected:
	/// <summary>
	/// カメラが変更されたときの処理。BossManagerはカメラを参照して描画や当たり判定を行うため、カメラが変更されたときに必要な処理をここに実装します。
	/// </summary>
	void OnCameraChanged() override;
};