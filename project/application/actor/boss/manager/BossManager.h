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
#include <array>

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
	/// ボス登場演出用にボス本体だけ生成します。
	/// まだ本戦開始せず、UIも出しません。
	/// </summary>
	void SpawnForEntrance();
	/// <summary>
	/// 生成済みのボスで本戦を開始します。
	/// UI表示・射撃許可などをここで有効化します。
	/// </summary>
	void BeginBattle();
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
	/// <summary>
	/// ボスの出現位置を取得します。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetSpawnPos() const { return bossConfig_.bossBattle_.spawnPos_; }
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
	bool isEntranceDrawing_ = false; // ボス登場演出中の描画フラグ
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

	//==============================
	// 撃破シーケンス用の判定ポータル
	// 撃破シーケンス中、ボスが特定の位置に来たときに攻撃を止めたりエフェクトを出したりするための判定用ポータル。
	// 6つ用意して、撃破シーケンスの進行に合わせて順番に有効化していきます。
	//==============================
	// 判定ポータル構造体
	struct JudgementPortal {
		Vector3 pos_;
		bool active_ = false;
	};
	// 判定ポータルリスト
	std::array<JudgementPortal, 6> judgementPortals_{};
	/// <summary>
	/// 撃破シーケンス用の判定ポータルを更新します。ボスがポータルの位置に来たら、次のシーケンスに進めるためのフラグを立てたり、攻撃を止めたりします。
	/// </summary>
	void StartJudgementPortals_();
	/// <summary>
	/// 撃破シーケンス用の判定ポータルを停止します。シーケンスが進んで次の段階に入ったら、前の段階のポータルはもう必要ないので無効化します。
	/// </summary>
	void StopJudgementPortals_();

	//==============================
	// レーザー光線
	// 撃破シーケンスの演出で、ボスからプレイヤーに向かってレーザーを発射する演出があります。
	// レーザーは、ボスの位置からプレイヤーの位置に向かって伸びる線で表現されます。
	//==============================
	struct JudgementLaser {
		Vector3 start_;
		Vector3 end_;
		float timer_ = 0.0f;
		bool active_ = false;
	};

	std::array<JudgementLaser, 6> judgementLasers_{}; // 判定レーザーリスト

	bool judgementActivePrev_ = false; // 前フレームの判定レーザー発射中フラグ
	float judgementLaserIntervalTimer_ = 0.0f; // 判定レーザーの発射間隔タイマー
	int judgementLaserFireIndex_ = 0; // 次に撃つポータル番号

	int judgementLaserTotalFireCount_ = 0; // 発射した判定レーザーの総数（撃破シーケンス全体で何発撃ったか）
	static constexpr int kJudgementLaserSingleFireCount_ = 12; // 撃破シーケンス全体で撃つ判定レーザーの総数
	bool judgementFinalBurstFired_ = false; // 撃破シーケンスの最後の一斉発射を撃ったかどうか
	float judgementStartDelayTimer_ = 0.0f; // 撃破シーケンス開始から判定レーザー発射までの遅延タイマー

	float judgementFinalChargeTimer_ = 0.0f; // 撃破シーケンスの最後の一斉発射のためのチャージタイマー
	static constexpr float kJudgementFinalChargeTime_ = 0.8f; // 撃破シーケンスの最後の一斉発射のためのチャージ時間（秒）

	bool judgementPortalVisible_ = false; // 撃破シーケンスの判定ポータルを描画するかどうかのフラグ（デバッグ用）

	float judgementPortalChargeTimer_ = 0.0f; // 撃破シーケンスの判定ポータルのチャージタイマー（ポータルが点滅する演出用）

	/// <summary>
	/// 撃破シーケンス用の判定レーザーを更新します。レーザーの発射タイミングや持続時間を管理し、必要に応じてレーザーを発射したり消したりします。
	/// </summary>
	void UpdateJudgementLasers_(float dt);
	/// <summary>
	/// 撃破シーケンス用の判定レーザーを発射します。ボスからプレイヤーに向かってレーザーを伸ばす演出を開始します。
	/// </summary>
	void FireJudgementLasers_();
	/// <summary>
	/// 撃破シーケンス用の判定レーザーを消します。レーザーの演出が終わったら、レーザーを消して次の段階に進める準備をします。
	/// </summary>
	void ClearJudgementLasers_();
	/// <summary>
	/// 撃破シーケンス用の判定レーザーを描画します。発射中のレーザーがあれば、ボスからプレイヤーに向かって線を描画します。
	/// </summary>
	void DrawJudgementLasers_();
	/// <summary>
	/// 撃破シーケンス用の判定レーザーの最後の一斉発射を行います。ボスからプレイヤーに向かって、6つのレーザーを同時に発射する演出を行います。
	/// </summary>
	void FireJudgementFinalBurst_();
	/// <summary>
	/// 撃破シーケンス用の判定レーザーの最後の一斉発射のためのエフェクトを発生させます。レーザーを発射する前に、ボスからプレイヤーに向かってエフェクトを表示します。
	/// </summary>
	void EmitJudgementPortalFx_();

	// ジャッジメントレーザーの当たり判定用ID（プレイヤーの攻撃と重複しないようにするため）
	int judgementAttackId_ = 10000;
	/// <summary>
	/// 撃破シーケンス用の判定レーザーの当たり判定を行います。レーザーがプレイヤーに当たっているかどうかを判定し、当たっていればプレイヤーにダメージを与えます。
	/// </summary>
	void CheckJudgementLaserHit_();
	/// <summary>
	/// レーザーとプレイヤーの当たり判定を行います。レーザーがプレイヤーの位置とサイズを考慮して当たっているかどうかを判定します。
	/// </summary>
	/// <param name="laserStart">レーザーの開始位置（ワールド座標）</param>
	/// <param name="laserEnd">レーザーの終了位置（ワールド座標）</param>
	/// <param name="playerPos">プレイヤーの位置（ワールド座標）</param>	
	/// <param name="playerSize">プレイヤーのサイズ（幅・高さ・奥行き）</param>
	/// <param name="laserRadius">レーザーの当たり判定半径</param>
	/// <returns>レーザーがプレイヤーに当たっているかどうか</returns>
	bool HitTestLaserToPlayer_(
		const Vector3& laserStart,
		const Vector3& laserEnd,
		const Vector3& playerPos,
		const Vector3& playerSize,
		float laserRadius
	);

protected:
	/// <summary>
	/// カメラが変更されたときの処理。BossManagerはカメラを参照して描画や当たり判定を行うため、カメラが変更されたときに必要な処理をここに実装します。
	/// </summary>
	void OnCameraChanged() override;
};