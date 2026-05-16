#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Sprite.h"
#include "SpriteCommon.h"
#include "MyMath.h"
#include "RBGaugeUI.h"
#include "LBGaugeUI.h"
#include "Player.h"
#include "Easing.h"

namespace TKM {

	// =============================================================
	// プレイヤーHUDUIクラス
	// プレイヤーのHPや残弾数を表示するHUDの管理クラスです。
	// =============================================================
	class PlayerHudUI {
	public:
		/// <summary>
		/// プレイヤーHUDを初期化します。
		/// </summary>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			BaseScene* parentScene,
			float screenW,
			float screenH
		);
		/// <summary>
		/// プレイヤーHUDを更新します。
		/// </summary>
		void Update(float dt);
		/// <summary>
		/// プレイヤーHUDを描画します。
		/// </summary>
		/// <param name="hudAlpha">UI全体アルファ</param>
		void Draw(float hudAlpha);
		/// <summary>
		/// ImGui調整を表示します。
		/// </summary>
		void DrawImGui();

		/// <summary>
		/// 画面サイズ変更時の再レイアウトを行います。
		/// </summary>
		void UpdateLayout(float screenW, float screenH);
		/// <summary>
		/// プレイヤーのHPが変化したときの処理を行います。
		/// </summary>
		/// <param name="state">HUDの状態情報</param>
		void OnHudStateChanged(const Player::HudState& state);

	private:
		/// <summary>
		/// 左下HUDの位置をまとめて再計算して適用します。
		/// </summary>
		void ApplyHudPositions_();
		/// <summary>
		/// HPセグメントゲージの位置を反映します。
		/// </summary>
		void ApplyHpSegmentPositions_(const Vector2& shakeOffset = { 0.0f, 0.0f });

