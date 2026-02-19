#pragma once
#include <memory>
#include <array>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "Input.h"
#include "MyMath.h"

class DirectXCommon;
class BaseScene;

class TitleMenuController {
public:
	enum class Command {
		None, // なし
		Start, // はじめる
		Exit, // とじる
	};

	struct Desc {
		std::string panelTex = "./resources/texture/gradationLine.png"; // パネル
		std::array<std::string, 2> itemTex = {
			"./resources/texture/start_title.png", // はじめる
			"./resources/texture/end_title.png",  // とじる
		};
		std::string cursorTex = "./resources/texture/circle2.png"; // カーソル
	};
	/// <summary>
	/// タイトルメニューを初期化します。
	/// </summary>
	/// <param name="spriteCommon">スプライト共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	/// <param name="parentScene">所属する親シーン</param>
	/// <param name="screenW">画面幅（ピクセル）</param>
	/// <param name="screenH">画面高さ（ピクセル）</param>
	/// <param name="desc">タイトルメニュー設定情報</param>
	void Initialize(
		TKM::SpriteCommon* spriteCommon,
		TKM::DirectXCommon* dxCommon,
		TKM::BaseScene* parentScene,
		float screenW,
		float screenH,
		const Desc& desc = Desc()
	);
	/// <summary>
	/// タイトルメニューの更新処理を行います。
	/// </summary>
	/// <param name="dt">デルタタイム（秒）</param>
	/// <returns>発行されたコマンド（何もなければ None 等）</returns>
	Command Update(float dt);
	/// <summary>
	/// タイトルメニューを描画します。
	/// </summary>
	void Draw();
	/// <summary>
	/// 画面サイズの変更に伴うレイアウト更新を行います。
	/// </summary>
	/// <param name="screenW">画面幅（ピクセル）</param>
	/// <param name="screenH">画面高さ（ピクセル）</param>
	void UpdateLayout(float screenW, float screenH);

private:
	enum class Item { // メニュー項目
		Start = 0, // はじめる
		Exit, // とじる
		Count // 項目数
	};
	/// <summary>
	/// 上ボタン入力がトリガーされたかを判定します。
	/// </summary>
	/// <returns>入力が発生したフレームで true、それ以外は false</returns>
	bool TriggerPadUp_();
	/// <summary>
	/// 下ボタン入力がトリガーされたかを判定します。
	/// </summary>
	/// <returns>入力が発生したフレームで true、それ以外は false</returns>
	bool TriggerPadDown_();
	/// <summary>
	/// A ボタン入力がトリガーされたかを判定します。
	/// </summary>
	/// <returns>入力が発生したフレームで true、それ以外は false</returns>
	bool TriggerA_();
	/// <summary>
	/// B ボタン入力がトリガーされたかを判定します。
	/// </summary>
	/// <returns>入力が発生したフレームで true、それ以外は false</returns>
	void MoveIndex_(int delta);
	
	//======================================================================
	// 設定 / 参照
	//======================================================================
	Desc desc_{}; // 設定
	TKM::SpriteCommon* spriteCommon_ = nullptr;
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::BaseScene* parentScene_ = nullptr;
	float screenW_ = 0.0f; // 画面幅
	float screenH_ = 0.0f; // 画面高さ
	//======================================================================
	// 状態
	//======================================================================
	int index_ = 0; // 選択中の項目インデックス
	//======================================================================
	// 入力（トリガー判定用）
	//======================================================================
	bool prevUp_ = false; // 上入力の前フレーム状態
	bool prevDown_ = false; // 下入力の前フレーム状態
	bool prevA_ = false; // Aボタンの前フレーム状態
	//======================================================================
	// スプライト
	//======================================================================
	std::unique_ptr<TKM::Sprite> panel_; // パネル
	std::array<std::unique_ptr<TKM::Sprite>, (int)Item::Count> items_; // 項目
	std::unique_ptr<TKM::Sprite> cursor_; // カーソル
	//======================================================================
	// レイアウト
	//======================================================================
	Vector2 panelPos_{}; // パネル位置
	Vector2 panelSize_{}; // パネルサイズ
	Vector2 baseItemPos_{}; // 項目基準位置
	float itemSpacingY_ = 64.0f; // 項目間隔
	//======================================================================
	// 選択アニメーション（拡大・追従）
	//======================================================================
	std::array<float, (int)Item::Count> itemScale_{}; // 項目ごとの現在の拡大率
	Vector2 itemSizeNormal_{ 260.0f, 48.0f }; // 項目の通常サイズ
	float selectScale_ = 1.18f;   // 選択中倍率
	float scaleSpeed_ = 14.0f;    // 追従速度（大きいほどキビキビ）
	//======================================================================
	// 脈動アニメーション
	//======================================================================
	float pulseTime_ = 0.0f; // 選択中の脈動
};