#pragma once
#include <memory>
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Input.h"

namespace TKM {

	//=============================================================
	// SkipGuideUIクラス
	// スキップ案内UIを管理するクラス
	//=============================================================
	class SkipGuideUI {
	public:
		//=============================================================
		// 初期化
		//=============================================================

		/// <summary>
		/// スキップ案内UIを初期化します。
		/// </summary>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			float screenW,
			float screenH
		);

		//=============================================================
		// 更新・描画
		//=============================================================

		/// <summary>
		/// スキップ案内UIを更新します。
		/// </summary>
		void Update(float dt, bool canSkip);

		/// <summary>
		/// スキップ案内UIを描画します。
		/// </summary>
		void Draw(float alpha);

		/// <summary>
		/// ImGui調整項目を表示します。
		/// </summary>
		void DrawImGui();

		/// <summary>
		/// スキップゲージが最大まで溜まったかを取得します。
		/// </summary>
		bool IsSkipCompleted() const { return skipCompleted_; }

	private:
		//=============================================================
		// スプライト
		//=============================================================

		std::unique_ptr<Sprite> sprite_ = nullptr; // UIスプライト
		std::unique_ptr<Sprite> gaugeSprite_ = nullptr; // スキップゲージ用スプライト

		//=============================================================
		// skip.png配置
		//=============================================================

		Vector2 basePos_{};                    // 基準位置
		Vector2 baseSize_{};                   // 基準サイズ
		Vector2 offset_{ -137.0f, -49.0f };    // 右下基準オフセット

		//=============================================================
		// 入力保持
		//=============================================================

		float holdTimer_ = 0.0f;               // 長押し時間
		static constexpr float kHoldTime_ = 2.0f; // 必要長押し時間
		bool skipCompleted_ = false; // スキップ成立フラグ

		//=============================================================
		// 見た目
		//=============================================================

		float normalScale_ = 0.30f;            // 通常時スケール

		//=============================================================
		// 画面サイズ
		//=============================================================

		float screenW_ = 0.0f;                 // 画面幅
		float screenH_ = 0.0f;                 // 画面高さ

		//=============================================================
		// ゲージ
		//=============================================================
		// ゲージの配置は、UIスプライトからの相対位置で指定する
		Vector2 gaugeOffset_{ -130.0f, -0.8f }; // UIスプライトの左端から少し右にずらす
		Vector2 gaugeMaxSize_{ 260.0f, 45.0f }; // ゲージの最大サイズ
		// 少し透明にする
		Vector4 gaugeColor_{ 1.0f, 1.0f, 1.0f, 1.0f }; // ゲージの色
		// ゲージ発光
		Vector4 gaugeGlowColor_{ 0.6f, 1.0f, 0.0f, 1.0f }; // 黄緑寄りネオン
		float gaugeGlowIntensity_ = 8.0f;   // 発光強さ
		float gaugeGlowWidth_ = 12.0f;      // 発光幅
		float gaugeGlowThreshold_ = 0.01f;  // かなり光らせる
		float gaugeGlowSoftness_ = 6.0f;    // にじみ強め
	};
}