#pragma once
#include <memory>

#include "SpriteCommon.h"
#include "MyMath.h"
#include "Player.h"
#include "OperationGuideUI.h"
#include "PlayerHudUI.h"
#include "SkipGuideUI.h"

namespace TKM {

	//=============================================================
	// UIControllerクラス
	// UI全体の窓口だけを担当するクラス。
	// 実際の表示ロジックは各役割クラスへ委譲します。
	//=============================================================
	class UIController {
	public:
		/// <summary>
		/// UIControllerを初期化します。
		/// </summary>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			BaseScene* parentScene,
			float screenW,
			float screenH,
			Player* player
		);
		/// <summary>
		/// UI全体を更新します。
		/// </summary>
		void Update(float dt);
		/// <summary>
		/// UI全体を描画します。
		/// </summary>
		void Draw();
		/// <summary>
		/// UI全体のImGuiを表示します。
		/// </summary>
		void DrawImGui();

		/// <summary>
		/// 画面サイズ変更時に再レイアウトします。
		/// </summary>
		void UpdateLayout(float screenW, float screenH);

		// Setter========================================
		/// <summary>
		/// HUD全体の透明度を設定します。
		/// </summary>
		void SetHudAlpha(float a);
		/// <summary>
		/// 右側操作UIの余白を設定します。
		/// </summary>
		void SetRightUiMargin(float px);
		/// <summary>
		/// 右側操作UIの縦間隔を設定します。
		/// </summary>
		void SetRightUiSpacing(float px);
		/// <summary>
		/// 開幕ボス演出中かどうかを設定します。
		/// true の間は SkipGuideUI だけを表示し、
		/// false になったら通常HUDを表示します。
		/// </summary>
		void SetIntroSkipUiActive(bool active);
		/// <summary>
		/// 通常HUD（操作UI / HP / RB / LB）を表示するかを設定します。
		/// </summary>
		void SetGameplayHudVisible(bool visible);
		// ==============================================

	private:
		//=============================================================
		// 共通参照
		//=============================================================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		float screenW_ = 0.0f; // 画面幅
		float screenH_ = 0.0f; // 画面高さ
		//=============================================================
		// HUD全体の透明度
		//=============================================================
		float hudAlpha_ = 1.0f; // HUD全体の透明度（0.0f〜1.0f）
		bool introSkipUiActive_ = false; // 開幕ボス演出中だけ true
		bool gameplayHudVisible_ = true; // ゲームスタート後だけ true
		//=============================================================
		// 各UIの実体
		//=============================================================
		std::unique_ptr<OperationGuideUI> operationGuideUI_; // 右側の操作UI（ボタンアイコンと説明テキスト）
		std::unique_ptr<PlayerHudUI> playerHudUI_; // プレイヤーのHPや残弾数などを表示するHUD
		std::unique_ptr<SkipGuideUI> skipGuideUI_; // イントロのボススタートムービーをスキップするためのUI
		//=============================================================
		// Playerから通知されたHUD状態
		//=============================================================
		Player::HudState hudState_{};
	};

} // namespace TKM