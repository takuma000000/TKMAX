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

//=============================================================
// BossManagerクラス
// ボス本体＋ボス弾の管理を行うクラス。
//=============================================================
class BossManager {
public:
	BossManager() = default;
	~BossManager() = default;

	/// <summary>
	/// 初期化。
	/// </summary>
	/// <param name="dxCommon"></param>
	/// <param name="camera"></param>
	/// <param name="parent"></param>
	/// <param name="player"></param>
	void Initialize(DirectXCommon* dxCommon, Camera* camera, BaseScene* parent, Player* player);
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
	void Draw(DirectXCommon* dxCommon);

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
	void SetTimeScaleController(TimeScaleController* t) { timeScale_ = t; }
	// =========================================

	void SetCamera(Camera* camera) {
		camera_ = camera;
		if (boss_) {
			boss_->SetCamera(camera_);
		}
		for (auto& b : bossBullets_) {
			b->SetCamera(camera_);
		}
	}

private:
	DirectXCommon* dxCommon_ = nullptr;
	Camera* camera_ = nullptr;
	BaseScene* parentScene_ = nullptr;
	Player* player_ = nullptr;

	bool bossBattle_ = false;                     // ボス戦フラグ
	bool bossP2BgmPlayed_ = false;                // P2BGMを1回だけ再生したか

	std::unique_ptr<BossEnemy> boss_;             // ボス本体
	std::vector<std::unique_ptr<BossBullet>> bossBullets_; // ボス弾リスト

	bool bossZoomStarted_ = false; // ボス撃破後のカメラズーム演出が始まったか

	std::unique_ptr<BossController> bossController_; // ボスコントローラー

	// オーラボリュームレンダラー
	std::unique_ptr<AuraVolumeRenderer> auraVolume_;

	// タイムスケールコントローラー参照
	TimeScaleController* timeScale_ = nullptr; // タイムスケールコントローラー参照
	bool slowTriggered_ = false; // スローが発動したか

	/// <summary>
	/// ボス弾を更新します。
	/// </summary>
	void UpdateBossBullets();
};
