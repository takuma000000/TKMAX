#pragma once
#include <memory>
#include <array>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "Input.h"
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;
	class BaseScene;

	class PauseMenuController {
	public:
		enum class Command {
			None, // なし
			Resume, // 再開
			Restart, // はじめから
			ReturnToTitle, // タイトルへ戻る
		};
		// 設定構造体
		struct Desc {
			std::string curtainTex = "./resources/gradationLine.png"; // 暗幕
			std::string panelTex = "./resources/gradationLine.png"; // パネル
			std::array<std::string, 3> itemTex = {
				"./resources/resume_pause.png", // Resume
				"./resources/restart_pause.png", // Restart
				"./resources/title_pause.png"  // ReturnToTitle
			};
			std::string cursorTex = "./resources/circle2.png"; // カーソル
		};

		/// <summary>
		/// ポーズメニューを初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="parentScene">所属する親シーン</param>
		/// <param name="screenW">画面幅（ピクセル）</param>
		/// <param name="screenH">画面高さ（ピクセル）</param>
		/// <param name="desc">ポーズメニュー設定情報</param>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			BaseScene* parentScene,
			float screenW,
			float screenH,
			const Desc& desc = Desc()
		);
		/// <summary>
		/// ポーズメニューの更新処理を行います。
		/// </summary>
		/// <param name="dt">
		/// デルタタイム（rawDt 推奨）。
		/// ポーズ中でも UI アニメーションを進行させるため、停止スケール未適用の値を想定します。
		/// </param>
		/// <param name="allowOpen">ポーズメニューを開くことを許可する場合 true</param>
		/// <returns>発行されたコマンド（何もなければ None 等）</returns>
		Command Update(float dt, bool allowOpen);
		/// <summary>
		/// ポーズメニューを描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// 画面サイズ変更に応じてレイアウトを更新します。
		/// </summary>
		/// <param name="screenW">画面幅（ピクセル）</param>
		/// <param name="screenH">画面高さ（ピクセル）</param>
		void UpdateLayout(float screenW, float screenH);
		/// <summary>
		/// ポーズ状態かどうかを取得します。
		/// </summary>
		/// <returns>ポーズ中の場合 true、それ以外は false</returns>
		bool IsPaused() const { return state_ != State::Closed; }

	private:
		Desc desc_{};

		// 状態
		enum class State {
			Closed, // 閉じている
			Pausing, // 開いている途中
			Paused, // 開いている
			Resuming, // 閉じている途中
		};
		// メニュー項目
		enum class Item {
			Resume = 0, // 再開
			Restart, // はじめから
			ReturnToTitle, // タイトルへ戻る
			Count // 項目数
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
		bool TriggerB_();
		/// <summary>
		/// ポーズメニューを開きます。
		/// </summary>
		void Open_();
		/// <summary>
		/// ポーズメニューを閉じます。
		/// </summary>
		void Close_();
		/// <summary>
		/// 選択インデックスを移動します。
		/// </summary>
		/// <param name="delta">インデックスの増減量（正数で下、負数で上）</param>
		void MoveIndex_(int delta);

		//==============================
		// 参照
		//==============================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		//==============================
		// 画面情報
		//==============================
		float screenW_ = 0.0f; // 画面幅
		float screenH_ = 0.0f; // 画面高さ
		//==============================
		// 状態
		//==============================
		State state_ = State::Closed; // 現在の状態
		int index_ = 0; // 選択中の項目インデックス
		//==============================
		// 演出
		//==============================
		float fadeT_ = 0.0f;        // 0→1（Pausing/Resuming の進行）
		float pulseTime_ = 0.0f;    // 選択中の脈動用（フェードとは分離）
		float curtainAlpha_ = 0.0f; // 暗幕のアルファ
		//==============================
		// 入力のエッジ検出用
		//==============================
		bool prevStart_ = false; // スタートボタン
		bool prevUp_ = false; // 上ボタン
		bool prevDown_ = false; // 下ボタン
		bool prevA_ = false; // Aボタン
		bool prevB_ = false; // Bボタン
		//==============================
		// 仮スプライト（後でリソース差し替え）
		//==============================
		std::unique_ptr<Sprite> curtain_;     // 暗幕
		std::unique_ptr<Sprite> panel_;       // パネル
		std::array<std::unique_ptr<Sprite>, (int)Item::Count> items_; // 項目（仮）
		std::unique_ptr<Sprite> cursor_;      // カーソル（仮）
		//==============================
		// レイアウト
		//==============================
		Vector2 panelPos_{}; // パネル位置
		Vector2 panelSize_{}; // パネルサイズ
		Vector2 baseItemPos_{}; // 項目基準位置
		float itemSpacingY_ = 64.0f; // 項目間隔
	};
} // namespace TKM