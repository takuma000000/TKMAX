#pragma once

#undef max
#undef min

#define NOMINMAX
#include <algorithm>
#include <string>
#include <memory>
#include "Object3d.h"
#include "MyMath.h"
#include "Enemy.h"
#include <ParticlerEmitter.h>

class Player;
class MidBossCore;

//=============================================================
// PlayerBulletクラス
// プレイヤーの弾を管理するクラス。
//=============================================================
class PlayerBullet {
public:

	/// <summary>
	/// プレイヤーの弾を初期化します。
	/// </summary>
	/// <param name="common"></param>
	/// <param name="dxCommon"></param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// プレイヤーの弾を更新します。
	/// </summary>
	void Update();
	/// <summary>
	/// プレイヤーの弾を描画します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void Draw(TKM::DirectXCommon* dxCommon);

	/// <summary>
	/// デバッグ用ImGui表示。
	/// </summary>
	/// <returns></returns>
	bool IsHit() const { return isHit_; }
	/// <summary>
	/// 弾が死亡したかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 発射の「出方」曲線を開始します。
	/// </summary>
	/// <param name="p0"></param>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="p3"></param>
	/// <param name="duration"></param>
	/// <param name="velocityAfter"></param>
	void StartSpawnBezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float duration, const Vector3& velocityAfter);

	// Getter===================================
	/// <summary>
	/// 弾が追従している敵を取得します。
	/// </summary>
	/// <returns></returns>
	Enemy* GetEnemy() const { return enemy_; }
	// =========================================
	// Setter===================================
	/// <summary>
	/// プレイヤーの位置を設定します。
	/// </summary>
	/// <param name="pos"></param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// 弾の速度を設定します。
	/// </summary>
	/// <param name="vel"></param>
	void SetVelocity(const Vector3& vel);
	/// <summary>
	/// カメラを設定します。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(TKM::Camera* camera) {
		if (object_) {
			object_->SetCamera(camera);
		}
	}
	/// <summary>
	/// 弾のスケールを設定します。
	/// </summary>
	/// <param name="group"></param>
	void SetTrailGroup(const std::string& group) {
		trailGroup_ = group;
		// 位置は現在地で再初期化（生成直後や途中でもOK）
		Vector3 pos = object_ ? object_->GetTranslate() : Vector3{};
		trailEmitter_.Initialize(trailGroup_, pos);
	}
	/// <summary>
	/// 弾が当たったときの処理。
	/// </summary>
	/// <param name="enemy"></param>
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
	/// <summary>
	/// プレイヤーを設定します。
	/// </summary>
	/// <param name="player"></param>
	void SetPlayer(Player* player) { player_ = player; }
	/// <summary>
	/// 一撃必殺フラグ設定。
	/// </summary>
	/// <param name="flag"></param>
	void SetSpecialAttack(bool flag) { isSpecialAttack_ = flag; }
	/// <summary>
	/// ホーミング設定。
	/// </summary>
	/// <param name="enable"></param>
	/// <param name="speed"></param>
	void SetHoming(bool enable, float speed) { isHoming_ = enable; homingSpeed_ = speed; }
	/// <summary>
	/// ホーミング遅延時間設定。
	/// </summary>
	/// <param name="sec"></param>
	void  SetHomingDelay(float sec) { homingDelay_ = std::max(0.0f, sec); }
	/// <summary>
	/// 中ボスコアを設定します。
	/// </summary>
	/// <param name="core"></param>
	void SetCore(MidBossCore* core) { core_ = core; }
	// =========================================
private:
	//======================================================================
	// 参照ポインタ / 本体
	//======================================================================
	Player* player_ = nullptr;

	std::unique_ptr<TKM::Object3d> object_;
	Vector3 velocity_{}; // 弾の現在速度
	Vector3 prevPos_{}; // 前フレームの位置（トンネリング対策用）

	Enemy* enemy_ = nullptr;
	MidBossCore* core_ = nullptr;
	//======================================================================
	// 生存状態・ヒットフラグ
	//======================================================================
	bool isDead_ = false;
	bool isHit_ = false;

	bool isSpecialAttack_ = false; // 一撃必殺フラグ
	//======================================================================
	// ホーミング / ベジェ出現フェーズ
	//======================================================================
	bool  isHoming_ = false;
	float homingSpeed_ = 0.6f;      // 追従弾の速度（調整可）
	float homingDelay_ = 0.0f;      // 追尾開始までの遅延秒

	bool  isSpawningCurve_ = false;  // 発射の「出方」曲線フェーズ中か
	float spawnT_ = 0.0f;   // 0..1 の補間量
	float spawnDuration_ = 0.25f;  // 出方にかける秒数（調整可）

	Vector3 bezP0_, bezP1_, bezP2_, bezP3_;      // ベジェ制御点
	Vector3 postSpawnVelocity_ = { 0,0,0 };      // 曲線フェーズ終了後に引き継ぐ速度

	/// <summary>
	/// 発射の「出方」曲線フェーズ更新。
	/// </summary>
	void UpdateSpawnBezier();
	//======================================================================
	// パーティクル（軌跡）
	//======================================================================
	ParticleEmitter trailEmitter_;              // 弾の軌跡パーティクル
	std::string     trailGroup_ = "bulletTrail"; // デフォルトのパーティクルグループ名
	//======================================================================
	// 共通パラメータ（マジックナンバー解消）
	//======================================================================
	// 共通パラメータ（マジックナンバー解消）
	static constexpr float kDefaultScale = 0.2f;  // 弾の見た目サイズ
	static constexpr float kDespawnZ = 150.0f; // 消えるZ位置
};