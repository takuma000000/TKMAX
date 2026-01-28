#pragma once
#include <memory>

#include "DirectXCommon.h"
#include "camera/Camera.h"
#include "Sprite.h"
#include "MyMath.h"
#include "Easing.h"
#include "IrisUtil.h"

namespace TKM {
	//=============================================================
	// IntroSequenceクラス
	// ゲーム開始時の演出（アイリス開き / カメラ回転 / start.png表示）を管理する。
	//=============================================================
	class IntroSequence {
	public:
		IntroSequence() = default;
		~IntroSequence() = default;

		/// <summary>
		/// イントロ表示関連の初期化を行います。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// イントロ表示の更新処理を行います。
		/// 敵初期化の要求生成もここで行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="camera">演出および描画に使用するカメラ</param>
		/// <param name="enemiesInitialized">敵の初期化が完了している場合 true</param>
		/// <param name="outRequestInitEnemies">敵初期化を要求する場合 true に設定されます</param>
		void Update(float dt, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies);
		/// <summary>
		/// イントロ表示の描画処理を行います。
		/// </summary>
		/// <param name="irisClosing">アイリス閉じ中の場合 true</param>
		void Draw(bool irisClosing) const;

		/// <summary>
		/// ゲームプレイがロックされているかを取得します。
		/// </summary>
		/// <returns>ゲームプレイがロック中の場合 true、それ以外は false</returns>
		bool IsGameplayLocked() const { return gameplayLocked_; }
		/// <summary>
		/// アイリスが開いている状態かを取得します。
		/// </summary>
		/// <returns>アイリス開き中の場合 true、それ以外は false</returns>
		bool IsIrisOpening()   const { return irisOpening_; }
		/// <summary>
		/// 「ゲームスタート」表示が可視状態かを取得します。
		/// </summary>
		/// <returns>表示中の場合 true、それ以外は false</returns>
		bool IsStartVisible()  const { return startVisible_; }

		// Getter=====================================
		/// <summary>
		/// Irisスプライトの取得。
		/// </summary>
		/// <returns></returns>
		Sprite* GetIrisSprite() const { return iris_.get(); }
		/// <summary>
		/// Iris最大スケールの取得。
		/// </summary>
		/// <returns></returns>
		float   GetIrisMaxScale() const { return irisMaxScale_; }
		// ===========================================

	private:
		// --- 共通（固定dtで進めたい演出用） ---
		static constexpr float kFixedDt_ = 0.016f;

		// --- ゲーム開始ロック ---
		bool gameplayLocked_ = true;

		//======================================================================
		// Iris開き演出
		//======================================================================
		std::unique_ptr<Sprite> iris_ = nullptr;
		bool   irisOpening_ = true;
		float  irisMaxScale_ = 0.0f;
		float  irisScale_ = 0.0f;
		Ease::Tween irisTween_;

		// Iris開きと同時に出すエフェクト
		bool  emitOpenBurst_ = true;
		float emitOpenDelaySec_ = 0.7f;
		float emitOpenElapsed_ = 0.0f;

		bool  emitFireworkPending_ = false;
		float emitFireworkDelaySec_ = 0.7f;
		float emitFireworkElapsed_ = 0.0f; // 念のため
		Vector3 lastEmitPos_{ 0.0f,0.0f,0.0f };

		static constexpr float kIrisDurationSec_ = 0.8f;

		//======================================================================
		// カメラインロ（回転）
		//======================================================================
		bool  camIntroActive_ = false;
		bool  camIntroDone_ = false;
		Ease::Tween camYawTween_;
		float camIntroDuration_ = 1.2f;

		float camYawStart_ = -1.2f;
		float camYawEnd_ = 0.0f;
		float camPitchStart_ = 0.12f;
		float camPitchEnd_ = 0.05f;

		//======================================================================
		// 「ゲームスタート」スライドイン演出
		//======================================================================
		std::unique_ptr<Sprite> startSprite_;
		float startT_ = 0.0f;
		bool  startSlideIn_ = false;
		bool  startVisible_ = false;
		bool  startPlayed_ = false;

		Vector2 startStartPos_ = { TKM::WindowsAPI::kClientWidth_ + 400.0f, TKM::WindowsAPI::kClientHeight_ * 0.5f };
		Vector2 startEndPos_ = { TKM::WindowsAPI::kClientWidth_ * 0.5f,  TKM::WindowsAPI::kClientHeight_ * 0.5f };

		Ease::Tween startTween_;
		float       startDuration_ = 1.0f;
		float       startHoldSec_ = 1.0f;
		float       startHoldElapsed_ = 0.0f;
		bool        startFadeOut_ = false;
		float       startFadeSec_ = 0.6f;
		float       startAlpha_ = 1.0f;

		float startGlowAmp_ = 0.8f;
		float startGlowSpeed_ = 10.0f;
		bool  startGlowOn_ = true;
	};
} // namespace TKM