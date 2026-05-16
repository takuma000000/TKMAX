#pragma once
#include <memory>

#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "Sprite.h"
#include "MyMath.h"
#include "Easing.h"
#include "IrisUtil.h"
#include "StateMachine.h"
#include "IntroStartBanner.h"
#include "IntroBossActor.h"

namespace TKM {

	class PostEffectController;

	//=============================================================
	// IntroSequenceクラス
	// ・ゲーム開始時のイントロ演出を管理するクラス。
	//=============================================================
	class IntroSequence : public TKM::IStateContext {
	public:
		IntroSequence() = default;
		~IntroSequence() = default;

		/// <summary>
		/// イントロ表示関連の初期化を行います。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="object3dCommon">Object3d 共通管理クラス</param>
		void Initialize(DirectXCommon* dxCommon, TKM::Object3dCommon* object3dCommon);
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
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		void Draw(bool irisClosing) const;
		/// <summary>
		/// イントロ用ボスを3D描画します。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		void DrawIntroBoss3D(DirectXCommon* dxCommon) const;

		/// <summary>
		/// はじめのボス演出をスキップ可能な状態かどうかを返します。
		/// </summary>
		/// <returns>スキップ可能なら true</returns>
		bool CanSkipBossIntro() const;
		/// <summary>
		/// はじめのボス演出をスキップして「ゲームスタート」表示に直接移行させます。
		/// </summary>
		/// <param name="camera">カメラのポインタ</param>
		void SkipBossIntroToShowStart(Camera* camera);

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
		bool IsStartVisible() const { return startBanner_.IsVisible(); }
		/// <summary>
		/// ボス登場演出のうち、空が赤くなっているフェーズかを取得します。
		/// </summary>
		/// <returns>空が赤くなっているフェーズの場合 true、それ以外は false</returns>
		bool IsBossSkyRedPhase() const;
		/// <summary>
		/// ボス開幕ムービー中かどうかを取得します。
		/// </summary>
		/// <returns>ボス開幕ムービー中の場合 true</returns>
		bool IsBossIntroPlaying() const;

		// Getter=====================================
		/// <summary>
		/// Irisスプライトの取得。
		/// </summary>
		/// <returns></returns>
		Sprite* GetIrisSprite() { return iris_.get(); }
		/// <summary>
		/// Irisスプライトの取得。
		/// </summary>
		/// <returns></returns>
		const Sprite* GetIrisSprite() const { return iris_.get(); }
		/// <summary>
		/// Iris最大スケールの取得。
		/// </summary>
		/// <returns></returns>
		float   GetIrisMaxScale() const { return irisMaxScale_; }
		/// <summary>
		/// StateMachineの取得。
		/// </summary>
		/// <returns></returns>
		StateMachine& GetStateMachine() { return flowSM_; }
		/// <summary>
		/// StateMachineの取得（const版）。
		/// </summary>
		/// <returns></returns>
		const StateMachine& GetStateMachine() const { return flowSM_; }
		// ===========================================
		// Setter=====================================
		/// <summary>
		/// PostEffectControllerのポインタをセットします。
		/// </summary>
		/// <param name="postEffect">PostEffectControllerのポインタ</param>
		void SetPostEffectController(TKM::PostEffectController* postEffect) {
			postEffect_ = postEffect;
		}
		// ===========================================

	private:
		//======================================================================
		// イントロ進行フェーズ
		//======================================================================
		enum class Phase {
			IrisOpen, // アイリス開き
			CameraIntro, // カメラインロ（回転）
			BossPreSpawn, // ボス出現前の待機
			BossAppear, // ボス登場
			BossPause, // 到達位置で止まる
			BossNoticeHop, // ボスが気づいて跳ねる
			BossPanic, // ボスが慌てる
			BossEscape, // ボスが逃げる
			ShowStart, // 「ゲームスタート」表示
			Done, // イントロ完了
		};
		Phase phase_ = Phase::IrisOpen; // 現在のイントロ進行フェーズ

		StateMachine flowSM_;

