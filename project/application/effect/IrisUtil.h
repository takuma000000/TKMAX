#pragma once
#include <memory>
#include <cmath>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "WindowsAPI.h"
#include "Easing.h"

//=============================================================
// IrisUtil
// アイリス演出に使うスプライト生成とスケール更新をまとめたユーティリティ。
//=============================================================

/// ------------------------------------------------------------
/// 画面中央に配置されたアイリス用スプライトを生成し、
/// 画面全体を覆える「最大スケール」を計算して返す。
/// ------------------------------------------------------------
inline std::unique_ptr<TKM::Sprite> CreateCenteredIrisSprite(
	TKM::DirectXCommon* dxCommon,
	float& outMaxScale,
	const char* texturePath = "./resources/texture/circle2.png") {
	// アイリス用スプライトを生成する
	auto sprite_ = std::make_unique<TKM::Sprite>();

	// 指定されたテクスチャでスプライトを初期化する
	sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, texturePath);

	// スプライトの中心を基準にして拡縮できるようにする
	sprite_->SetAnchorPoint({ 0.5f, 0.5f });

	// 画面中央に配置する
	sprite_->SetPosition(
		{ TKM::WindowsAPI::GetClientWidth() * 0.5f, TKM::WindowsAPI::GetClientHeight() * 0.5f });

	// 画面幅を取得する
	const float w_ = static_cast<float>(TKM::WindowsAPI::GetClientWidth());

	// 画面高さを取得する
	const float h_ = static_cast<float>(TKM::WindowsAPI::GetClientHeight());

	// 画面対角線の長さを計算する
	const float diag_ = std::sqrt(w_ * w_ + h_ * h_);

	// 画面端が見切れないように、対角線より大きい最大サイズを設定する
	outMaxScale = diag_ * 2.0f;

	// 初期サイズとして最大スケールを設定する
	sprite_->SetSize({ outMaxScale, outMaxScale });

	// 生成したアイリス用スプライトを返す
	return sprite_;
}

/// ------------------------------------------------------------
/// アイリスのスケール更新処理。
/// 開く演出・閉じる演出のどちらでも使用できる。
/// ------------------------------------------------------------
inline float UpdateIrisScale(TKM::Sprite* iris, Ease::Tween& tween, float dt) {
	// スプライトが無ければ更新できないので0を返す
	if (!iris) { return 0.0f; }

	// Tweenを進めて現在のスケール値を取得する
	float scale_ = tween.Update(dt);

	// 現在スケールをスプライトサイズに反映する
	iris->SetSize({ scale_, scale_ });

	// スプライトを更新する
	iris->Update();

	// 現在スケールを返す
	return scale_;
}