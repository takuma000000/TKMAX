#pragma once
#include <memory>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

//=============================================================
// ActionPlayer
// 2Dアクション用プレイヤー
//=============================================================
class ActionPlayer {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update();
	void Draw(float scrollX);
	void ImGuiDebug();

	void TakeDamage();
	/// <summary>
	/// ダメージを受けられるかどうか。ダメージを受けた後は一定時間無敵になる。
	/// </summary>
	/// <returns></returns>
	bool CanTakeDamage() const { return damageCooldownTimer_ <= 0.0f; }

	const Vector2& GetPosition() const { return position_; }
	Vector2 GetSize() const { return { kPlayerWidth_, kPlayerHeight_ }; }

	AABB GetAABB() const {
		return AABB(
			{
				position_.x + kPlayerWidth_ * 0.5f,
				position_.y + kPlayerHeight_ * 0.5f,
				0.0f
			},
		{
			kPlayerWidth_,
			kPlayerHeight_,
			1.0f
		}
		);
	}

	bool IsDead() const { return hp_ <= 0; }
	int GetHP() const { return hp_; }
	int GetMaxHP() const { return kMaxHP_; }
	void SetGroundTopY(float groundTopY) {
		groundY_ = groundTopY - kPlayerHeight_;
	}
	void SetStageWidth(float stageWidth) {
		stageWidth_ = stageWidth;
	}

private:
	std::unique_ptr<TKM::Sprite> sprite_ = nullptr;

	Vector2 position_ = { 300.0f, 400.0f };
	Vector2 velocity_ = { 0.0f, 0.0f };

	bool isGrounded_ = false;

	float moveSpeed_ = 6.0f;
	float jumpPower_ = -18.0f;
	float gravity_ = 0.8f;
	float groundY_ = 500.0f;

	static constexpr float kPlayerWidth_ = 48.0f;
	static constexpr float kPlayerHeight_ = 48.0f;

	static constexpr int kMaxHP_ = 3;
	int hp_ = kMaxHP_;

	static constexpr float kDamageCooldownSec_ = 2.0f;
	static constexpr float kFrameTime_ = 1.0f / 60.0f;

	float damageCooldownTimer_ = 0.0f;

	static constexpr float kBlinkInterval_ = 0.1f; // 点滅間隔

	float stageWidth_ = 1280.0f;
};