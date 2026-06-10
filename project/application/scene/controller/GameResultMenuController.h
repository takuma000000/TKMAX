#pragma once
#include <memory>
#include <array>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "Input.h"
#include "MyMath.h"
#include "PostEffectController.h"

//=============================================================
// GameResultMenuControllerクラス
// ゲーム結果メニューの管理を行うクラス。
//=============================================================
class GameResultMenuController {
public:
	enum class Command {
		None,            // コマンドなし
		Restart,         // リスタート
		RestartFromBoss, // ボス戦からリスタート
		ReturnToTitle,   // タイトルに戻る
	};

	struct Desc {
		std::array<std::string, 3> itemTex = { //
			"./resources/texture/restart_pause.png", // リスタート
			"./resources/texture/resume_boss.png",   // ボス戦からリスタート
			"./resources/texture/title_pause.png",   // タイトルに戻る
		};
		std::string cursorTex = "./resources/texture/circle2.png"; // カーソル
	};

	/// <summary>
	/// ゲーム結果メニューを初期化します。
	/// </summary>
	/// <param name="spriteCommon">スプライト共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	/// <param name="parentScene">所属する親シーン</param>
	/// <param name="screenW">画面幅（ピクセル）</param>
	/// <param name="screenH">画面高さ（ピクセル）</param>
	/// <param name="desc">ゲーム結果メニュー設定情報</param>
	void Initialize(
		TKM::SpriteCommon* spriteCommon,
		TKM::DirectXCommon* dxCommon,
		TKM::BaseScene* parentScene,
		float screenW,
		float screenH,
		const Desc& desc = Desc()
	);
	/// <summary>
	/// ゲーム結果メニューの更新処理を行います。
	/// </summary>
	/// <param name="dt">デルタタイム（rawDt 推奨）。</param>
	/// <returns>発行されたコマンド（何もなければ None 等）</returns>
	Command Update(float dt);
	/// <summary>
	/// ゲーム結果メニューを描画します。
	/// </summary>
	void Draw();
	/// <summary>
	/// 画面サイズの変更に伴うレイアウト更新を行います。
	/// </summary>
	/// <param name="screenW">画面幅（ピクセル）</param>
	/// <param name="screenH">画面高さ（ピクセル）</param>
	void UpdateLayout(float screenW, float screenH);

	// Setter=====================================
	/// <summary>
	/// ボス戦からリスタートの項目の表示・非表示を設定します。
	/// </summary>
	/// <param name="visible">表示する場合は true、非表示にする場合は false</param>
	void SetBossRetryVisible(bool visible);
	// ===========================================

private:
	enum class Item { // コマンドと対応させる
		Restart = 0,     // リスタート
		RestartFromBoss, // ボス戦からリスタート
		ReturnToTitle,   // タイトルに戻る
		Count            // 項目数
	};

	/// <summary>
	/// パッドの上入力がトリガーされたかを判定します。
	/// </summary>
	/// <returns>入力が発生したフレームで true、それ以外は false</returns>
	bool TriggerPadUp_();
	/// <summary>
	/// パッドの下入力がトリガーされたかを判定します。
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
	/// <summary>
	/// 値を 0〜1 の範囲にクランプします。
	/// </summary>
	/// <param name="v">クランプする値</param>
	/// <returns>クランプされた値</returns>
	static float Clamp01_(float v);

	//======================================================================
	// 設定 / 参照
	//======================================================================
	Desc desc_{}; // メニュー設定情報
	TKM::SpriteCommon* spriteCommon_ = nullptr;
	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::BaseScene* parentScene_ = nullptr;
	float screenW_ = 0.0f; // 画面幅
	float screenH_ = 0.0f; // 画面高さ
	//======================================================================
	// 状態
	//======================================================================
	int index_ = 0;              // 選択中の項目インデックス
	bool showBossRetry_ = false; // ボス戦からリスタートの項目を表示するかどうか
	//======================================================================
	// 入力（トリガー判定用）
	//======================================================================
	bool prevUp_ = false; // 上入力の前フレーム状態
	bool prevDown_ = false; // 下入力の前フレーム状態
	bool prevA_ = false; // A ボタンの前フレーム状態
	//======================================================================
	// スプライト
	//======================================================================
	std::array<std::unique_ptr<TKM::Sprite>, (int)Item::Count> items_; // 項目
	std::unique_ptr<TKM::Sprite> cursor_; // カーソル
	//======================================================================
	// レイアウト
	//======================================================================
	Vector2 panelPos_{}; // パネル位置
	Vector2 panelSize_{}; // パネルサイズ（幅・高さ）
	Vector2 baseItemPos_{}; // 項目基準位置（パネルに対する相対座標）
	float itemSpacingY_ = 64.0f; // 項目間隔
	//======================================================================
	// アニメーション
	//======================================================================
	float pulseTime_ = 0.0f; // 選択中の項目の脈動用（フェードとは分離）
};