		Camera* currentCamera_ = nullptr;
		bool currentEnemiesInitialized_ = false;
		bool* currentOutRequestInitEnemies_ = nullptr;
		// ======================================================================
		// --- 演出全体に関するフラグや定数 ---
		// ======================================================================
		// --- 共通（固定dtで進めたい演出用） ---
		static constexpr float kFixedDt_ = 0.016f;
		// --- ゲーム開始ロック ---
		bool gameplayLocked_ = true;
		//======================================================================
		// Iris開き演出
		//======================================================================
		std::unique_ptr<Sprite> iris_ = nullptr; // アイリススプライト
		bool   irisOpening_ = true;              // アイリス開き中フラグ
		float  irisMaxScale_ = 0.0f;             // 最大スケール
		Ease::Tween irisTween_;                  // イージング

		// Iris開き中エフェクト
		bool  emitOpenBurst_ = true;             // 開き中の爆発エフェクト発生フラグ（1回）
		float emitOpenDelaySec_ = 0.7f;          // 爆発発生タイミング
		float emitOpenElapsed_ = 0.0f;           // 経過時間

		bool  emitFireworkPending_ = false;      // 花火発射待機フラグ
		float emitFireworkDelaySec_ = 0.7f;      // 花火発射タイミング
		Vector3 lastEmitPos_{ 0.0f,0.0f,0.0f };  // 花火発射位置

		// Irisトランジション時間
		static constexpr float kIrisDurationSec_ = 0.8f;

		//======================================================================
		// カメラインロ（回転）
		//======================================================================
		Ease::Tween camYawTween_;    // ヨー回転イージング
		float camIntroDuration_ = 1.2f; // 演出時間

		float camYawStart_ = -1.2f;  // 開始ヨー
		float camYawEnd_ = 0.0f;     // 終了ヨー
		float camPitchStart_ = 0.12f;// 開始ピッチ
		float camPitchEnd_ = 0.05f;  // 終了ピッチ

		//======================================================================
		// 「ゲームスタート」表示
		//======================================================================
		IntroStartBanner startBanner_; // スタート表示管理

		//======================================================================
		// イントロ用ボス
		//======================================================================
		IntroBossActor introBossActor_;

		//======================================================================
		// ボス演出カメラブレンド
		//======================================================================
		bool camBlendToBossActive_ = false; // 通常→ボスカメラ補間中
		bool camBlendBackActive_ = false;   // ボス→通常カメラ補間中

		Vector3 camSavedRot_{ 0.0f, 0.0f, 0.0f };       // 元の回転
		Vector3 camBossStartRot_{ 0.0f, 0.0f, 0.0f };   // 補間開始回転
		Vector3 camBossTargetRot_{ 0.0f, 0.0f, 0.0f };  // 目標回転
		Vector3 camReturnStartRot_{ 0.0f, 0.0f, 0.0f }; // 戻り開始回転

		Ease::Tween camBlendToBossTween_; // 通常→ボス
		Ease::Tween camBlendBackTween_;   // ボス→通常

		float camBlendToBossSec_ = 0.45f; // 入り時間
		float camBlendBackSec_ = 0.55f;   // 戻り時間

		//======================================================================
		// スキップ制御
		//======================================================================
		float skipHoldTimer_ = 0.0f;              // 長押し時間
		static constexpr float kSkipHoldSec_ = 2.0f; // スキップ判定時間

		//======================================================================
		// イントロ全体で使用するエフェクトコントローラー
		//======================================================================
		TKM::PostEffectController* postEffect_ = nullptr;

		//======================================================================
		// StateMachine
		//======================================================================
		friend class IntroIrisOpenState;
		friend class IntroCameraIntroState;
		friend class IntroBossPreSpawnState;
		friend class IntroBossAppearState;
		friend class IntroBossPauseState;
		friend class IntroBossNoticeHopState;
		friend class IntroBossPanicState;
		friend class IntroBossEscapeState;
		friend class IntroShowStartState;
		friend class IntroDoneState;
};
} // namespace TKM