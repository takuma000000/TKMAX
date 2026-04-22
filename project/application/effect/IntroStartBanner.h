#pragma once
#include <memory>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "Easing.h"
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// IntroStartBannerクラス
	// ゲーム開始バナーを管理するクラス
	//=============================================================
	class IntroStartBanner {
	public:
		//=============================================================
		// 初期化・制御
		//=============================================================

		/// <summary>
		/// 開始バナーを初期化します。
		/// </summary>
		void Initialize(DirectXCommon* dxCommon);

		/// <summary>
		/// バナー状態をリセットします。
		/// </summary>
		void Reset();

		/// <summary>
		/// バナー表示を開始します。
		/// </summary>
		void Start();

		//=============================================================
		// 更新・描画
		//=============================================================

		/// <summary>
		/// バナーを更新します。
		/// </summary>
		void Update(float dt);

		/// <summary>
		/// バナーを描画します。
		/// </summary>
		void Draw() const;

		//=============================================================
		// 状態取得
		//=============================================================

		/// <summary>
		/// 表示中かを返します。
		/// </summary>
		bool IsVisible() const { return visible_; }

		/// <summary>
		/// 表示終了済みかを返します。
		/// </summary>
		bool IsFinished() const { return finished_; }

	private:
		//=============================================================
		// スプライト
		//=============================================================

		std::unique_ptr<Sprite> sprite_ = nullptr; // バナースプライト

		//=============================================================
		// 状態フラグ
		//=============================================================

		bool slideIn_ = false;  // スライドイン中
		bool visible_ = false;  // 表示中
		bool started_ = false;  // 開始済み
		bool fadeOut_ = false;  // フェードアウト中
		bool finished_ = false; // 終了済み

		//=============================================================
		// 位置
		//=============================================================

		Vector2 startPos_ = { TKM::WindowsAPI::GetClientWidth() + 400.0f, TKM::WindowsAPI::GetClientHeight() * 0.5f }; // 開始位置
		Vector2 endPos_ = { TKM::WindowsAPI::GetClientWidth() * 0.5f,  TKM::WindowsAPI::GetClientHeight() * 0.5f };    // 終了位置

		//=============================================================
		// 時間制御
		//=============================================================

		Ease::Tween tween_;       // 位置補間
		float duration_ = 1.0f;   // スライド時間
		float holdSec_ = 1.0f;    // 停止時間
		float holdElapsed_ = 0.0f;// 停止経過
		float fadeSec_ = 0.6f;    // フェード時間
		float alpha_ = 1.0f;      // 透明度

		//=============================================================
		// 発光演出
		//=============================================================

		float glowAmp_ = 0.8f;    // 発光量
		float glowSpeed_ = 10.0f; // 発光速度
		bool  glowOn_ = true;     // 発光ON/OFF
	};

}