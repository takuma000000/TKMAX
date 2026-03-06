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
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw(TKM::DirectXCommon* dxCommon);
	void DrawTrail(TKM::DirectXCommon* dxCommon);

	bool IsDead() const { return isDead_; }
	bool IsHit() const { return isHit_; }

	Enemy* GetEnemy() { return enemy_; }
	const Enemy* GetEnemy() const { return enemy_; }

	void SetPosition(const Vector3& pos);
	void SetCamera(TKM::Camera* camera);
	void SetEnemy(Enemy* enemy);
	void SetPlayer(Player* player);
	void SetCore(MidBossCore* core);

	void StartArc(
		const Vector3& start,
		const Vector3& control1,
		const Vector3& control2,
		const Vector3& end,
		float duration
	);

private:
	void UpdateTrail_(const Vector3& p);

private:
	Player* player_ = nullptr;
	Enemy* enemy_ = nullptr;
	MidBossCore* core_ = nullptr;
	TKM::Camera* camera_ = nullptr;

	std::unique_ptr<TKM::Object3d> object_;

	bool isDead_ = false;
	bool isHit_ = false;

	Vector3 prevPos_ = { 0,0,0 };

	// 山なり弾道
	Vector3 p0_ = { 0,0,0 };
	Vector3 p1_ = { 0,0,0 };
	Vector3 p2_ = { 0,0,0 };
	Vector3 p3_ = { 0,0,0 };
	float arcT_ = 0.0f;
	float arcDuration_ = 0.55f;
	bool isArcActive_ = false;

	// トレイル
	std::vector<Vector3> trailPts_;
	float trailDistAcc_ = 0.0f;

	// 定数
	static constexpr float dt_ = 1.0f / 60.0f; // 更新間隔の想定値（秒）
	static constexpr float kDefaultScale_ = 0.8f; // 弾の基本スケール
	static constexpr float kLifeTime_ = 1.2f; // 一定時間経過で消える
	static constexpr float kTrailStep_ = 0.15f; // トレイルの点を追加する距離の閾値
	static constexpr size_t kTrailHardCap_ = 64; // トレイルの最大点数（これ以上は古い点から削除される）
	static constexpr int kEnemyDamage_ = 50; // 敵へのダメージ量
	static constexpr int kCoreDamage_ = 10; // コアへのダメージは小さめ

	float lifeTimer_ = 0.0f;

	float sparkleDistAcc_ = 0.0f; // スパークエフェクトを出すための距離の累積値

	bool isTrailFading_ = false; // トレイルが消え始めているかどうか
	float trailFadeTimer_ = 0.0f; // トレイルが消え始めてからの経過時間
	float trailFadeInterval_ = 0.025f; // トレイルが完全に消えるまでの時間（秒）
};