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
		std::unique_ptr<Sprite> uiX_;
		std::unique_ptr<Sprite> uiLS_;
		std::unique_ptr<Sprite> uiRBGaugeIcon_;
		std::string lbTex_;
		std::string rbTex_;
		std::string xTex_;
		std::string lsTex_;
		std::string rbGaugeIconTex_;

		Vector2 lbTexSize_{};
		Vector2 rbTexSize_{};
		Vector2 xTexSize_{};
		Vector2 lsTexSize_{};
		Vector2 rbGaugeIconTexSize_{};

		Vector2 lbDrawSize_{};
		Vector2 rbDrawSize_{};
		Vector2 xDrawSize_{};
		Vector2 lsDrawSize_{};
		Vector2 rbGaugeIconDrawSize_{};

		// 数値で調整するパラメータ
		float rightUiScale_ = 0.20f; // これを変えるだけで大きさ変わる
		float rightUiMargin_ = 20.0f;
		float rightUiSpacing_ = 10.0f;

		// HUD透明度
		float hudAlpha_ = 1.0f;

		// 色（押下で変える）
		Vector4 colLB_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colRB_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colX_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colLS_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colRBGaugeIcon_{ 1.0f,1.0f,1.0f,1.0f };

		// 他UI
		std::unique_ptr<TKM::RBGaugeUI> rbGaugeUI_;

		std::unique_ptr<Sprite> hpFrame_;
		std::unique_ptr<Sprite> hpFill_;
		Vector4 colHPFrame_{ 1.0f,1.0f,1.0f,1.0f };
		Vector4 colHPFill_{ 1.0f,1.0f,1.0f,1.0f };
		Vector2 hpCenter_{};
		// 縦HPゲージ（左の確保スペースに入れる想定）
		Vector2 hpVertSize_{ 22.0f, 365.0f }; // (幅, 高さ)
		Vector2 hpVertOffset_{ -25.0f, -210.0f };  // 微調整（+xで右 / +yで下）
		float   hpFramePad_ = 10.0f;          // フレームの余白（上下左右に足す）
		float   ammoUiRaiseY_ = 60.0f; // 弾UIを通常位置からどれだけ上に上げるか（ピクセル）。これもHUD全体の位置調整用。
		// ===== 左下HUD：配置調整 =====
		float hudLeftMargin_ = 18.5f;     // 画面左端からの余白
		float hudReserveLeftW_ = 87.5f;  // 左側に置く縦長ゲージ分の確保幅
		float hudReserveGap_ = 26.0f;     // 確保幅の右側の間隔（見栄え用）
		float hudBottomMargin_ = 44.0f;   // 画面下からHPバー中心までの距離
		float rbGaugeIconPadX_ = 60.0f;   // RBゲージ右端→アイコンまでの余白

		// ===== 右側UI：個別調整用 =====

		// 個別：スケール
		float lbScale_ = 0.065f;
		float rbScale_ = 0.114f;
		float xScale_ = 0.066f;
		float lsScale_ = 0.064f;
		float rbGaugeIconScale_ = 0.075f;

		// オフセット（右下基準からのズラし）
		Vector2 lbOffset_{ 0.0f, 0.0f };
		Vector2 rbOffset_{ 0.0f, 0.0f };
		Vector2 xOffset_{ 0.0f, 0.0f };
		Vector2 lsOffset_{ 1.0f, -37.5f };
		Vector2 rbGaugeIconOffset_{ -624.0f, 8.5f };

		// 個別：色（押下色を後でいじるなら）
		Vector4 idleCol_{ 1.0f, 1.0f, 1.0f, 0.75f };
		Vector4 onCol_{ 1.0f, 0.25f, 0.25f, 1.0f };

		// シェイク（押下中だけ位置を小刻みにズラす）
		float shakeAmpPx_ = 3.0f;     // 揺れ幅（ピクセル）
		float shakeFreq_ = 45.0f;     // 更新頻度っぽいやつ（大きいほど細かく震える）

		// 押下継続時間（ボタンごと）
		float shakeT_RB_ = 0.0f;
		float shakeT_LB_ = 0.0f;
		float shakeT_X_ = 0.0f;
		float shakeT_RBGaugeIcon_ = 0.0f;

		// 基準座標（ApplyRightUiPositions_で決めた位置を保持）
		Vector2 basePosRB_{};
		Vector2 basePosLB_{};
		Vector2 basePosX_{};
		Vector2 basePosLS_{};
		Vector2 basePosRBGaugeIcon_{};

		// LSの倒し方向で動く量（ピクセル）
		float lsMoveRangePx_ = 10.0f;
		// LSのデッドゾーン（0.0f〜1.0f）
		float lsDeadzone_ = 0.20f;

		// HPアニメ用（被弾時の減りを“ヌルッ”と動かす）
		int   prevHp_ = -1;
		float hpAnimRate_ = 1.0f;      // 表示しているHP割合（0..1）
		float hpTargetRate_ = 1.0f;    // 目標HP割合（0..1）
		float hpDrainSpeed_ = 6.5f;    // 減少時の追従速度（大きいほど速い）
		float hpHealSpeed_ = 10.0f;   // 回復時の追従速度
		float hpHitFlashT_ = 0.0f;    // 被弾フラッシュ残り秒
		float hpHitFlashSec_ = 0.18f; // 被弾フラッシュの持続時間
		float hpShakeT_ = 0.0f;        // 被弾シェイク残り秒
		float hpShakeSec_ = 0.22f; // 被弾シェイクの持続時間
		float hpShakeAmpPx_ = 4.0f;    // シェイク幅
		Vector2 basePosHPFrame_{};     // HPフレーム基準位置
		Vector2 basePosHPFill_{};      // HPフィル基準位置（下基準）

		// ---- HPイージング（減少時の“演出”用）----
		Ease::Tween hpTween_;
		bool  hpTweenActive_ = false;
		float hpDrainEaseSec_ = 1.0f;              // 減少アニメ時間
		float hpHealEaseSec_ = 0.12f;              // 回復アニメ時間（任意）
		Ease::Type hpDrainEaseType_ = Ease::Type::OutElastic; // 減少アニメのイージングタイプ

		/// <summary>
		/// 押下中はスプライトの位置を小刻みにズラしてシェイクさせる処理。tは押下継続時間で、これを元に揺れのオフセットを計算します。
		/// </summary>
		/// <param name="sp">揺らす対象のスプライト</param>
		/// <param name="basePos">揺らす前の基準位置</param>
		/// <param name="down">そのボタンが押されているかどうか</param>
		/// <param name="t">押下継続時間。これを元に揺れのオフセットを計算します。</param>
		void ApplyShake_(Sprite* sp, const Vector2& basePos, bool down, float& t);
		/// <summary>
		/// 右側UI（LT / LB / RB）のサイズを、テクスチャサイズと rightUiScale_ を元に計算して適用します。
		/// </summary>
		void ApplyHudPositions_();
	};
}