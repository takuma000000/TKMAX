#pragma once
#include <memory>
#include "Sprite.h"
#include "DirectXCommon.h"
#include "Easing.h"
#include "MyMath.h"

namespace TKM {

	//=============================================================
	// IntroStartBannerクラス
	// ・ゲーム開始時の「START」バナーの表示を管理するクラス。
	//=============================================================
	class IntroStartBanner {
	public:
		/// <summary>
		/// イントロ開始バナーを初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// イントロ開始バナーをリセットして再生準備します。
		/// </summary>
		void Reset();
		/// <summary>
		/// イントロ開始バナーの表示を開始します。
		/// </summary>
		void Start();
		/// <summary>
		/// イントロ開始バナーの更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		void Update(float dt);
		/// <summary>
		/// イントロ開始バナーの描画処理を行います。
		/// </summary>
		void Draw() const;

		/// <summary>
		/// イントロ開始バナーの表示中かどうかを取得します。
		/// </summary>
		/// <returns>表示中の場合 true</returns>
		bool IsVisible() const { return visible_; }
		/// <summary>
		/// イントロ開始バナーの表示が終了したかどうかを取得します。
		/// </summary>
		/// <returns>表示が終了した場合 true</returns>
		bool IsFinished() const { return finished_; }

	private:
		std::unique_ptr<Sprite> sprite_;

		bool slideIn_ = false;
		bool visible_ = false;
		bool started_ = false;
		bool fadeOut_ = false;
		bool finished_ = false;

		Vector2 startPos_ = { TKM::WindowsAPI::kClientWidth_ + 400.0f, TKM::WindowsAPI::kClientHeight_ * 0.5f };
		Vector2 endPos_ = { TKM::WindowsAPI::kClientWidth_ * 0.5f,  TKM::WindowsAPI::kClientHeight_ * 0.5f };

		Ease::Tween tween_;
		float duration_ = 1.0f;
		float holdSec_ = 1.0f;
		float holdElapsed_ = 0.0f;
		float fadeSec_ = 0.6f;
		float alpha_ = 1.0f;

		float glowAmp_ = 0.8f;
		float glowSpeed_ = 10.0f;
		bool  glowOn_ = true;
	};

}