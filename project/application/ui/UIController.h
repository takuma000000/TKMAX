#pragma once
#include <memory>
#include <string>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"
#include "RBGaugeUI.h"
#include "Player.h"
#include <Input.h>
#include "LBGaugeUI.h"

namespace TKM {

	//=============================================================
	// UIControllerクラス
	// ゲーム全体のUI管理を行うクラス。
	//=============================================================
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

		//======================================================================
		// 参照ポインタ / 共通
		//======================================================================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		float screenW_ = 0.0f; // 画面幅（レイアウト計算用）
		float screenH_ = 0.0f; // 画面高さ（レイアウト計算用）
		//======================================================================
		// 右側UI（LB / RB）
		//======================================================================
		std::unique_ptr<Sprite> uiLB_; // LBアイコンのスプライト
		std::unique_ptr<Sprite> uiRB_; // RBアイコンのスプライト
		std::unique_ptr<Sprite> uiX_; // Xアイコンのスプライト
		std::unique_ptr<Sprite> uiLS_; // LSアイコンのスプライト
		std::unique_ptr<Sprite> uiRBGaugeIcon_; // RBゲージアイコンのスプライト

		std::string lbTex_; // LBアイコンのテクスチャパス
		std::string rbTex_; // RBアイコンのテクスチャパス
		std::string xTex_; // Xアイコンのテクスチャパス
		std::string lsTex_; // LSアイコンのテクスチャパス
		std::string rbGaugeIconTex_; // RBゲージアイコンのテクスチャパス

		Vector2 lbTexSize_{}; // LBアイコンのテクスチャサイズ
		Vector2 rbTexSize_{}; // RBアイコンのテクスチャサイズ
		Vector2 xTexSize_{}; // Xアイコンのテクスチャサイズ
		Vector2 lsTexSize_{}; // LSアイコンのテクスチャサイズ
		Vector2 rbGaugeIconTexSize_{}; // RBゲージアイコンのテクスチャサイズ

		Vector2 lbDrawSize_{}; // LBアイコンの描画サイズ（テクスチャサイズを元に rightUiScale_ で計算して入れる）
		Vector2 rbDrawSize_{}; // RBアイコンの描画サイズ（テクスチャサイズを元に rightUiScale_ で計算して入れる）
		Vector2 xDrawSize_{}; // Xアイコンの描画サイズ（テクスチャサイズを元に rightUiScale_ で計算して入れる）
		Vector2 lsDrawSize_{}; // LSアイコンの描画サイズ（テクスチャサイズを元に rightUiScale_ で計算して入れる）
		Vector2 rbGaugeIconDrawSize_{}; // RBゲージアイコンの描画サイズ（テクスチャサイズを元に rightUiScale_ で計算して入れる）

		// 数値で調整するパラメータ
		float rightUiScale_ = 0.20f; // 右側UI全体のスケール（これを変えるだけでLB/RB/X/LSアイコン全ての大きさが変わる）
		float rightUiMargin_ = 20.0f; // 右側UI全体の画面端からの余白（これを変えるだけでLB/RB/X/LSアイコン全ての位置が変わる）
		float rightUiSpacing_ = 10.0f; // 右側UI同士の間隔（これを変えるだけでLB/RB/X/LSアイコン全ての位置が変わる）

		// HUD透明度
		float hudAlpha_ = 1.0f; // 0.0f〜1.0fでHUD全体の透明度を調整。これを変えるだけでHUD全体の明るさが変わる。

		// 色（押下で変える）
		Vector4 colLB_{ 1.0f,1.0f,1.0f,1.0f }; // LBアイコンの色
		Vector4 colRB_{ 1.0f,1.0f,1.0f,1.0f }; // RBアイコンの色
		Vector4 colX_{ 1.0f,1.0f,1.0f,1.0f }; // Xアイコンの色
		Vector4 colLS_{ 1.0f,1.0f,1.0f,1.0f }; // LSアイコンの色
		Vector4 colRBGaugeIcon_{ 1.0f,1.0f,1.0f,1.0f }; // RBゲージアイコンの色
		//======================================================================
		// RB残弾UI
		//======================================================================
		std::unique_ptr<TKM::RBGaugeUI> rbGaugeUI_; // RB残弾ゲージUI
		//======================================================================
		// LB残弾UI
		//======================================================================
		std::unique_ptr<TKM::LBGaugeUI> lbGaugeUI_; // LB残弾ゲージUI
		float lbGaugeSpacingY_ = 52.0f; // RBゲージの下にどれだけ離して置くか
		Vector2 lbGaugeOffset_{ 0.0f, 0.0f }; // LBゲージの位置微調整（+xで右 / +yで下）
		//======================================================================
		// 左下HUD（HP）
		//======================================================================
		std::unique_ptr<Sprite> hpFrame_; // HPフレームのスプライト
		std::unique_ptr<Sprite> hpFill_; // HPフィルのスプライト
		Vector4 colHPFrame_{ 1.0f,1.0f,1.0f,1.0f }; // HPフレームの色
		Vector4 colHPFill_{ 1.0f,1.0f,1.0f,1.0f }; // HPフィルの色
		// 縦HPゲージ（左の確保スペースに入れる想定）
		Vector2 hpVertSize_{ 22.0f, 365.0f }; // (幅, 高さ)
		Vector2 hpVertOffset_{ -25.0f, -210.0f }; // 微調整（+xで右 / +yで下）
		float   hpFramePad_ = 10.0f; // フレームの余白（上下左右に足す）
		float   ammoUiRaiseY_ = 60.0f; // 弾UIを通常位置からどれだけ上に上げるか（ピクセル）。これもHUD全体の位置調整用。
		// ===== 左下HUD：配置調整 =====
		float hudLeftMargin_ = 18.5f;     // 画面左端からの余白
		float hudReserveLeftW_ = 87.5f;  // 左側に置く縦長ゲージ分の確保幅
		float hudReserveGap_ = 26.0f;     // 確保幅の右側の間隔（見栄え用）
		float hudBottomMargin_ = 44.0f;   // 画面下からHPバー中心までの距離
		float rbGaugeIconPadX_ = 60.0f;   // RBゲージ右端→アイコンまでの余白

