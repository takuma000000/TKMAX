#include "ClearSequenceController.h"

namespace TKM {
	void ClearSequenceController::Start(
		DirectXCommon* dxCommon,
		BossManager* bossManager,
		Player* player,
		Camera* camera,
		VignettingEffect* vignetting,
		GameFlowController* flow,
		FireworkController* firework,
		Skybox* skybox
	) {
		dxCommon_ = dxCommon;
		bossManager_ = bossManager;
		player_ = player;
		camera_ = camera;
		vignetting_ = vignetting;
		flow_ = flow;
		firework_ = firework;
		skybox_ = skybox;

		active_ = true;
		phase_ = Phase::CamZoom;
		timer_ = 0.0f;

		// GameScene側にあった「開始時にやること」がもしあるなら、ここに移す
		// ※今は“UpdateClearSequence移植”が目的なので、必要最低限だけ

		if (camera_) {
			camStartPos_ = camera_->GetTranslate();
		}
		// GameSceneで使ってた targetPos をここにコピーして調整してね（とりあえず同じ）
		camTargetPos_ = camStartPos_;
		camTargetPos_.z -= 12.0f;

		if (player_) {
			playerStartPos_ = player_->GetPosition();
		}

		irisClosing_ = false;
	}

	bool ClearSequenceController::Update(float dt) {
		if (!active_) { return false; }

		timer_ += dt;

		// skyboxはずっと回し続ける（GameSceneと同じ）
		if (skybox_) {
			skybox_->UpdateRotation();
		}

		bool finished = false;

		switch (phase_) {
		case Phase::CamZoom:
		{
			float t = std::clamp(timer_ / 1.0f, 0.0f, 1.0f);

			if (camera_) {
				Vector3 camPos = MyMath::Vector3Lerp(camStartPos_, camTargetPos_, t);
				camera_->SetTranslate(camPos);
				camera_->Update();
			}

			if (t >= 1.0f) {
				phase_ = Phase::PlayerFly;
				timer_ = 0.0f;

				if (firework_) {
					firework_->Reset();
				}
			}
		}
		break;

		case Phase::PlayerFly:
		{
			if (camera_) {
				camera_->SetTranslate(camTargetPos_);
				camera_->Update();
			}

			if (player_) {
				Vector3 pos = player_->GetPosition();
				pos.z += playerSpeed_ * dt;
				player_->SetPosition(pos);
				player_->UpdateVisualOnly();

				// 花火はControllerへ委譲（引数2つ）
				if (firework_) {
					firework_->Update(dt, camera_);
				}

				if (timer_ >= playerFlyMinTime_ &&
					pos.z > playerStartPos_.z + playerFlyDistance_) {

					phase_ = Phase::IrisClose;
					timer_ = 0.0f;

					irisClosing_ = true;
					irisCloseTween_.Reset(
						0.0f,
						flow_ ? flow_->GetIrisMaxScale() : 0.0f,
						kIrisDurationSec_,
						Ease::Type::InBack
					);
				}
			}
		}
		break;

		case Phase::IrisClose:
		{
			if (irisClosing_) {
				UpdateIrisScale(flow_ ? flow_->GetIrisSprite() : nullptr, irisCloseTween_, dt);
				if (irisCloseTween_.Finished()) {
					finished = true;
					active_ = false;
				}
			}
		}
		break;
		}

		return finished;
	}
} // namespace TKM