#pragma once
#include <memory>
#include "Sprite.h"
#include "SpriteCommon.h"
#include "Input.h"

namespace TKM {
	//==============================================================================
	// SkipGuideUIクラス
	// 開始時のボスのスタートムービー演出をスキップするためのUI
	//==============================================================================
	class SkipGuideUI {
	public:
		/// <summary>
		/// SkipGuideUIを初期化します。
		/// </summary>
		/// <param name="spriteCommon">スプライト共通管理クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="screenW">画面幅</param>
		/// <param name="screenH">画面高さ</param>
		void Initialize(
			SpriteCommon* spriteCommon,
			DirectXCommon* dxCommon,
			float screenW,
			float screenH
		);
		/// <summary>
		/// SkipGuideUIを更新します。
		/// </summary>
		/// <param name="dt">デルタタイム</param>
		/// <param name="canSkip">スキップ可能かどうか</param>
		void Update(float dt, bool canSkip);
		/// <summary>
		/// SkipGuideUIを描画します。
		/// </summary>
		/// <param name="alpha">アルファ値（0.0f〜1.0f）</param>
		void Draw(float alpha);
		/// <summary>
		/// SkipGuideUI の ImGui 調整を表示します。
		/// </summary>
		void DrawImGui();

	private:
		std::unique_ptr<Sprite> sprite_;

		Vector2 basePos_{};
		Vector2 baseSize_{};
		Vector2 offset_{ -137.0f, -49.0f }; // 右下基準の位置オフセット

		float holdTimer_ = 0.0f;
		static constexpr float kHoldTime_ = 2.0f; // 押し続ける必要のある時間（秒）

		float normalScale_ = 0.90f;
		float pressScale_ = 1.10f;

		Vector4 normalColor_{ 1.0f, 1.0f, 1.0f, 0.60f };
		Vector4 pressColor_{ 1.0f, 1.0f, 0.0f, 1.00f }; // 押しているときの色

		float screenW_ = 0.0f;
		float screenH_ = 0.0f;
	};
}