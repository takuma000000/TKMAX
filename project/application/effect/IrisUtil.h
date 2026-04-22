#pragma once
#include <memory>
#include <cmath>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "DirectXCommon.h"
#include "WindowsAPI.h"
#include "Easing.h"

//=============================================================
// IrisUtilクラス
// アイリス演出に関連するユーティリティ関数をまとめたクラス。
//=============================================================

/// ------------------------------------------------------------
/// 画面中央に配置されたアイリス用スプライトを生成し、
/// 画面全体を覆える「最大スケール」を計算して返すユーティリティ。
/// ------------------------------------------------------------
inline std::unique_ptr<TKM::Sprite> CreateCenteredIrisSprite(
	TKM::DirectXCommon* dxCommon,
	float& outMaxScale,
	const char* texturePath = "./resources/texture/circle2.png") // texturePathは、アイリスに使用するテクスチャのパス。デフォルトは円形のテクスチャ（circle2.png）を想定。
{
	// アイリス用スプライトの生成
	auto sprite_ = std::make_unique<TKM::Sprite>(); 
	sprite_->Initialize(TKM::SpriteCommon::GetInstance(), dxCommon, texturePath);

	// 画面中央
	sprite_->SetAnchorPoint({ 0.5f, 0.5f }); // アイリスの中心が画面中央に来るようにアンカーポイントを設定
	sprite_->SetPosition(
		{ TKM::WindowsAPI::GetClientWidth() * 0.5f, TKM::WindowsAPI::GetClientHeight() * 0.5f }); // アイリスの中心が画面中央に来るように位置を設定

	// 画面対角長から「絶対にはみ出す」スケールを計算
	const float w_ = static_cast<float>(TKM::WindowsAPI::GetClientWidth()); // 画面の幅
	const float h_ = static_cast<float>(TKM::WindowsAPI::GetClientHeight()); // 画面の高さ
	const float diag_ = std::sqrt(w_ * w_ + h_ * h_);

	// 2 倍くらいにしておけば端がチラ見えしない
	outMaxScale = diag_ * 2.0f;
	// アイリスのサイズを最大スケールに設定
	sprite_->SetSize({ outMaxScale, outMaxScale });
	// アイリスの初期スケールは小さめにしておく
	return sprite_;
}

/// ------------------------------------------------------------
/// アイリスのスケール更新処理（開く/閉じるどちらでも使用可）
/// ・tween.Update(dt) した値をそのままスプライトのサイズに適用する
/// ・戻り値として現在スケールを返す
/// ------------------------------------------------------------
inline float UpdateIrisScale(TKM::Sprite* iris, Ease::Tween& tween, float dt)
{
	if (!iris) { return 0.0f; } // 念のためnullptrチェック

	float scale_ = tween.Update(dt); // tween.Update(dt) した値をそのままスプライトのサイズに適用
	iris->SetSize({ scale_, scale_ }); // スプライトのサイズを更新
	iris->Update(); // スプライトの更新（必要に応じて）
	return scale_;
}
