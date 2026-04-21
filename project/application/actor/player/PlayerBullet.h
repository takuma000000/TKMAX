#pragma once

#undef max
#undef min

#define NOMINMAX
#include <algorithm>
#include <string>
#include <memory>
#include "Object3d.h"
#include "MyMath.h"
#include <ParticlerEmitter.h>
#include <vector>

class Player;
class Enemy;
class BarrierCoreManager;
class BarrierCore;

//=============================================================
// PlayerBulletクラス
// プレイヤーの弾を管理するクラス。
//=============================================================
class PlayerBullet {
public:

	/// <summary>
	/// プレイヤーの弾オブジェクトを初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// プレイヤーの弾の更新処理を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// プレイヤーの弾を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// プレイヤーの弾のトレイル（軌跡）を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void DrawTrail(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// 弾がヒットしたかどうかを取得します。
	/// </summary>
	/// <returns>ヒットしている場合 true、それ以外は false</returns>
	bool IsHit() const { return isHit_; }
	/// <summary>
	/// 弾が死亡しているかどうかを取得します。
	/// </summary>
	/// <returns>死亡している場合 true、それ以外は false</returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// 発射時の出現演出としてベジェ曲線移動を開始します。
	/// </summary>
	/// <param name="p0">開始位置</param>
	/// <param name="p1">制御点1</param>
	/// <param name="p2">制御点2</param>
	/// <param name="p3">終了位置</param>
	/// <param name="duration">演出の継続時間（秒）</param>
	/// <param name="velocityAfter">演出終了後に適用する速度</param>
	void StartSpawnBezier(
		const Vector3& p0,
		const Vector3& p1,
		const Vector3& p2,
		const Vector3& p3,
		float duration,
		const Vector3& velocityAfter
	);

	// Getter===================================
	/// <summary>
	/// 弾の位置を取得します。（ワールド座標）
	/// </summary>
	/// <returns></returns>
	Enemy* GetEnemy() { return enemy_; }
	/// <summary>
	/// 弾の位置を取得します。（ワールド座標）(const版)
	/// </summary>
	/// <returns></returns>
	const Enemy* GetEnemy() const { return enemy_; }
	// =========================================
	// Setter===================================
	/// <summary>
	/// プレイヤーの位置を設定します。
	/// </summary>
	/// <param name="pos">設定する位置（ワールド座標）</param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// 弾の速度を設定します。
	/// </summary>
	/// <param name="vel">設定する速度ベクトル</param>
	void SetVelocity(const Vector3& vel);
	/// <summary>
	/// 使用するカメラを設定します。
	/// </summary>
	/// <param name="camera">描画および判定に使用するカメラ</param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// 弾のトレイル（軌跡）グループを設定します。
	/// </summary>
	/// <param name="group">使用するトレイルグループ名</param>
	void SetTrailGroup(const std::string& group);
	/// <summary>
	/// 弾が当たった対象の敵を設定します。
	/// </summary>
	/// <param name="enemy">ヒット対象となる敵（nullptr 可）</param>
	void SetEnemy(Enemy* enemy);
	/// <summary>
	/// プレイヤー参照を設定します。
	/// </summary>
	/// <param name="player">発射元となるプレイヤー</param>
	void SetPlayer(Player* player);
	/// <summary>
	/// 一撃必殺フラグを設定します。
	/// </summary>
	/// <param name="flag">有効にする場合 true、それ以外は false</param>
	void SetSpecialAttack(bool flag);
	/// <summary>
	/// 中ボスコア参照を設定します。
	/// </summary>
	/// <param name="core">中ボスコア（nullptr 可）</param>
	void SetCore(BarrierCore* core);
	/// <summary>
	/// 発射時の出現演出としてベジェ曲線移動を開始します。
	/// </summary>
	/// <param name="p0">開始位置</param>
	void SetUseTrail(bool use);
	/// <summary>
	/// バリアコアマネージャー参照を設定します。
	/// </summary>
	/// <param name="manager">バリアコアマネージャー（nullptr 可）</param>
	void SetBarrierCoreManager(BarrierCoreManager* manager);
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
	BarrierCore* core_ = nullptr;
	BarrierCoreManager* barrierCoreManager_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	//======================================================================
	// 生存状態・ヒットフラグ
	//======================================================================
	bool isDead_ = false; // 死亡フラグ（消滅しているかどうか）
	bool isHit_ = false; // ヒットフラグ（当たったかどうか。死亡とは別に管理）
	bool isSpecialAttack_ = false; // 一撃必殺フラグ
	//======================================================================
	// ホーミング / ベジェ出現フェーズ
	//======================================================================
	bool  isSpawningCurve_ = false;  // 発射の「出方」曲線フェーズ中か
	float spawnT_ = 0.0f;   // 0..1 の補間量
	float spawnDuration_ = 0.25f;  // 出方にかける秒数（調整可）
	Vector3 bezP0_, bezP1_, bezP2_, bezP3_;      // ベジェ制御点
	Vector3 postSpawnVelocity_ = { 0,0,0 };      // 曲線フェーズ終了後に引き継ぐ速度
	float ltRingDistAcc_ = 0.0f; // LT弾：リング間引き（移動距離の蓄積）
	std::vector<Vector3> ltTrailPts_; // LT弾：軌跡点のキュー
	float ltTrailDistAcc_ = 0.0f; // LT弾：軌跡点追加の距離蓄積
	// 点を追加する間隔＆保持数（調整）
	static constexpr float  kLTTrailStep_ = 0.05f; // LT弾：軌跡点追加の距離間隔（LT弾は細かく点を追加）
	static constexpr size_t kLTTrailHardCap_ = 4096; // LT弾：軌跡点のハードキャップ（これ以上は追加しない。安全策）
	float lifeTimer_ = 0.0f; // LT弾の寿命タイマー
	static constexpr float kLifeTime_ = 2.5f; // LT弾の寿命（秒。これを超えたら消える。安全策）
	/// <summary>
	/// LT弾の軌跡（リボン）を更新します。
	/// </summary>
	/// <param name="pos">現在の弾の位置</param>
	void UpdateLTTrail_(const Vector3& pos);
	/// <summary>
	/// 発射の「出方」曲線フェーズ更新。
	/// </summary>
	void UpdateSpawnBezier();
	//======================================================================
	// パーティクル（軌跡）
	//======================================================================
	ParticleEmitter trailEmitter_;               // 弾の軌跡パーティクル
	std::string     trailGroup_ = "bulletTrail"; // デフォルトのパーティクルグループ名
	bool useTrail_ = true; // トレイルを使うかどうか
	//======================================================================
	// 共通パラメータ（マジックナンバー解消）
	//======================================================================
	static constexpr float kDefaultScale_ = 1.3f;  // 弾の見た目サイズ
	static constexpr float kDespawnZ_ = 150.0f; // 消えるZ位置
	//======================================================================
	// その他の定数
	//======================================================================
	const float dt_ = 1.0f / 60.0f; // 更新ごとの想定デルタタイム（秒）。ホーミングの遅延減算などで使用
};