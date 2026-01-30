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
			None,
			Resume,
			Restart,
			ReturnToTitle,
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

		enum class State {
			Closed,
			Pausing,
			Paused,
			Resuming,
		};

		enum class Item {
			Resume = 0,
			Restart,
			ReturnToTitle,
			Count
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

		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;

		float screenW_ = 0.0f;
		float screenH_ = 0.0f;

		State state_ = State::Closed;
		int index_ = 0;

		// 演出
		float fadeT_ = 0.0f;        // 0→1（Pausing/Resuming の進行）
		float pulseTime_ = 0.0f;    // 選択中の脈動用（フェードとは分離）
		float curtainAlpha_ = 0.0f; // 暗幕のアルファ

		// 入力のエッジ検出用
		bool prevStart_ = false;
		bool prevUp_ = false;
		bool prevDown_ = false;
		bool prevA_ = false;
		bool prevB_ = false;

		// 仮スプライト（後でリソース差し替え）
		std::unique_ptr<Sprite> curtain_;     // 暗幕
		std::unique_ptr<Sprite> panel_;       // パネル
		std::array<std::unique_ptr<Sprite>, (int)Item::Count> items_; // 項目（仮）
		std::unique_ptr<Sprite> cursor_;      // カーソル（仮）

		// レイアウト
		Vector2 panelPos_{};
		Vector2 panelSize_{};
		Vector2 baseItemPos_{};
		float itemSpacingY_ = 64.0f;
	};
} // namespace TKM