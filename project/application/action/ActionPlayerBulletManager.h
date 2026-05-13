#pragma once
#include <memory>
#include <vector>
#include "DirectXCommon.h"
#include "MyMath.h"
#include "ActionPlayerBullet.h"
#include "ActionEnemy.h"
#include "ActionBlock.h"
#include "Sprite.h"

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
		std::vector<std::unique_ptr<ActionEnemy>>& enemies,
		const std::vector<std::unique_ptr<ActionBlock>>& blocks,
		float scrollX,
		float screenWidth
	);
	void Draw(float scrollX);

private:
	void Shoot_(const Vector2& playerPosition, const Vector2& playerSize, float facingDirection);
	void CheckHitBlocks_(const std::vector<std::unique_ptr<ActionBlock>>& blocks);
	void CheckHitEnemyBullets_(std::vector<std::unique_ptr<ActionEnemy>>& enemies);
	void KillOutOfScreen_(float scrollX, float screenWidth);
	void UpdateClashEffects_();
	void SpawnClashEffect_(const Vector2& worldCenter);

	TKM::DirectXCommon* dxCommon_ = nullptr;

	std::vector<std::unique_ptr<ActionPlayerBullet>> bullets_;

	static constexpr float kShotCooldownSec_ = 0.25f;
	static constexpr float kFrameTime_ = 1.0f / 60.0f;

	float shotCooldownTimer_ = 0.0f;

	void CheckHitEnemies_(std::vector<std::unique_ptr<ActionEnemy>>& enemies);

	struct ClashEffect {
		Vector2 worldCenter = { 0.0f, 0.0f };
		float timer = 0.0f;
	};
	std::vector<ClashEffect> clashEffects_;
	std::unique_ptr<TKM::Sprite> clashEffectSprite_ = nullptr;
	static constexpr float kClashEffectDuration_ = 0.18f;
	static constexpr float kClashEffectBaseSize_ = 20.0f;
};