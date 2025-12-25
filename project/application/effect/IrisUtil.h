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
	auto sprite = std::make_unique<TKM::Sprite>();
	sprite->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, texturePath);

	// 画面中央
	sprite->SetAnchorPoint({ 0.5f, 0.5f });
	sprite->SetPosition(
		{ WindowsAPI::kClientWidth * 0.5f, WindowsAPI::kClientHeight * 0.5f });

	// 画面対角長から「絶対にはみ出す」スケールを計算
	const float w = static_cast<float>(WindowsAPI::kClientWidth);
	const float h = static_cast<float>(WindowsAPI::kClientHeight);
	const float diag = std::sqrt(w * w + h * h);

	// 2 倍くらいにしておけば端がチラ見えしない
	outMaxScale = diag * 2.0f;

	sprite->SetSize({ outMaxScale, outMaxScale });

	return sprite;
}

/// ------------------------------------------------------------
/// アイリスのスケール更新処理（開く/閉じるどちらでも使用可）
/// ・tween.Update(dt) した値をそのままスプライトのサイズに適用する
/// ・戻り値として現在スケールを返す
/// ------------------------------------------------------------
inline float UpdateIrisScale(TKM::Sprite* iris, Ease::Tween& tween, float dt)
{
	if (!iris) { return 0.0f; }

	float scale = tween.Update(dt);
	iris->SetSize({ scale, scale });
	iris->Update();
	return scale;
}
