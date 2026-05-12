#pragma once
#include <memory>
#include <vector>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "MyMath.h"
#include "ActionEnemy.h"

//=============================================================
// ActionPlayerMagic
// Kキーで発動するプレイヤー魔法を管理するクラス
//=============================================================
class ActionPlayerMagic {
public:
	void Initialize(TKM::DirectXCommon* dxCommon);
	void Update(
		const Vector2& playerPosition,
		const Vector2& playerSize,
		float facingDirection,
		float scrollX,
		float screenWidth,
		std::vector<std::unique_ptr<ActionEnemy>>& enemies
	);
	void Draw(float scrollX);

	bool IsPlayerControlLocked() const {
		return phase_ == Phase::Charge || phase_ == Phase::Rain;
	}

private:
	enum class Phase {
		Idle,
		Charge,
		Rain,
		Finish
	};

	struct MagicArrow {
		std::unique_ptr<TKM::Sprite> sprite;
		Vector2 position;
		float fallSpeed;
		bool active;
	};

private:
	ActionEnemy* FindTargetEnemy_(
		const Vector2& playerPosition,
		float facingDirection,
		float scrollX,
		float screenWidth,
		std::vector<std::unique_ptr<ActionEnemy>>& enemies
	);

	void StartMagic_(ActionEnemy* target, const Vector2& playerPosition);
	void UpdateCharge_();
	void UpdateRain_();
	void FinishMagic_();

	void CreateSprite_(
		std::unique_ptr<TKM::Sprite>& sprite,
		const std::string& texturePath,
		const Vector2& size,
		const Vector4& color
	);

	bool HasTargetEnemy_(const std::vector<std::unique_ptr<ActionEnemy>>& enemies) const;

private:
	TKM::DirectXCommon* dxCommon_ = nullptr;

	Phase phase_ = Phase::Idle;

	ActionEnemy* targetEnemy_ = nullptr;

	std::unique_ptr<TKM::Sprite> auraSprite_ = nullptr;
	std::unique_ptr<TKM::Sprite> portalSprite_ = nullptr;

	std::vector<MagicArrow> arrows_;

	Vector2 auraPosition_ = {};
	Vector2 portalPosition_ = {};

	float timer_ = 0.0f;
	float scrollX_ = 0.0f;

	static constexpr float kFrameTime_ = 1.0f / 60.0f;

	static constexpr float kChargeTime_ = 1.0f;
	static constexpr float kRainTime_ = 1.2f;

	static constexpr int kArrowCount_ = 8;

	static constexpr float kAuraWidth_ = 80.0f;
	static constexpr float kAuraHeight_ = 80.0f;

	static constexpr float kPortalWidth_ = 120.0f;
	static constexpr float kPortalHeight_ = 28.0f;

	static constexpr float kArrowWidth_ = 10.0f;
	static constexpr float kArrowHeight_ = 150.0f;
};