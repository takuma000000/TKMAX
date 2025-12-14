#pragma once
#include "BaseEffect.h"

/// LT弾用の「衝撃ラジアルブラー」エフェクト
class RadialBlurEffect : public BaseEffect {
public:
	void Initialize(DirectXCommon* dx) override;
	void Update(float dt) override;
	void Draw() override;

	/// LT弾発射時に呼ぶ
	void BulrStartShock(float strength = 1.0f, float duration = 0.35f);

	/// 有効かどうか（必要なら使う用）
	bool IsActive() const { return active_; }

private:
	bool  active_ = false;
	float timer_ = 0.0f;
	float duration_ = 0.35f;
	float maxStrength_ = 1.0f;   // 今は未使用だが拡張用
};