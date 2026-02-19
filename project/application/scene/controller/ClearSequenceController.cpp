#include "ClearSequenceController.h"

namespace TKM {
	void ClearSequenceController::Initialize(Camera* camera, Player* player, BossManager* bossManager, GameFlowController* flow, DirectXCommon* dxCommon, Skybox* skybox, FireworkController* fireworkController) {
		camera_ = camera;
		player_ = player;
		bossManager_ = bossManager;
		flow_ = flow;
		dxCommon_ = dxCommon;
		skybox_ = skybox;
		fireworkController_ = fireworkController;

		active_ = false;
		phase_ = Phase::None;
		timer_ = 0.0f;
		irisClosing_ = false;
	}

	void ClearSequenceController::Start() {
		active_ = true;
		phase_ = Phase::CamZoom;
		timer_ = 0.0f;

		//  外部Iris描画は一旦OFF
		if (flow_) {
			flow_->SetExternalIrisDraw(false);
		}

		// --- ボス、ボス弾、レティクルを消し、プレイヤー操作をロック ---
		if (bossManager_) {
			bossManager_->OnClearSequenceStart();
		}
		if (player_) {
			player_->SetControlEnabled(false);
			player_->SetReticleVisible(false);
		}

		if (dxCommon_) {
			dxCommon_->SetVignettingEffect(nullptr);
		}

		// カメラ開始位置
		if (camera_) {
			camStartPos_ = camera_->GetTranslate();
		}

		// カメラ目標位置（プレイヤーへ寄せる）
		if (camera_ && player_) {
			Vector3 camPos = camera_->GetTranslate();
			Vector3 playerPos = player_->GetPosition();

			float targetZ = MyMath::Lerp(camPos.z, playerPos.z - 15.0f, 1.0f);
			camTargetPos_ = { camPos.x, camPos.y + 2.0f, targetZ };

			playerStartPos_ = player_->GetPosition();
		}

		irisClosing_ = false;
	}

	bool ClearSequenceController::Update(float dt) {
		if (!active_) {
			return false;
		}

		timer_ += dt;

		// skyboxはずっと回し続ける
		if (skybox_) {
			skybox_->UpdateRotation();
		}

		bool finished = false;

		switch (phase_) {
		case Phase::CamZoom:
			UpdateCamZoom(dt, finished);
			break;
		case Phase::PlayerFly:
			UpdatePlayerFly(dt, finished);
			break;
		case Phase::IrisClose:
			UpdateIrisClose(dt, finished);
			break;
		default:
			break;
		}

		if (finished) {
			active_ = false;
			phase_ = Phase::None;
		}

		return finished;
	}

	void ClearSequenceController::SetPlayerSpeed(float v) {
		playerSpeed_ = v; // プレイヤーの飛行速度を設定
	}

	void ClearSequenceController::SetPlayerFlyMinTime(float v) {
		playerSpeed_ = v; // プレイヤーの飛行の最短継続時間を設定
	}

	void ClearSequenceController::SetPlayerFlyDistance(float v) {
		playerFlyDistance_ = v; // プレイヤーの飛行距離を設定
	}

	void ClearSequenceController::UpdateCamZoom(float dt, bool& finished) {
		(void)dt;

		if (!camera_) {
			return;
		}

		float t = std::clamp(timer_ / 1.0f, 0.0f, 1.0f);
		Vector3 camPos = MyMath::Vector3Lerp(camStartPos_, camTargetPos_, t);
		camera_->SetTranslate(camPos);
		camera_->Update();

		if (t >= 1.0f) {
			phase_ = Phase::PlayerFly;
			timer_ = 0.0f;

			if (fireworkController_) {
				fireworkController_->Reset();
			}
		}
	}

	void ClearSequenceController::UpdatePlayerFly(float dt, bool& finished) {
		(void)finished;

		if (!camera_ || !player_) {
			return;
		}

		camera_->SetTranslate(camTargetPos_);
		camera_->Update();

		Vector3 pos = player_->GetPosition();
		pos.z += playerSpeed_ * dt;
		player_->SetPosition(pos);

		player_->UpdateVisualOnly(dt); // 移動に合わせて見た目も更新（当たり判定はなし）

		if (fireworkController_) {
			fireworkController_->Update(dt, camera_);
		}

		if (timer_ >= playerFlyMinTime_ &&
			pos.z > playerStartPos_.z + playerFlyDistance_) {

			phase_ = Phase::IrisClose;
			timer_ = 0.0f;

			//  Irisは flow_->Draw() 側で描かれるので、外部描画ON
			if (flow_) {
				flow_->SetExternalIrisDraw(true);
			}

			irisClosing_ = true;
			irisCloseTween_.Reset(
				0.0f,
				flow_ ? flow_->GetIrisMaxScale() : 0.0f,
				kIrisDurationSec_,
				Ease::Type::InBack
			);
		}
	}

	void ClearSequenceController::UpdateIrisClose(float dt, bool& finished) {
		if (!irisClosing_) {
			return;
		}

		UpdateIrisScale(flow_ ? flow_->GetIrisSprite() : nullptr, irisCloseTween_, dt);

		if (irisCloseTween_.Finished()) {
			// ここで externalIrisDraw をOFFにしない（遷移まで保持）
			finished = true;
		}
	}
}