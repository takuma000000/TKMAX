#pragma once
#include <memory>
#include <vector>
#include "DirectXCommon.h"
#include "camera/Camera.h"
#include "Vector3.h"
#include "Object3dCommon.h"
#include "AudioManager.h"
#include "BossEnemy.h"
#include "BossBullet.h"
#include "Player.h"
#include "BaseScene.h"
#include "BossController.h"
#include "AuraVolumeRenderer.h"
#include "TimeScaleController.h"
#include "WaterRippleEffect.h"
#include "LaserBeam3D.h"
#include "BossHpBarUI.h"

//=============================================================
// BossManagerクラス
// ボス本体＋ボス弾の管理を行うクラス。
//=============================================================
class BossManager {
public:
	BossManager() = default;
	~BossManager() = default;

	// Boss戦設定構造体
	struct BossBattleConfig {
		Vector3 spawnPos;
		Vector3 arenaMin;
		Vector3 arenaMax;

		TKM::WaterRippleEffect::RippleDesc killRipple;

		float killSlowScale = 1.0f;
		float killSlowDuration = 0.0f;
	};
	// 撃破シーケンス状態構造体
	struct KillSequenceState {
		bool zoomStarted = false;
		bool slowTriggered = false;
		bool rippleTriggered = false;

		/// <summary>
		/// リセット。
		/// </summary>
		void Reset() {
			zoomStarted = false;
			slowTriggered = false;
			rippleTriggered = false;
		}
	};

	// =========================
	// Laser（怒り中攻撃）情報
	// =========================
	struct LaserInfo {
		bool active = false;       // 予告 or 発射中
		bool telegraph = false;    // 予告中
		Vector3 startWS{ 0.0f,0.0f,0.0f };
		Vector3 endWS{ 0.0f,0.0f,0.0f };
		float radius = 0.0f;       // 当たり判定半径
	};

	/// <summary>
	/// 現在のレーザー情報を取得（描画/当たり判定用）
	/// </summary>
	LaserInfo GetLaserInfo() const;

	/// <summary>
	/// 点（球）とレーザー線分の当たり判定
	/// </summary>
	static bool TestLaserHit(const LaserInfo& laser, const Vector3& sphereCenterWS, float sphereRadius);

	/// <summary>
	/// 初期化。
	/// </summary>
	/// <param name="dxCommon"></param>
	/// <param name="camera"></param>
	/// <param name="parent"></param>
	/// <param name="player"></param>
	void Initialize(TKM::DirectXCommon* dxCommon, TKM::Camera* camera, TKM::BaseScene* parent, Player* player);
	/// <summary>
	/// ボス戦開始。
	/// </summary>
	void StartBattle();
	/// <summary>
	/// 更新。
	/// </summary>
	/// <param name="dt"></param>
	void Update(float dt);
	/// <summary>
	/// 描画。
	/// </summary>
	/// <param name="dxCommon"></param>
	void Draw(TKM::DirectXCommon* dxCommon);

	/// <summary>
	/// UI描画。
	/// </summary>
	void DrawUI();

	/// <summary>
	/// ボス弾をスポーンさせる。
	/// </summary>
	/// <param name="pos"></param>
	/// <param name="dir"></param>
	/// <param name="speed"></param>
	/// <param name="damage"></param>
	/// <param name="lifeFrame"></param>
	void SpawnEnemyBullet(const Vector3& pos, const Vector3& dir, float speed, int damage, int lifeFrame);
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
	BossEnemy* GetBoss() const { return boss_.get(); }
	// =========================================
	// Setter===================================
	/// <summary>
	/// タイムスケールコントローラー設定。
	/// </summary>
	/// <param name="t"></param>
	void SetTimeScaleController(TKM::TimeScaleController* t) { timeScale_ = t; }
	/// <summary>
	/// ウォーターリップルエフェクト設定。
	/// </summary>
	/// <param name="r"></param>
	void SetWaterRippleEffect(TKM::WaterRippleEffect* r) { waterRipple_ = r; }
	/// <summary>
	/// カメラ設定。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(TKM::Camera* camera) {
		camera_ = camera;
		if (boss_) { boss_->SetCamera(camera_); }
		for (auto& b : bossBullets_) { b->SetCamera(camera_); }
	}
	// =========================================

private:
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	TKM::BaseScene* parentScene_ = nullptr;
	Player* player_ = nullptr;

	bool bossBattle_ = false;                     // ボス戦フラグ
	bool bossP2BgmPlayed_ = false;                // P2BGMを1回だけ再生したか

	std::unique_ptr<BossEnemy> boss_;             // ボス本体
	std::vector<std::unique_ptr<BossBullet>> bossBullets_; // ボス弾リスト
	std::unique_ptr<BossController> bossController_; // ボスコントローラー
	std::unique_ptr<TKM::AuraVolumeRenderer> auraVolume_; // オーラボリュームレンダラー
	std::unique_ptr<TKM::LaserBeam3D> laserBeam3D_; // レーザー描画

	// タイムスケールコントローラー参照
	TKM::TimeScaleController* timeScale_ = nullptr;
	// ウォーターリップルエフェクト参照
	TKM::WaterRippleEffect* waterRipple_ = nullptr;

	/// <summary>
	/// ボス弾を更新します。
	/// </summary>
	void UpdateBossBullets();

	KillSequenceState killSeq_; // 撃破シーケンス状態

	std::unique_ptr<TKM::BossHpBarUI> hpUI_; // ボスHPバーUI
};