#pragma once
#include <memory>

#include "IntroSequence.h"
#include "Easing.h"
#include "IrisUtil.h"
#include "Input.h"
#include "Player.h"
#include "Sprite.h"
#include "camera/Camera.h"

class BossManager;

namespace TKM {

	class ClearSequenceController;
	class PostEffectController;
	class UIController;

	//===============================
	// GameFlowControllerクラス
	// ゲーム進行フロー制御クラス
	//===============================
	class GameFlowController {

	public:
		enum class TransitionRequest {
			None, // 何もなし
			ToTitle, // タイトルへ戻る
			ToGameOver, // ゲームオーバーへ
			ToGameClear, // ゲームクリアへ
		};

		/// <summary>
		/// イントロシーケンスを初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// イントロシーケンスの更新処理を行います。
		/// 敵初期化の要求（リクエスト）もここで生成します。
		/// </summary>
		/// <param name="rawDeltaTime">前フレームからの経過時間（秒）</param>
		/// <param name="camera">演出および描画に使用するカメラ</param>
		/// <param name="enemiesInitialized">敵の初期化が完了している場合 true</param>
		/// <param name="outRequestInitEnemies">敵の初期化を要求する場合 true に設定されます</param>
		void Update(float rawDeltaTime, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies);
		/// <summary>
		/// 描画処理を行います。
		/// </summary>
		/// <note>
		/// 実際の描画呼び出しは IntroSequence 側から行われます。
		/// </note>
		void Draw() const;

		/// <summary>
		/// トランジション更新処理を行います。
		/// 死亡／タイトル戻り（Tキー）／アイリス閉じ進行を扱います。
		/// </summary>
		/// <param name="rawDeltaTime">前フレームからの経過時間（秒）</param>
		/// <param name="player">状態参照対象となるプレイヤー</param>
		/// <returns>遷移要求（何もなければ None 等）</returns>
		TransitionRequest UpdateTransitions(float rawDeltaTime, Player* player);
		/// <summary>
		/// アイリスクローズによってタイトルへ戻るリクエストを出します。
		/// </summary>
		void RequestToTitleByIris();
		/// <summary>
		/// クリアシーケンスの更新処理を行います。
		/// </summary>
		/// <param name="rawDeltaTime">前フレームからの経過時間（未スケール、秒）</param>
		/// <param name="scaledDeltaTime">タイムスケール適用後の経過時間（秒）</param>
		/// <param name="postFx">ポストエフェクトコントローラ</param>
		/// <param name="ui">UI コントローラ</param>
		/// <param name="bossManager">ボスマネージャ</param>
		/// <param name="camera">使用中のカメラ</param>
		/// <param name="player">プレイヤー</param>
		/// <returns>クリアシーケンスが完了した場合 true、それ以外は false</returns>
		bool UpdateClear(
			float rawDeltaTime,
			float scaledDeltaTime,
			PostEffectController* postFx,
			UIController* ui,
			BossManager* bossManager,
			Camera* camera,
			Player* player
		);
		/// <summary>
		/// クリアシーケンス開始のリクエストを出します。
		/// </summary>
		void RequestStartClear();
		/// <summary>
		/// クリアシーケンスコントローラをバインドします。
		/// </summary>
		/// <param name="clearSeq">クリアシーケンスコントローラ</param>
		void BindClearSequence(ClearSequenceController* clearSeq);
		/// <summary>
		/// ゲームプレイがロックされているかを取得します。
		/// </summary>
		/// <returns>ゲームプレイがロック中の場合 true、それ以外は false</returns>
		bool IsGameplayLocked() const;
		/// <summary>
		/// アイリス閉じ中かを取得します。
		/// </summary>
		/// <returns>アイリス閉じ中の場合 true、それ以外は false</returns>
		bool IsIrisClosing() const { return irisClosing_; }
		/// <summary>
		/// アイリス描画を外部制御しているかを取得します。
		/// </summary>
		/// <returns>外部制御する場合 true、それ以外は false</returns>
		bool IsExternalIrisDraw() const { return externalIrisDraw_; }
		/// <summary>
		/// アイリスが開いている状態かを取得します。
		/// </summary>
		/// <returns></returns>
		bool IsInClear() const;
		// Setter=====================================
		/// <summary>
		/// アイリス描画を外部から制御するかを設定します。
		/// </summary>
		/// <param name="enable">外部制御する場合 true、それ以外は false</param>
		void SetExternalIrisDraw(bool enable);
		// ===========================================
		// Getter=====================================
		/// <summary>
		/// Irisスプライトの取得。
		/// </summary>
		/// <returns></returns>
		TKM::Sprite* GetIrisSprite() const;
		/// <summary>
		/// Iris最大スケールの取得。
		/// </summary>
		/// <returns></returns>
		float GetIrisMaxScale() const;
		// ===========================================
	private:
		//==============================
		// Intro
		//==============================
		std::unique_ptr<IntroSequence> intro_ = nullptr; // イントロシーケンス
		//==============================
		// Lock
		//==============================
		bool gameplayLocked_ = true; // ゲームプレイロック中フラグ
		//==============================
		// 遷移状態
		//==============================
		TransitionRequest pendingRequest_ = TransitionRequest::None; // 保留中の遷移要求
		//==============================
		// Iris close
		//==============================
		static constexpr float kIrisDurationSec_ = 0.8f; // アイリスクローズ時間（秒）
		bool        irisClosing_ = false; // アイリスクローズ中フラグ
		Ease::Tween irisCloseTween_; // アイリスクローズ用イージング
		bool        irisToTitle_ = false; // タイトルへ戻るためのアイリスクローズか
		//==============================
		// Death → transition
		//==============================
		bool  playerDeathStarted_ = false; // プレイヤー死亡処理開始フラグ
		float playerDeathElapsed_ = 0.0f; // プレイヤー死亡処理経過時間
		//==============================
		// 内部制御
		//==============================
		// 固定dtで閉じ進行（いまの実装に合わせる）
		static constexpr float kFixedDeltaTime_ = 0.016f; // 固定デルタタイム（秒）
		bool externalIrisDraw_ = false; // アイリス描画を外部制御するか
		//==============================
		// 参照先
		//==============================
		ClearSequenceController* clearSeq_ = nullptr; // 参照先クリアシーケンスコントローラ
		//==============================
		// Transition helpers（ネスト削減）
		//==============================
		/// <summary>
		/// 保留中の遷移要求を消費して返します。呼び出すと pendingRequest_ は None にリセットされます。
		/// </summary>
		/// <returns>保留中の遷移要求</returns>
		TransitionRequest ConsumePendingRequest_();
		/// <summary>
		/// プレイヤー死亡からの遷移処理を行います。死亡処理開始からの経過時間に応じて、必要な処理を実行し、遷移要求を生成します。
		/// </summary>
		/// <param name="player">状態参照対象となるプレイヤー</param>
		void HandlePlayerDeathTransition_(Player* player);
		/// <summary>
		/// Tキーによるタイトル戻りの遷移処理を行います。必要に応じてアイリスクローズを開始し、遷移要求を生成します。
		/// </summary>
		void TryStartTitleTransitionByKey_();
		/// <summary>
		/// アイリスクローズの更新処理を行います。閉じ進行が完了した場合、遷移要求を生成します。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		TransitionRequest StepIrisClosing_();
		/// <summary>
		/// アイリスクローズを開始します。toTitle が true の場合はタイトルへ戻るためのアイリスクローズとなります。
		/// </summary>
		/// <param name="toTitle">タイトルへ戻るためのアイリスクローズかどうか</param>
		void BeginIrisClosing_(bool toTitle);
	};
} // namespace TKM