#pragma once
#define NOMINMAX

#include <list>
#include <memory>
#include <vector>
#include <algorithm>

#include "PlayerBullet.h"
#include "HomingBullet.h"
#include "Object3d.h"
#include "Camera.h"
#include "Input.h"
#include "PlayerShotConfig.h"
#include "reticle/Reticle.h"

class Player;
class Enemy;
class BarrierCore;
class BarrierCoreManager;

class PlayerShotManager {
public:
	/// <summary>
	/// プレイヤーのショット管理クラスを初期化します。
	/// </summary>
	/// <param name="owner">初期化するプレイヤーのポインタ</param>
	/// <param name="common">Object3dCommonのポインタ</param>
	/// <param name="dxCommon">DirectXCommonのポインタ</param>
	void Initialize(Player* owner, TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// プレイヤーのショット管理クラスを終了処理します。
	/// </summary>
	/// <param name="dt">デルタタイム（秒）</param>
	/// <param name="canShoot">ショット可能かどうか</param>
	void Update(float dt, bool canShoot);
	/// <summary>
	/// プレイヤーのショット管理クラスを描画します。
	/// </summary>
	/// <param name="dxCommon">DirectXCommonのポインタ</param>
	void DrawTrails(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// プレイヤーのショット管理クラスを描画します。
	/// </summary>
	/// <param name="dxCommon">DirectXCommonのポインタ</param>
	void DrawBullets(TKM::DirectXCommon* dxCommon);

	/// <summary>
	/// ロック状態をクリアします。
	/// </summary>
	void ClearLockState();
	/// <summary>
	/// 死亡した弾を削除します。
	/// </summary>
	void RemoveDeadTargets();
	/// <summary>
	/// 敵が破壊されたときの処理を行います。
	/// </summary>
	/// <param name="e">破壊された敵のポインタ</param>
	void OnEnemyDestroyed(Enemy* e);
	/// <summary>
	/// ミッドボスコアが破壊されたときの処理を行います。
	/// </summary>
	/// <param name="core">破壊されたミッドボスコアのポインタ</param>
	void OnBarrierCoreDestroyed(BarrierCore* core);
	/// <summary>
	/// バリアコアが破壊されたときの処理を行います。
	/// </summary>
	void EnableSpecialAttack() { canUseSpecial_ = true; }
	/// <summary>
	/// ショット設定を反映します。
	/// </summary>
	/// <param name="config">プレイヤーショット設定</param>
	void SetConfig(const PlayerShotConfig* config);

	/// <summary>
	/// RBのリフィル中かどうかを取得します。
	/// </summary>
	/// <returns>RBのリフィル中であればtrue、それ以外はfalse</returns>
	bool IsRbRefilling() const { return rbRefilling_; }

	// Getter========================================
	/// <summary>
	/// 現在存在するプレイヤーの弾のリストを取得します。
	/// </summary>
	/// <returns>プレイヤーの弾のリスト</returns>
	const std::list<std::unique_ptr<PlayerBullet>>& GetBullets() const {
		return bullets_;
	}
	/// <summary>
	/// 現在存在するホーミング弾のリストを取得します。
	/// </summary>
	/// <returns>ホーミング弾のリスト</returns>
	const std::list<std::unique_ptr<HomingBullet>>& GetHomingBullets() const {
		return homingBullets_;
	}
	/// <summary>
	/// RBの現在の弾数を取得します。
	/// </summary>
	/// <returns>RBの現在の弾数</returns>
	int GetRbAmmo() const { return rbAmmo_; }
	/// <summary>
	/// LBの現在の弾数を取得します。
	/// </summary>
	/// <returns>LBの現在の弾数</returns>
	int GetLbAmmo() const { return lbAmmo_; }
	/// <summary>
	/// RBの最大弾数を取得します。
	/// </summary>
	/// <returns>RBの最大弾数</returns>
	int GetRbAmmoMax() const {
		return config_ ? config_->GetRB().ammoMax_ : 0;
	}
	/// <summary>
	/// LBの最大弾数を取得します。
	/// </summary>
	/// <returns>LBの最大弾数</returns>
	int GetLbAmmoMax() const {
		return config_ ? config_->GetLB().ammoMax_ : 0;
	}
	// ==============================================
	// Setter========================================
	/// <summary>
	/// 所有者のプレイヤーを設定します。
	/// </summary>
	/// <param name="object">所有者のプレイヤーのオブジェクト</param>
	void SetOwnerObject(TKM::Object3d* object) { ownerObject_ = object; }
	/// <summary>
	/// レティクルを設定します。
	/// </summary>
	/// <param name="reticle">設定するレティクルのポインタ</param>
	void SetReticle(Reticle* reticle) { reticle_ = reticle; }
	/// <summary>
	/// カメラを設定します。
	/// </summary>
	/// <param name="camera">設定するカメラのポインタ</param>
	void SetCamera(TKM::Camera* camera) { camera_ = camera; }
	/// <summary>
	/// 敵を設定します。
	/// </summary>
	/// <param name="enemy">設定する敵のポインタ</param>
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	/// <summary>
	/// 全ての敵のリストを設定します。
	/// </summary>
	/// <param name="enemies">設定する全ての敵のリストのポインタ</param>
	void SetAllEnemies(std::vector<std::unique_ptr<Enemy>>* enemies) { allEnemies_ = enemies; }
	/// <summary>
	/// ミッドボスコアを設定します。
	/// </summary>
	/// <param name="core">設定するミッドボスコアのポインタ</param>
	void SetBarrierCore(BarrierCore* core) { core_ = core; }
	/// <summary>
	/// バリアコアマネージャーを設定します。
	/// </summary>
	/// <param name="manager">設定するバリアコアマネージャーのポインタ</param>
	void SetBarrierCoreManager(BarrierCoreManager* manager) { barrierCoreManager_ = manager; }
	/// <summary>
	/// ショットの有効/無効を設定します。
	/// </summary>
	/// <param name="enabled">ショットを有効にする場合はtrue、無効にする場合はfalse</param>
	void SetShootingEnabled(bool enabled);
	// ==============================================

private:
	/// <summary>
	/// ロック状態を更新します。ロック状態は、プレイヤーが敵をロックオンしているかどうかを示す状態で、ショットの挙動に影響を与えます。
	/// </summary>
	void UpdateLockState_();
	/// <summary>
	/// ショットの入力を処理します。プレイヤーの入力に応じて、ショットの発射や弾数の管理などを行います。
	/// </summary>
	/// <param name="dt">デルタタイム</param>
	void HandleShooting_(float dt);

	/// <summary>
	/// RBショットの処理を行います。
	/// </summary>
	void RBShoot_();
	/// <summary>
	/// LBショットの処理を行います。
	/// </summary>
	void LBShoot_();

	//======================================================================
	// 参照ポインタ / 共通オブジェクト
	//======================================================================
	Player* owner_ = nullptr; // このマネージャを所有しているPlayer本体

	TKM::Object3dCommon* common_ = nullptr;     // Object3d共通描画データ
	TKM::DirectXCommon* dxCommon_ = nullptr;    // DirectX共通処理
	TKM::Camera* camera_ = nullptr;             // 描画・弾追従用カメラ
	TKM::Object3d* ownerObject_ = nullptr;      // Playerの3Dオブジェクト（位置取得用）
	Reticle* reticle_ = nullptr;                // 照準（弾の発射方向取得用）

	Enemy* enemy_ = nullptr;                    // 現在ロック中の敵
	BarrierCore* core_ = nullptr;               // 中ボスコア（優先ターゲット）
	BarrierCoreManager* barrierCoreManager_ = nullptr; // バリアコア管理（当たり判定用）
	std::vector<std::unique_ptr<Enemy>>* allEnemies_ = nullptr; // 全敵リスト（RB用レイ判定）
	Enemy* lastLockedEnemy_ = nullptr;          // 前フレームでロックしていた敵（ロック解除用）

	//======================================================================
	// 弾オブジェクト管理
	//======================================================================
	std::list<std::unique_ptr<PlayerBullet>> bullets_;        // 使用中の通常弾（RB）
	std::list<std::unique_ptr<HomingBullet>> homingBullets_;  // 使用中のホーミング弾（LB）

	//======================================================================
	// ObjectPool
	//======================================================================
	std::list<std::unique_ptr<PlayerBullet>> bulletPool_;         // 待機中の通常弾（RB）
	std::list<std::unique_ptr<HomingBullet>> homingBulletPool_;   // 待機中のホーミング弾（LB）

	//======================================================================
	// 射撃制御フラグ
	//======================================================================
	bool shootingEnabled_ = true; // 射撃許可フラグ（falseなら一切撃てない）

	bool rtHeld_ = false; // RT押しっぱなし判定（離した瞬間発射用）
	bool ltHeld_ = false; // LB押しっぱなし判定（連射防止ラッチ）

	bool canUseSpecial_ = false;      // RT必殺技が使えるか
	bool debugUnlimitedSpecial_ = false; // デバッグ：RT無限使用
	bool debugUnlimitedLB_ = false;      // デバッグ：LB無限弾

	//======================================================================
	// 射撃共通パラメータ
	//======================================================================
	static constexpr int kTriggerThreshold = 128;

	//======================================================================
	// RB弾管理（通常連射弾）
	//======================================================================
	int rbAmmo_ = 0;                     // RB弾の現在弾数

	static constexpr float kRbEmptyWaitSec_ = 3.0f; // 弾切れ後、回復開始までの待機時間
	static constexpr float kRbRefillSec_ = 0.60f;   // 満タンまでの回復時間

	float rbEmptyTimer_ = 0.0f;   // 弾切れ状態の経過時間
	float rbRefillValue_ = 0.0f;  // 回復中の内部値（小数で管理）
	bool rbRefilling_ = false;    // 回復中フラグ

	bool rbRefillStartedFromEmpty_ = false; // 0発から始まった回復かどうか

	float rbNoFireTimer_ = 0.0f;  // 最後に撃ってからの経過時間（アイドル回復判定用）

	static constexpr float kRbShotCooldownSec_ = 0.25f; // 1発ごとの発射間隔
	float rbShotCooldownTimer_ = 0.0f;                 // クールダウン残り時間

	//======================================================================
	// LB弾管理（ホーミング弾）
	//======================================================================
	int lbAmmo_ = 0;                     // LB弾の現在弾数

	static constexpr float kLbRefillWaitSec_ = 3.0f; // 最後に撃ってから満タン回復までの待機時間
	float lbNoFireTimer_ = 0.0f;                    // 最後に撃ってからの経過時間


	const PlayerShotConfig* config_ = nullptr; // プレイヤー弾設定(JSON)
};