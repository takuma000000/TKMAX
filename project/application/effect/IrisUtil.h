#pragma once
#include <memory>
#include <cmath>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "WindowsAPI.h"
#include "Easing.h"

/// ------------------------------------------------------------
/// 画面中央に配置されたアイリス用スプライトを生成し、
/// 画面全体を覆える「最大スケール」を計算して返すユーティリティ。
/// ------------------------------------------------------------
inline std::unique_ptr<TKM::Sprite> CreateCenteredIrisSprite(
	TKM::DirectXCommon* dxCommon,
	float& outMaxScale,
	const char* texturePath = "./resources/circle2.png")
{
	auto sprite_ = std::make_unique<TKM::Sprite>();
	sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, texturePath);

	// 画面中央
	sprite_->SetAnchorPoint({ 0.5f, 0.5f });
	sprite_->SetPosition(
		{ TKM::WindowsAPI::kClientWidth_ * 0.5f, TKM::WindowsAPI::kClientHeight_ * 0.5f });

	// 画面対角長から「絶対にはみ出す」スケールを計算
	const float w_ = static_cast<float>(TKM::WindowsAPI::kClientWidth_);
	const float h_ = static_cast<float>(TKM::WindowsAPI::kClientHeight_);
	const float diag_ = std::sqrt(w_ * w_ + h_ * h_);

	// 2 倍くらいにしておけば端がチラ見えしない
	outMaxScale = diag_ * 2.0f;

	sprite_->SetSize({ outMaxScale, outMaxScale });

	return sprite_;
}

/// ------------------------------------------------------------
/// アイリスのスケール更新処理（開く/閉じるどちらでも使用可）
/// ・tween.Update(dt) した値をそのままスプライトのサイズに適用する
/// ・戻り値として現在スケールを返す
/// ------------------------------------------------------------
inline float UpdateIrisScale(TKM::Sprite* iris, Ease::Tween& tween, float dt)
{
	if (!iris) { return 0.0f; }

	float scale_ = tween.Update(dt);
	iris->SetSize({ scale_, scale_ });
	iris->Update();
	return scale_;
}
