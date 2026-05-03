#pragma once
#include <memory>
#include <vector>
#include "DirectXCommon.h"
#include "MyMath.h"
#include "ActionPlayerBullet.h"
#include "ActionEnemy.h"

//=============================================================
// ActionPlayerBulletManager
// プレイヤー弾の生成・更新・描画を管理するクラス
//=============================================================
class ActionPlayerBulletManager {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update(
		const Vector2& playerPosition,
		const Vector2& playerSize,
		float facingDirection,
		std::vector<std::unique_ptr<ActionEnemy>>& enemies
	);
	void Draw(float scrollX);

private:
	void Shoot_(const Vector2& playerPosition, const Vector2& playerSize, float facingDirection);

private:
	TKM::DirectXCommon* dxCommon_ = nullptr;

	std::vector<std::unique_ptr<ActionPlayerBullet>> bullets_;

	static constexpr float kShotCooldownSec_ = 0.25f;
	static constexpr float kFrameTime_ = 1.0f / 60.0f;

	float shotCooldownTimer_ = 0.0f;

	void CheckHitEnemies_(std::vector<std::unique_ptr<ActionEnemy>>& enemies);
};