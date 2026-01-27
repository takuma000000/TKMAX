#pragma once
#include <algorithm>

#include "MyMath.h"
#include "Easing.h"
#include "IrisUtil.h"

#include "camera/Camera.h"
#include "SkyBox.h"
#include "Player.h"
#include "manager/BossManager.h"
#include "GameFlowController.h"
#include "DirectXCommon.h"

#include "FireworkController.h"

namespace TKM {
	class ClearSequenceController {
	public:
		enum class Phase {
			None, CamZoom, PlayerFly, IrisClose
		};

		/// <summary>
		/// 初期化。
		/// </summary>
		/// <param name="camera"></param>
		/// <param name="player"></param>
		/// <param name="bossManager"></param>
		/// <param name="flow"></param>
		/// <param name="dxCommon"></param>
		/// <param name="skybox"></param>
		/// <param name="fireworkController"></param>
		void Initialize(Camera* camera, Player* player, BossManager* bossManager, GameFlowController* flow, DirectXCommon* dxCommon, Skybox* skybox, FireworkController* fireworkController);
		/// <summary>
		/// 開始。
		/// </summary>
		void Start();
		/// <summary>
		/// 更新。
		/// </summary>
		/// <param name="dt"></param>
		/// <returns></returns>
		bool Update(float dt);
		/// <summary>
		/// アクティブか？
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }

		// Setter===================================
		/// <summary>
		/// プレイヤーの飛行速度設定。
		/// </summary>
		/// <param name="v"></param>
		void SetPlayerSpeed(float v) { playerSpeed_ = v; }
		/// <summary>
		/// プレイヤーの飛行最短時間設定。
		/// </summary>
		/// <param name="v"></param>
		void SetPlayerFlyMinTime(float v) { playerFlyMinTime_ = v; }
		/// <summary>
		/// プレイヤーの飛行距離設定。
		/// </summary>
		/// <param name="v"></param>
		void SetPlayerFlyDistance(float v) { playerFlyDistance_ = v; }
		// =========================================

	private:
		void UpdateCamZoom(float dt, bool& finished);
		void UpdatePlayerFly(float dt, bool& finished);
		void UpdateIrisClose(float dt, bool& finished);

		bool active_ = false;
		Phase phase_ = Phase::None;
		float timer_ = 0.0f;

		// 参照先
		Camera* camera_ = nullptr;
		Player* player_ = nullptr;
		BossManager* bossManager_ = nullptr;
		GameFlowController* flow_ = nullptr;
		DirectXCommon* dxCommon_ = nullptr;
		Skybox* skybox_ = nullptr;
		FireworkController* fireworkController_ = nullptr;

		// カメラ・プレイヤー
		Vector3 camStartPos_{};
		Vector3 camTargetPos_{};
		Vector3 playerStartPos_{};

		float playerSpeed_ = 10.0f;
		float playerFlyMinTime_ = 1.8f;
		float playerFlyDistance_ = 80.0f;

		// Iris
		static constexpr float kIrisDurationSec_ = 0.8f;
		bool irisClosing_ = false;
		Ease::Tween irisCloseTween_;
	};
}