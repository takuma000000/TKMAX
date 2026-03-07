#pragma once

#define NOMINMAX
#include <memory>
#include <vector>
#include "Object3d.h"
#include "MyMath.h"

class Player;
class Enemy;
class MidBossCore;

//=============================================================
// HomingBulletクラス
// LB専用のホーミング弾を管理するクラス。
//=============================================================
class HomingBullet {
public:
	/// <summary>
	/// ホーミング弾を初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// ホーミング弾の更新処理を行います。
	/// </summary>
	void Update();
	/// <summary>
	/// ホーミング弾を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// ホーミング弾のトレイル（軌跡）を描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void DrawTrail(TKM::DirectXCommon* dxCommon);

	/// <summary>
	/// ホーミング弾の山なり弾道を開始します。
	/// </summary>
	/// <param name="start">開始位置</param>
	/// <param name="control1">ベジェ曲線の制御点1</param>
	/// <param name="control2">ベジェ曲線の制御点2</param>
	/// <param name="end">終了位置</param>
	/// <param name="duration">弾道の継続時間（秒）</param>
	void StartArc(
		const Vector3& start,
		const Vector3& control1,
		const Vector3& control2,
		const Vector3& end,
		float duration
	);

	/// <summary>
	/// ホーミング弾が死亡しているかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsDead() const { return isDead_; }
	/// <summary>
	/// ホーミング弾が敵に当たったかどうかを取得します。
	/// </summary>
	/// <returns></returns>
	bool IsHit() const { return isHit_; }

	// Getter===================================
	/// <summary>
	/// ホーミング弾の位置を取得します。
	/// </summary>
	/// <returns></returns>
	Enemy* GetEnemy() { return enemy_; }
	/// <summary>
	/// ホーミング弾の位置を取得します。(const版)
	/// </summary>
	/// <returns></returns>
	const Enemy* GetEnemy() const { return enemy_; }
	// =========================================
	// Setter===================================
	
	/// <summary>
	/// ホーミング弾の位置を設定します。
	/// </summary>
	/// <param name="pos"></param>
	void SetPosition(const Vector3& pos);
	/// <summary>
	/// ホーミング弾のスケールを設定します。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// ホーミング弾のターゲットとなる敵を設定します。
	/// </summary>
	/// <param name="enemy"></param>
	void SetEnemy(Enemy* enemy);
	/// <summary>
	/// ホーミング弾のターゲットとなるプレイヤーを設定します。
	/// </summary>
	/// <param name="player"></param>
	void SetPlayer(Player* player);
	/// <summary>
	/// ホーミング弾のターゲットとなるコアを設定します。
	/// </summary>
	/// <param name="core"></param>
	void SetCore(MidBossCore* core);
	// =========================================

private:
	/// <summary>
	/// ホーミング弾の山なり弾道を更新します。
	/// </summary>
	/// <param name="p"></param>
	void UpdateTrail_(const Vector3& p);

	//==============================================
	// メンバ変数
	//==============================================
	Player* player_ = nullptr;
	Enemy* enemy_ = nullptr;
	MidBossCore* core_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	//==============================================
	// 描画オブジェクト
	//==============================================
	std::unique_ptr<TKM::Object3d> object_; // ホーミング弾のモデルオブジェクト
	//==============================================
	// 状態フラグ
	//==============================================
	bool isDead_ = false; // ホーミング弾が死亡しているかどうか
	bool isHit_ = false; // ホーミング弾が敵に当たったかどうか
	bool isArcActive_ = false; // 山なり弾道がアクティブかどうか
	bool isTrailFading_ = false; // トレイルが消え始めているかどうか
	//==============================================
	// 位置・移動
	//==============================================
	Vector3 prevPos_ = { 0,0,0 }; // 前フレームの位置（トレイル更新のため）
	//==============================================
	// 山なり弾道
	//==============================================
	// 山なり弾道のためのベジェ曲線の4点とパラメータ
	Vector3 p0_ = { 0,0,0 }; // ベジェ曲線の4点（開始位置、制御点1、制御点2、終了位置）
	Vector3 p1_ = { 0,0,0 };
	Vector3 p2_ = { 0,0,0 };
	Vector3 p3_ = { 0,0,0 };
	float arcT_ = 0.0f; // ベジェ曲線のパラメータ（0〜1）
	float arcDuration_ = 0.55f; // 山なり弾道の継続時間（秒）
	//==============================================
	// トレイル
	//==============================================
	std::vector<Vector3> trailPts_; // トレイルの点のリスト
	float trailDistAcc_ = 0.0f; // 前回トレイルを更新した位置からの距離の累積
	float trailFadeTimer_ = 0.0f; // トレイルが消え始めてからの経過時間
	float trailFadeInterval_ = 0.015f; // トレイルが完全に消えるまでの時間（秒）
	//==============================================
	// 定数
	//==============================================
	static constexpr float dt_ = 1.0f / 60.0f; // 更新間隔の想定値（秒）
	static constexpr float kDefaultScale_ = 0.8f; // 弾の基本スケール
	static constexpr float kLifeTime_ = 3.0f; // 一定時間経過で消える
	static constexpr float kTrailStep_ = 0.15f; // トレイルの点を追加する距離の閾値
	static constexpr size_t kTrailHardCap_ = 64; // トレイルの最大点数（これ以上は古い点から削除される）
	static constexpr int kEnemyDamage_ = 50; // 敵へのダメージ量
	static constexpr int kCoreDamage_ = 10; // コアへのダメージは小さめ
	//==============================================
	// タイマー
	//==============================================
	float lifeTimer_ = 0.0f; // 弾が存在してからの経過時間
};