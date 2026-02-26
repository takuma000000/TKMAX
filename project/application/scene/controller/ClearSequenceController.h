#pragma once
#include <algorithm>

#include "MyMath.h"
#include "Easing.h"
#include "IrisUtil.h"

#include "CameraManager.h"
#include "SkyBox.h"
#include "Player.h"
#include "manager/BossManager.h"
#include "GameFlowController.h"
#include "DirectXCommon.h"
#include "FireworkController.h"

namespace TKM {

	//===============================================================================
	// クリーンシーケンスコントローラー
	// クリア時の演出（カメラズーム、プレイヤー飛行、アイリスクローズ）を管理するクラス。
	//===============================================================================
	class ClearSequenceController {
	public:
		enum class Phase {
			None, CamZoom, PlayerFly, IrisClose
		};

		/// <summary>
		/// ゲーム進行関連システムを初期化します。
		/// </summary>
		/// <param name="player">制御対象となるプレイヤー</param>
		/// <param name="bossManager">ボス管理クラス</param>
		/// <param name="flow">ゲーム進行フロー制御クラス</param>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="skybox">背景表示用スカイボックス</param>
		/// <param name="fireworkController">花火演出制御クラス</param>
		void Initialize(
			Player* player,
			BossManager* bossManager,
			GameFlowController* flow,
			DirectXCommon* dxCommon,
			Skybox* skybox,
			FireworkController* fireworkController
		);
		/// <summary>
		/// 処理を開始します。
		/// </summary>
		void Start();
		/// <summary>
		/// 毎フレームの更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <returns>処理が完了した場合 true、それ以外は false</returns>
		bool Update(float dt);
		/// <summary>
		/// アクティブか？
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		// Setter===================================
		/// <summary>
		/// プレイヤーの飛行速度を設定します。
		/// </summary>
		/// <param name="v">プレイヤーの飛行速度</param>
		void SetPlayerSpeed(float v);
		/// <summary>
		/// プレイヤーの飛行距離を設定します。
		/// </summary>
		/// <param name="v">飛行距離</param>
		void SetPlayerFlyDistance(float v);
		// =========================================

	private:
		/// <summary>
		/// カメラズーム演出の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="finished">演出が完了した場合 true に設定されます</param>
		void UpdateCamZoom(float dt, bool& finished);
		/// <summary>
		/// プレイヤー飛行演出の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="finished">演出が完了した場合 true に設定されます</param>
		void UpdatePlayerFly(float dt, bool& finished);
		/// <summary>
		/// アイリスクローズ演出の更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="finished">演出が完了した場合 true に設定されます</param>
		void UpdateIrisClose(float dt, bool& finished);

		//==============================
		// 状態
		//==============================
		bool active_ = false; // アクティブフラグ
		Phase phase_ = Phase::None; // 現在フェーズ
		float timer_ = 0.0f; // フェーズ経過時間
		//==============================
		// 参照先
		//==============================
		Player* player_ = nullptr;
		BossManager* bossManager_ = nullptr;
		GameFlowController* flow_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		Skybox* skybox_ = nullptr;
		FireworkController* fireworkController_ = nullptr;
		//==============================
		// カメラ・プレイヤー
		//==============================
		Vector3 camStartPos_{}; // カメラ開始位置
		Vector3 camTargetPos_{}; // カメラ目標位置
		Vector3 playerStartPos_{}; // プレイヤー開始位置
		float playerSpeed_ = 10.0f; // プレイヤー飛行速度
		float playerFlyMinTime_ = 1.8f; // プレイヤー飛行最短時間
		float playerFlyDistance_ = 80.0f; // プレイヤー飛行距離
		//==============================
		// Iris
		//==============================
		static constexpr float kIrisDurationSec_ = 0.8f; // アイリスクローズ演出時間
		bool irisClosing_ = false; // アイリスクローズ中フラグ
		Ease::Tween irisCloseTween_; // アイリスクローズ用イージング
	};
}