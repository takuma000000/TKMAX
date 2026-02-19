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
		// ===========================================

	private:
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
		float  irisScale_ = 0.0f; // 開始スケールは小さめにしておく
		Ease::Tween irisTween_; // アイリス開きのイージング

		// Iris開きと同時に出すエフェクト
		bool  emitOpenBurst_ = true; // アイリス開きと同時に出す爆発エフェクト（最初はtrueで、アイリス開きの途中で一度だけ出す）
		float emitOpenDelaySec_ = 0.7f; // アイリス開きの途中で出す（0.0fだと同時、0.8fだとアイリスが完全に開いてから）
		float emitOpenElapsed_ = 0.0f; // アイリス開き開始からの経過時間（秒）

		bool  emitFireworkPending_ = false; // アイリス開きと同時に出す花火エフェクトの発射が保留されているか（最初はfalseで、アイリス開きの途中で一度だけtrueになる）
		float emitFireworkDelaySec_ = 0.7f; // アイリス開きの途中で出す花火エフェクトの発射（0.0fだと同時、0.8fだとアイリスが完全に開いてから）
		float emitFireworkElapsed_ = 0.0f; // 念のため
		Vector3 lastEmitPos_{ 0.0f,0.0f,0.0f }; // アイリス開きと同時に出す花火エフェクトの発射位置（ワールド座標）。アイリスの中心に近い位置をランダムに選ぶ。
		// アイリスのトランジション時間
		static constexpr float kIrisDurationSec_ = 0.8f; // アイリスの最大スケール（画面全体を覆うサイズ）に対する、開始スケールの割合
		//======================================================================
		// カメラインロ（回転）
		//======================================================================
		bool  camIntroActive_ = false; // カメラインロ演出がアクティブか
		bool  camIntroDone_ = false; // カメラインロ演出が完了したか
		Ease::Tween camYawTween_; // カメラインロのヨーイング（左右回転）のイージング
		float camIntroDuration_ = 1.2f; // カメラインロの全体の時間（秒）
		// カメラインロの開始・終了時の角度（ラジアン）。開始は少し左を向いていて、終了は正面を向く。
		float camYawStart_ = -1.2f; // ラジアンで、左を向いている状態（-1.2は約-68.75度）。この値を大きくすると開始時により左を向いていることになる。
		float camYawEnd_ = 0.0f; // ラジアンで、正面を向いている状態。通常は0.0fで問題ないはず。
		float camPitchStart_ = 0.12f; // ラジアンで、カメラが少し上を向いている状態。これを大きくすると開始時により上を向いていることになる。
		float camPitchEnd_ = 0.05f; // ラジアンで、カメラが少し上を向いている状態。これを大きくすると終了時により上を向いていることになる。
		//======================================================================
		// 「ゲームスタート」スライドイン演出
		//======================================================================
		std::unique_ptr<Sprite> startSprite_; // 「ゲームスタート」表示用スプライト
		float startT_ = 0.0f; // 演出の進行度合い（0.0f～1.0f）。スライドインとフェードアウト両方で使用する。
		bool  startSlideIn_ = false; // 「ゲームスタート」スライドイン演出中か
		bool  startVisible_ = false; // 「ゲームスタート」表示が可視状態か
		bool  startPlayed_ = false; // 「ゲームスタート」表示の演出が一度でも開始されたか（スライドイン開始のトリガー用）
		// 「ゲームスタート」スライドインの開始・終了位置。開始位置は画面右外、終了位置は画面中央。
		Vector2 startStartPos_ = { TKM::WindowsAPI::kClientWidth_ + 400.0f, TKM::WindowsAPI::kClientHeight_ * 0.5f }; // 画面右外（右端からさらに400ピクセル右）。この値を大きくすると開始位置がさらに右になる。
		Vector2 startEndPos_ = { TKM::WindowsAPI::kClientWidth_ * 0.5f,  TKM::WindowsAPI::kClientHeight_ * 0.5f }; // 画面中央
		// 「ゲームスタート」スライドインのイージング
		Ease::Tween startTween_; // 「ゲームスタート」スライドインのイージング
		float       startDuration_ = 1.0f; // 「ゲームスタート」スライドインの全体の時間（秒）
		float       startHoldSec_ = 1.0f; // 「ゲームスタート」表示が中央に留まる時間（秒）
		float       startHoldElapsed_ = 0.0f; // 「ゲームスタート」表示が中央に留まっている時間の経過（秒）
		bool        startFadeOut_ = false; // 「ゲームスタート」表示のフェードアウト中か
		float       startFadeSec_ = 0.6f; // 「ゲームスタート」表示のフェードアウトにかける時間（秒）
		float       startAlpha_ = 1.0f; // 「ゲームスタート」表示のアルファ値（0.0f～1.0f）。フェードアウトで使用。
		// 「ゲームスタート」表示のフェードアウトのイージング
		float startGlowAmp_ = 0.8f; // 「ゲームスタート」表示のグローの強さ。0.0fでグローなし、1.0fで最大のグロー。スライドインとフェードアウト両方で使用。
		float startGlowSpeed_ = 10.0f; // 「ゲームスタート」表示のグローの速さ。値が大きいほど速くグローが変化する。スライドインとフェードアウト両方で使用。
		bool  startGlowOn_ = true; // 「ゲームスタート」表示のグローがオンか。スライドインとフェードアウト両方で使用。trueのとき、startGlowAmp_の値に応じてグローが変化する。falseのとき、グローなし。
	};
} // namespace TKM