		//=============================================================
		// 共通参照
		//=============================================================
		SpriteCommon* spriteCommon_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		BaseScene* parentScene_ = nullptr;
		float screenW_ = 0.0f; // 画面幅
		float screenH_ = 0.0f; // 画面高さ
		//=============================================================
		// 残弾UI
		//=============================================================
		std::unique_ptr<RBGaugeUI> rbGaugeUI_; // RBゲージUI
		std::unique_ptr<LBGaugeUI> lbGaugeUI_; // LBゲージUI
		//=============================================================
		// RBゲージアイコン
		//=============================================================
		std::unique_ptr<Sprite> rbGaugeIcon_; // RBゲージアイコン
		std::string rbGaugeIconTex_ = "./resources/texture/RB_gauge_ui.png"; // RBゲージアイコンのテクスチャパス
		Vector2 rbGaugeIconTexSize_{}; // テクスチャサイズ
		Vector2 rbGaugeIconDrawSize_{}; // 描画サイズ
		float rbGaugeIconScale_ = 0.075f; // 描画サイズ = テクスチャサイズ * この値
		Vector2 rbGaugeIconOffset_{ -624.0f, 8.5f }; // アイコンの基準位置からのオフセット
		float rbGaugeIconPadX_ = 60.0f; // アイコンとゲージの間隔
		Vector4 colRBGaugeIcon_{ 1.0f, 1.0f, 1.0f, 1.0f }; // アイコンの色
		Vector2 basePosRBGaugeIcon_{}; // アイコンの基準位置（画面サイズ変更時に再計算して保存）
		float shakeAmpPx_ = 3.0f; // アイコンの揺れの強さ（ピクセル）
		float shakeT_RBGaugeIcon_ = 0.0f; // アイコンの揺れの経過時間
		//=============================================================
		// LBゲージ配置
		//=============================================================
		float lbGaugeSpacingY_ = 52.0f; // LBゲージ同士の垂直間隔
		Vector2 lbGaugeOffset_{ 0.0f, 0.0f }; // LBゲージの基準位置からのオフセット
		//=============================================================
		// HPゲージ
		//=============================================================
		std::unique_ptr<Sprite> hpFrame_; // HPゲージのフレーム
		std::unique_ptr<Sprite> hpFill_; // HPゲージの塗り部分
		std::unique_ptr<Sprite> hpIcon_; // HPゲージのアイコン
		std::vector<std::unique_ptr<Sprite>> hpOuterFrameSegments_; // HPゲージ外側フレーム用セグメント
		// HPシェイクの状態
		float hpShakePower_ = 3.0f; // HPシェイクの強さ（ピクセル）
		// テクスチャパス
		Vector4 colHPFrame_{ 1.0f, 1.0f, 1.0f, 0.90f }; // HPゲージフレームの色
		Vector4 colHPFill_{ 0.25f, 1.0f, 0.35f, 0.90f }; // HPゲージ塗りの色
		Vector4 colHPIcon_{ 1.0f, 1.0f, 1.0f, 1.0f }; // HPゲージアイコンの色
		// サイズ・位置
		Vector2 hpVertSize_{ 22.0f, 365.0f }; // HPゲージの幅と高さ（フレーム・塗り共通）
		Vector2 hpVertOffset_{ -25.0f, -210.0f }; // HPゲージの基準位置からのオフセット
		float hpFramePad_ = 10.0f; // HPゲージフレームの内側余白（フレームと塗りの間隔）
		// HPアイコン
		Vector2 hpIconTexSize_{}; // テクスチャサイズ
		Vector2 hpIconDrawSize_{}; // 描画サイズ
		Vector2 hpIconOffset_{ -15.0f, -4.0f }; // 準位置からのオフセット
		float hpIconScale_ = 0.058f; // 画サイズ = テクスチャサイズ * この値
		// 基準位置（画面サイズ変更時に再計算して保存）
		Vector2 basePosHPFrame_{}; // HPゲージフレームの基準位置
		Vector2 basePosHPFill_{}; // HPゲージ塗りの基準位置
		// HPゲージセグメント
		std::vector<std::unique_ptr<Sprite>> hpBackSegments_; // HPゲージの背景セグメント
		std::vector<std::unique_ptr<Sprite>> hpFillSegments_; // HPゲージの塗りセグメント
		// HPゲージセグメントの配置
		int hpSegmentCount_ = 28; // HPゲージの分割数
		float hpSegmentGap_ = 3.5f; // セグメント同士の隙間
		float hpSegmentMinW_ = 13.0f; // 下側の細さ
		float hpSegmentMaxW_ = 42.0f; // 上側の太さ
		float hpSegmentSkewX_ = 0.0f; // 縦一列に揃えるため、Xずらしは使わない
		// HPゲージセグメントの色
		Vector4 colHPBackSegment_{ 0.18f, 0.18f, 0.18f, 0.75f }; // HP背景セグメント色
		// HPゲージセグメントの色
		Vector4 colHPOuterFrame_{ 0.08f, 0.08f, 0.8f, 0.75f }; // 外側フレーム色
		Vector2 hpOuterFramePad_{ 18.0f, 20.0f }; // 外側フレームの余白
		//=============================================================
		// 左下HUDの配置調整
		//=============================================================
		float ammoUiRaiseY_ = 60.0f; // 弾数UI全体を上に持ち上げる量（画面下の余白）
		float hudLeftMargin_ = 18.5f; // 左端の余白
		float hudReserveLeftW_ = 87.5f; // 左端からHPゲージの開始位置までの距離
		float hudReserveGap_ = 26.0f; // HPゲージの確保スペースの右端とRBゲージアイコンの間の距離
		float hudBottomMargin_ = 44.0f; // 下端の余白
		//=============================================================
		// HP演出
		//=============================================================
		int prevHp_ = -1; // 前フレームのHP。これと現在のHPを比較して、HPが減ったか増えたかを判定する。
		// HP減少アニメーションの状態
		float hpTargetRate_ = 1.0f; // H目標値（0.0f～1.0f）
		float hpAnimRate_ = 1.0f; // 現在値（0.0f～1.0f）
		// HP減少アニメーションのフラッシュ演出の状態
		float hpHitFlashT_ = 0.0f; // 経過時間
		float hpHitFlashSec_ = 0.12f; // 継続時間
		// HP減少アニメーションのシェイク演出の状態
		float hpShakeT_ = 0.0f; // 経過時間
		float hpShakeSec_ = 0.15f; // 継続時間
		float hpShakeAmpPx_ = 4.0f; // シェイクの強さ（ピクセル）
		// HP減少アニメーションのイージング演出の状態
		bool hpTweenActive_ = false; // 進行中かどうか
		float hpDrainEaseSec_ = 1.3f; // 演出の継続時間
		Ease::Type hpDrainEaseType_ = Ease::Type::OutElastic; // 演出のイージングの種類
		Ease::Tween hpTween_; // 演出用のイージングオブジェクト
		//=============================================================
		// 入力状態
		//=============================================================
		bool isGamepadConnected_ = false; // ゲームパッドが接続されているかどうか

		//=============================================================
		// Playerから通知されたHUD表示状態
		//=============================================================
		Player::HudState hudState_{}; // Playerから通知されたHUD表示状態の最新値を保存する構造体
		int currentHp_ = 0; // 現在HP
		int maxHp_ = 1; // 最大HP
	};

} // namespace TKM