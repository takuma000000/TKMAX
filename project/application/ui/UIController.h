#pragma once
#include <memory>
#include <string>

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
		/// UIControllerを初期化します。
		/// </summary>
		/// <param name="spriteCommon">SpriteCommonのインスタンス</param>
		/// <param name="dxCommon">DirectXCommonのインスタンス</param>
		/// <param name="parentScene">このUIControllerを使用するシーン</param>
		/// <param name="screenW">画面幅</param>
		/// <param name="screenH">画面高さ</param>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			BaseScene* parentScene,
			float screenW,
			float screenH
		);
		/// <summary>
		/// UIControllerを終了処理します。
		/// </summary>
		/// <param name="dt"></param>
		/// <param name="player"></param>
		void Update(float dt, Player* player);
		/// <summary>
		/// UIControllerを描画します。
		/// </summary>
		void Draw();
		/// <summary>
		/// UIControllerのImGui表示を行います。
		/// </summary>
		void DrawImGui();
		/// <summary>
		/// 画面サイズの変更に伴うUIレイアウトの更新を行います。
		/// </summary>
		/// <param name="screenW">新しい画面幅</param>
		/// <param name="screenH">新しい画面高さ</param>
		void UpdateLayout(float screenW, float screenH);

		// Setter========================================
		/// <summary>
		/// HUD全体の透明度を設定します（0.0f〜1.0f）。これを変えるだけでHUD全体の明るさが変わります。
		/// </summary>
		/// <param name="a">透明度（0.0f〜1.0f）</param>
		void SetHudAlpha(float a);
		/// <summary>
		/// 右側UI（LT / LB / RB）のスケールを設定します。これを変えるだけで右側UI全体の大きさが変わります。
		/// </summary>
		/// <param name="s">スケール（例: 0.20f）</param>
		void SetRightUiScale(float s);
		/// <summary>
		/// 右側UI（LT / LB / RB）の画面端からの余白を設定します。これを変えるだけで右側UI全体の位置が変わります。
		/// </summary>
		/// <param name="px">余白（ピクセル）</param>
		void SetRightUiMargin(float px);
		/// <summary>
		/// 右側UI（LT / LB / RB）同士の間隔を設定します。これを変えるだけで右側UI全体の位置が変わります。
		/// </summary>
		/// <param name="px">間隔（ピクセル）</param>
		void SetRightUiSpacing(float px);
		// ==============================================

	private:
		/// <summary>
		/// テクスチャパスとアンカーを指定してスプライトを生成するヘルパー関数。
		/// </summary>
		/// <param name="texPath">テクスチャのファイルパス</param>
		/// <param name="anchor">アンカー（例: {1.0f, 1.0f}）</param>
		/// <param name="outTexSize">テクスチャサイズの出力先（幅, 高さ）</param>
		/// <returns>生成されたスプライトのユニークポインタ</returns>
		std::unique_ptr<Sprite> CreateSprite_(const std::string& texPath, const Vector2& anchor, Vector2* outTexSize);
		/// <summary>
		/// 右側UI（LT / LB / RB）のサイズを、テクスチャサイズと rightUiScale_ を元に計算して適用します。
		/// </summary>
		void ApplyRightUiSizes_();
		/// <summary>
		/// 右側UI（LT / LB / RB）の位置を、画面サイズと rightUiMargin_、rightUiSpacing_ を元に計算して適用します。
		/// </summary>
		void ApplyRightUiPositions_();

		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;

		float screenW_ = 0.0f;
		float screenH_ = 0.0f;

		// 右側UI（LB / RB）
		std::unique_ptr<Sprite> uiLB_;
		std::unique_ptr<Sprite> uiRB_;
		std::string lbTex_;
		std::string rbTex_;

		Vector2 lbTexSize_{};
		Vector2 rbTexSize_{};

		Vector2 lbDrawSize_{};
		Vector2 rbDrawSize_{};

		// 数値で調整するパラメータ
		float rightUiScale_ = 0.20f; // これを変えるだけで大きさ変わる
		float rightUiMargin_ = 20.0f;
		float rightUiSpacing_ = 10.0f;

		// HUD透明度
		float hudAlpha_ = 1.0f;

		// 色（押下で変える）
		Vector4 colLB_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colRB_{ 1.0f,1.0f,1.0f,1.0f };

		// 他UI（既存）
		std::unique_ptr<TKM::RBGaugeUI> rbGaugeUI_;

		std::unique_ptr<Sprite> hpFrame_;
		std::unique_ptr<Sprite> hpFill_;
		Vector4 colHPFrame_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colHPFill_{ 1.0f,1.0f,1.0f,1.0f };
		Vector2 hpCenter_{};
		Vector2 hpSize_{ 520.0f, 18.0f };
		float   ammoUiRaiseY_ = 60.0f;

		// ===== 右側UI：個別調整用 =====

		// 個別：スケール
		float lbScale_ = 0.065f;
		float rbScale_ = 0.114f;

		// オフセット（右下基準からのズラし）
		Vector2 lbOffset_{ 0.0f, 0.0f };
		Vector2 rbOffset_{ 0.0f, 0.0f };

		// 個別：色（押下色を後でいじるなら）
		Vector4 idleCol_{ 1.0f, 1.0f, 1.0f, 0.75f };
		Vector4 onCol_{ 1.0f, 0.25f, 0.25f, 1.0f };

	};
}