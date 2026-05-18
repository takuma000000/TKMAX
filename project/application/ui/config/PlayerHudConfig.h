#pragma once
#include <string>
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// PlayerHudConfigクラス
	// PlayerHudUIの外部設定を管理するクラス。
	//=============================================================
	class PlayerHudConfig {
	public:
		/// <summary>
		/// テクスチャ設定
		/// </summary>
		struct TextureConfig {
			std::string rbGaugeIconTex_; // RBゲージアイコンテクスチャ
			std::string hpFrameTex_;    // HPゲージフレームテクスチャ
			std::string hpFillTex_;     // HPゲージフィルテクスチャ
			std::string hpIconTex_;     // HPゲージアイコンテクスチャ
		};
		/// <summary>
		/// RBゲージアイコン設定
		/// </summary>
		struct RBGaugeIconConfig {
			float scale_;      // アイコンのスケール
			Vector2 offset_;   // アイコンのオフセット
			float padX_;       // アイコン同士の水平間隔
			Vector4 color_;    // RGBA
			float shakeAmpPx_; // シェイクの振れ幅（ピクセル）
		};
		/// <summary>
		/// LBゲージ設定
		/// </summary>
		struct LBGaugeConfig {
			float spacingY_; // ゲージ同士の垂直間隔
			Vector2 offset_; // ゲージのオフセット
		};
		/// <summary>
		/// HPゲージ設定
		/// </summary>
		struct HPGaugeConfig {
			// HPゲージのシェイク設定
			float shakePower_; // シェイクの振れ幅（ピクセル）
			// HPゲージの色設定
			Vector4 frameColor_; // フレームの色
			Vector4 fillColor_;  // フィルの色
			Vector4 iconColor_;  // アイコンの色
			// HPゲージのサイズと位置の設定
			Vector2 size_;   // ゲージのサイズ（幅、高さ）
			Vector2 offset_; // ゲージのオフセット（アイコンからの位置）
			float framePad_; // フレームとゲージの隙間
			// HPアイコンの設定
			Vector2 iconOffset_; // アイコンのオフセット
			float iconScale_;    // アイコンのスケール
			// HPゲージのセグメント設定
			int segmentCount_;   // HPゲージのセグメント数
			float segmentGap_;   // セグメント同士の隙間
			float segmentMinW_;  // セグメントの幅の最小値（HP残り1/28のときの幅）
			float segmentMaxW_;  // セグメントの幅の最大値（HP満タン時の幅）
			float segmentSkewX_; // セグメントの傾き（X軸方向）
			// セグメントの色設定
			Vector4 backSegmentColor_; // 空のセグメントの色
			Vector4 outerFrameColor_;  // 外枠の色
			Vector2 outerFramePad_;    // 外枠のパッド
		};
		/// <summary>
		/// レイアウト設定
		/// </summary>
		struct LayoutConfig {
			float ammoUiRaiseY_;    // 弾薬UIを通常位置からどれだけ上に上げるか
			float hudLeftMargin_;   // HUDの左端から画面左端までの距離
			float hudReserveLeftW_; // LBゲージの左端からHUDの左端までの距離
			float hudReserveGap_;   // LBゲージとRBゲージの間の距離
			float hudBottomMargin_; // HUDの下端から画面下端までの距離
		};
		/// <summary>
		/// HPエフェクト設定
		/// </summary>
		struct HpEffectConfig {
			float hitFlashSec_;  // ヒットフラッシュの継続時間
			float shakeSec_;     // シェイクの継続時間
			float shakeAmpPx_;   // シェイクの振れ幅（ピクセル）
			float drainEaseSec_; // HP減少のイージング時間
		};

		/// <summary>
		/// JSONファイルから設定を読み込む。
		/// </summary>
		/// <param name="path">JSONファイルのパス</param>
		/// <returns>読み込み成功ならtrue、失敗ならfalse</returns>
		bool Load(const char* path);
		/// <summary>
		/// JSONファイルから設定を読み込む内部処理。Load()から呼び出される。
		/// </summary>
		/// <param name="path">JSONファイルのパス</param>
		/// <returns>読み込み成功ならtrue、失敗ならfalse</returns>
		bool LoadJson(const char* path);

		// Getter=======================================
		/// <summary>
		/// テクスチャ設定を取得する。
		/// </summary>
		/// <returns>テクスチャ設定</returns>
		const TextureConfig& GetTexture() const { return texture_; }
		/// <summary>
		/// RBゲージアイコン設定を取得する。
		/// </summary>
		/// <returns>RBゲージアイコン設定</returns>
		const RBGaugeIconConfig& GetRBGaugeIcon() const { return rbGaugeIcon_; }
		/// <summary>
		/// LBゲージ設定を取得する。
		/// </summary>
		/// <returns>LBゲージ設定</returns>
		const LBGaugeConfig& GetLBGauge() const { return lbGauge_; }
		/// <summary>
		/// HPゲージ設定を取得する。
		/// </summary>
		/// <returns>HPゲージ設定</returns>
		const HPGaugeConfig& GetHPGauge() const { return hpGauge_; }
		/// <summary>
		/// レイアウト設定を取得する。
		/// </summary>
		/// <returns>レイアウト設定</returns>
		const LayoutConfig& GetLayout() const { return layout_; }
		/// <summary>
		/// HPエフェクト設定を取得する。
		/// </summary>
		/// <returns>HPエフェクト設定</returns>
		const HpEffectConfig& GetHpEffect() const { return hpEffect_; }
		// =============================================

	private:
		/// <summary>
		/// ファイルパスが指定した拡張子を持っているかをチェックする。
		/// </summary>
		/// <param name="path">ファイルパス</param>
		/// <param name="ext">拡張子（例: ".json"）</param>
		/// <returns>持っていればtrue、持っていなければfalse</returns>
		static bool HasExtension(const std::string& path, const char* ext);

		//============================================
		// 設定データ
		//============================================
		TextureConfig texture_;         // テクスチャ設定
		RBGaugeIconConfig rbGaugeIcon_; // RBゲージアイコン設定
		LBGaugeConfig lbGauge_;         // LBゲージ設定
		HPGaugeConfig hpGauge_;         // HPゲージ設定
		LayoutConfig layout_;           // レイアウト設定
		HpEffectConfig hpEffect_;       // HPエフェクト設定
	};

}