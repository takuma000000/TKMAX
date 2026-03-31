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
		/// はじめのボス演出をスキップして「ゲームスタート」表示へ進めます。
		/// </summary>
		void SkipBossIntroToShowStart();

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
		std::unique_ptr<Sprite> iris_ = nullptr; // アイリス用スプライト
		bool   irisOpening_ = true; // trueで「開く」演出中
		float  irisMaxScale_ = 0.0f; // アイリスが最大に広がるスケール（画面全体を覆うサイズ）
		Ease::Tween irisTween_; // アイリス開きのイージング

		// Iris開きと同時に出すエフェクト
		bool  emitOpenBurst_ = true; // アイリス開きと同時に出す爆発エフェクト（最初はtrueで、アイリス開きの途中で一度だけ出す）
		float emitOpenDelaySec_ = 0.7f; // アイリス開きの途中で出す（0.0fだと同時、0.8fだとアイリスが完全に開いてから）
		float emitOpenElapsed_ = 0.0f; // アイリス開き開始からの経過時間（秒）

		bool  emitFireworkPending_ = false; // アイリス開きと同時に出す花火エフェクトの発射が保留されているか（最初はfalseで、アイリス開きの途中で一度だけtrueになる）
		float emitFireworkDelaySec_ = 0.7f; // アイリス開きの途中で出す花火エフェクトの発射（0.0fだと同時、0.8fだとアイリスが完全に開いてから）
		Vector3 lastEmitPos_{ 0.0f,0.0f,0.0f }; // アイリス開きと同時に出す花火エフェクトの発射位置（ワールド座標）。アイリスの中心に近い位置をランダムに選ぶ。
		// アイリスのトランジション時間
		static constexpr float kIrisDurationSec_ = 0.8f; // アイリスの最大スケール（画面全体を覆うサイズ）に対する、開始スケールの割合
		//======================================================================
		// カメラインロ（回転）
		//======================================================================
		Ease::Tween camYawTween_; // カメラインロのヨーイング（左右回転）のイージング
		float camIntroDuration_ = 1.2f; // カメラインロの全体の時間（秒）
		// カメラインロの開始・終了時の角度（ラジアン）。開始は少し左を向いていて、終了は正面を向く。
		float camYawStart_ = -1.2f; // ラジアンで、左を向いている状態（-1.2は約-68.75度）。この値を大きくすると開始時により左を向いていることになる。
		float camYawEnd_ = 0.0f; // ラジアンで、正面を向いている状態。通常は0.0fで問題ないはず。
		float camPitchStart_ = 0.12f; // ラジアンで、カメラが少し上を向いている状態。これを大きくすると開始時により上を向いていることになる。
		float camPitchEnd_ = 0.05f; // ラジアンで、カメラが少し上を向いている状態。これを大きくすると終了時により上を向いていることになる。
		//======================================================================
		// 「ゲームスタート」表示
		//======================================================================
		IntroStartBanner startBanner_; // 「ゲームスタート」表示の管理クラス
		//======================================================================
		// イントロ用ボス
		//======================================================================
		IntroBossActor introBossActor_;
		//======================================================================
		// ボス演出用カメラブレンド
		//======================================================================
		bool camBlendToBossActive_ = false;     // 通常→ボス演出カメラへ補間中
		bool camBlendBackActive_ = false;       // ボス演出→通常カメラへ補間中
		// カメラの位置は変えず、回転のみを補間する。以下はそのための変数。
		Vector3 camSavedRot_{ 0.0f, 0.0f, 0.0f };      // ボス演出開始前の回転を保存
		Vector3 camBossStartRot_{ 0.0f, 0.0f, 0.0f };  // ボス演出ブレンド開始回転
		Vector3 camBossTargetRot_{ 0.0f, 0.0f, 0.0f }; // ボス演出時の目標回転
		Vector3 camReturnStartRot_{ 0.0f, 0.0f, 0.0f };// 戻り補間開始回転
		// カメラブレンドのイージング
		Ease::Tween camBlendToBossTween_; // 通常→ボス演出
		Ease::Tween camBlendBackTween_;   // ボス演出→通常
		// カメラブレンドの時間
		float camBlendToBossSec_ = 0.45f; // 入り補間時間
		float camBlendBackSec_ = 0.55f;   // 戻り補間時間
		//======================================================================
		// ボス演出のうち、スキップ可能なフェーズに入っているかどうかを管理する変数
		// ======================================================================
		float skipHoldTimer_ = 0.0f; // スキップ用の長押し時間
		static constexpr float kSkipHoldSec_ = 2.0f; // 何秒でスキップするか
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