		// HPアイコン（player_hp.png）をゲージの下に置く
		std::unique_ptr<Sprite> hpIcon_;
		Vector2 hpIconTexSize_{}; // HPアイコンのテクスチャサイズ
		Vector2 hpIconDrawSize_{}; // HPアイコンの描画サイズ（テクスチャサイズを元に hpIconScale_ で計算して入れる）
		Vector2 hpIconOffset_{ -1.0f, -4.0f }; // +yで下にズラす（下に置くので正）
		float   hpIconScale_ = 0.055f;        // 画像に合わせて調整
		Vector4 colHPIcon_{ 1.0f,1.0f,1.0f,1.0f }; // HPアイコンの色
		//======================================================================
		// 右側UI：個別調整用
		//======================================================================
		// 個別：スケール
		float lbScale_ = 0.065f; // LBスケール
		float rbScale_ = 0.114f; // RBスケール
		float xScale_ = 0.066f; // Xスケール
		float lsScale_ = 0.064f; // LSスケール
		float rbGaugeIconScale_ = 0.075f; // RBゲージアイコンのスケール
		// オフセット（右下基準からのズラし）
		Vector2 lbOffset_{ 0.0f, 0.0f }; // LBアイコンの位置微調整（+xで右 / +yで下）
		Vector2 rbOffset_{ 0.0f, 0.0f }; // RBアイコンの位置微調整（+xで右 / +yで下）
		Vector2 xOffset_{ 0.0f, 0.0f }; // Xアイコンの位置微調整（+xで右 / +yで下）
		Vector2 lsOffset_{ 1.0f, -37.5f }; // LSアイコンの位置微調整（+xで右 / +yで下）
		Vector2 rbGaugeIconOffset_{ -624.0f, 8.5f }; // RBゲージアイコンの位置微調整（+xで右 / +yで下）
		// 色
		Vector4 idleCol_{ 1.0f, 1.0f, 1.0f, 0.75f }; // 通常の色
		Vector4 onCol_{ 1.0f, 0.25f, 0.25f, 1.0f }; // 押下中の色
		// シェイク（押下中だけ位置を小刻みにズラす）
		float shakeAmpPx_ = 3.0f;     // 揺れ幅（ピクセル）
		// 押下継続時間（ボタンごと）
		float shakeT_RB_ = 0.0f; // RBの押下継続時間
		float shakeT_LB_ = 0.0f; // LBの押下継続時間
		float shakeT_X_ = 0.0f; // Xの押下継続時間
		float shakeT_RBGaugeIcon_ = 0.0f; // RBゲージアイコンの押下継続時間
		// 基準座標（ApplyRightUiPositions_で決めた位置を保持）
		Vector2 basePosRB_{}; // RBアイコンの基準位置
		Vector2 basePosLB_{}; // LBアイコンの基準位置
		Vector2 basePosX_{}; // Xアイコンの基準位置
		Vector2 basePosLS_{}; // LSアイコンの基準位置
		Vector2 basePosRBGaugeIcon_{}; // RBゲージアイコンの基準位置
		// LSの倒し方向で動く量（ピクセル）
		float lsMoveRangePx_ = 10.0f;
		// LSのデッドゾーン（0.0f〜1.0f）
		float lsDeadzone_ = 0.20f;
		//======================================================================
		// HPアニメ用（被弾時の減りを“ヌルッ”と動かす）
		//======================================================================
		int   prevHp_ = -1; // 前フレームのHP。これと現在のHPを比べて減ってたらアニメ開始。
		float hpAnimRate_ = 1.0f;      // 表示しているHP割合（0..1）
		float hpTargetRate_ = 1.0f;    // 目標HP割合（0..1）
		float hpHitFlashT_ = 0.0f;    // 被弾フラッシュ残り秒
		float hpHitFlashSec_ = 0.18f; // 被弾フラッシュの持続時間
		float hpShakeT_ = 0.0f;        // 被弾シェイク残り秒
		float hpShakeSec_ = 0.22f; // 被弾シェイクの持続時間
		float hpShakeAmpPx_ = 4.0f;    // シェイク幅
		Vector2 basePosHPFrame_{};     // HPフレーム基準位置
		Vector2 basePosHPFill_{};      // HPフィル基準位置（下基準）
		// ---- HPイージング（減少時の“演出”用）----
		Ease::Tween hpTween_; // HP割合のイージング用Tween
		bool  hpTweenActive_ = false; // HPイージングがアクティブかどうか
		float hpDrainEaseSec_ = 1.0f; // 減少アニメ時間
		Ease::Type hpDrainEaseType_ = Ease::Type::OutElastic; // 減少アニメのイージングタイプ
	};
}