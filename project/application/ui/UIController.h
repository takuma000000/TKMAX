#pragma once
#include <memory>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"
#include "RBGaugeUI.h"
#include "Player.h"
#include <Input.h>

namespace TKM {
	class UIController {
	public:

		/// <summary>
		/// UI を初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="parentScene">所属する親シーン</param>
		/// <param name="screenW">画面幅（ピクセル）</param>
		/// <param name="screenH">画面高さ（ピクセル）</param>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			BaseScene* parentScene,
			float screenW,
			float screenH
		);
		/// <summary>
		/// UI の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="player">参照対象となるプレイヤー</param>
		void Update(float dt, Player* player);
		/// <summary>
		/// UI を描画します。
		/// </summary>
		void Draw();

		/// <summary>
		/// 画面サイズ変更に応じてレイアウトを更新します。
		/// </summary>
		/// <param name="screenW">画面幅（ピクセル）</param>
		/// <param name="screenH">画面高さ（ピクセル）</param>
		void UpdateLayout(float screenW, float screenH);

		/// <summary>
		/// HUD 全体の透明度を設定します（1.0=通常, 0.0=非表示）。
		/// </summary>
		/// <param name="a">HUD透明度</param>
		void SetHudAlpha(float a);

	private:
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;

		std::unique_ptr<Sprite> uiLT_;
		std::unique_ptr<Sprite> uiLB_;
		std::unique_ptr<Sprite> uiRB_;

		std::unique_ptr<TKM::RBGaugeUI> rbGaugeUI_;

		// HUD透明度（ポーズ中はここを下げる）
		float hudAlpha_ = 1.0f;

		// 現在の見た目色（Draw側で hudAlpha_ を掛け直すために保持）
		Vector4 colLT_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colLB_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colRB_{ 1.0f,1.0f,1.0f,1.0f };

		// --- HPバー ---
		std::unique_ptr<Sprite> hpFrame_;
		std::unique_ptr<Sprite> hpFill_;
		Vector4 colHPFrame_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colHPFill_{ 1.0f,1.0f,1.0f,1.0f };

		Vector2 hpCenter_{};
		Vector2 hpSize_{ 520.0f, 18.0f }; // RBGaugeUIと同じ幅にして「その枠」に入れる
		float   ammoUiRaiseY_ = 60.0f;    // 弾UIを上に上げる量（好みで調整）
	};
}