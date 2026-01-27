#pragma once
#include <algorithm>
#include <memory>

#include "MyMath.h"
#include "IrisUtil.h"
#include "Easing.h"
#include "Player.h"
#include "manager/BossManager.h"
#include "DirectXCommon.h"
#include "camera/Camera.h"
#include <SkyBox.h>
#include "VignettingEffect.h"
#include "GameFlowController.h"
#include "FireworkController.h"

namespace TKM {
	//=============================================================
	// ClearSequenceController
	// GameSceneのUpdateClearSequenceを丸ごと移植して切り出す版
	//=============================================================
	class ClearSequenceController {
	public:
		enum class Phase { CamZoom, PlayerFly, IrisClose };

		void Start(
			DirectXCommon* dxCommon,
			BossManager* bossManager,
			Player* player,
			Camera* camera,
			VignettingEffect* vignetting,
			GameFlowController* flow,
			FireworkController* firework,
			Skybox* skybox
		);

		// trueで完了（GameClearSceneへ遷移する合図）
		bool Update(float dt);

		bool IsActive() const { return active_; }
		Phase GetPhase() const { return phase_; }

	private:
		static constexpr float kIrisDurationSec_ = 0.8f;

	private:
		bool active_ = false;

		Phase phase_ = Phase::CamZoom;
		float timer_ = 0.0f;

		// 外部参照（所有しない）
		DirectXCommon* dxCommon_ = nullptr;
		BossManager* bossManager_ = nullptr;
		Player* player_ = nullptr;
		Camera* camera_ = nullptr;
		VignettingEffect* vignetting_ = nullptr;
		GameFlowController* flow_ = nullptr;
		FireworkController* firework_ = nullptr;
		Skybox* skybox_ = nullptr;

		// 元のGameSceneと同じ状態
		Vector3 camStartPos_{};
		Vector3 camTargetPos_{};

		Vector3 playerStartPos_{};
		float   playerSpeed_ = 10.0f;
		float   playerFlyMinTime_ = 1.8f;
		float   playerFlyDistance_ = 80.0f;

		bool        irisClosing_ = false;
		Ease::Tween irisCloseTween_;
	};
} // namespace TKM