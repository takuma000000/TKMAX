#include "GameFlowController.h"

namespace TKM {
	void GameFlowController::Initialize(DirectXCommon* dxCommon) {
		intro_ = std::make_unique<IntroSequence>();
		intro_->Initialize(dxCommon);

		gameplayLocked_ = true;

		irisClosing_ = false;
		irisToTitle_ = false;
		playerDeathStarted_ = false;
		playerDeathElapsed_ = 0.0f;
	}

	void GameFlowController::Update(float dt, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies) {
		if (intro_) {
			intro_->Update(kFixedDt_, camera, enemiesInitialized, outRequestInitEnemies);
			gameplayLocked_ = intro_->IsGameplayLocked();
		} else {
			gameplayLocked_ = false;
		}
	}

	GameFlowController::TransitionRequest GameFlowController::UpdateTransitions(float dt, Player* player) {
		// ─── プレイヤー死亡 → GameOver ───
		if (player && player->IsDead()) {
			if (!playerDeathStarted_) {
				playerDeathStarted_ = true;
				playerDeathElapsed_ = 0.0f;
			} else {
				playerDeathElapsed_ += kFixedDt_;

				if (playerDeathElapsed_ >= 4.0f && !irisClosing_) {
					irisClosing_ = true;
					irisToTitle_ = false; // GameOverへ
					irisCloseTween_.Reset(
						0.0f,
						intro_ ? intro_->GetIrisMaxScale() : 0.0f,
						kIrisDurationSec_,
						Ease::Type::InBack
					);
				}
			}
		}

		// ─── Tキーでタイトルへ（アイリス閉じ）───
		if (!irisClosing_ && Input::GetInstance()->TriggerKey(DIK_T)) {
			irisClosing_ = true;
			irisToTitle_ = true; // Titleへ
			irisCloseTween_.Reset(
				0.0f,
				intro_ ? intro_->GetIrisMaxScale() : 0.0f,
				kIrisDurationSec_,
				Ease::Type::InBack
			);
		}

		// ─── アイリス閉じ進行 ───
		if (irisClosing_) {
			UpdateIrisScale(intro_ ? intro_->GetIrisSprite() : nullptr, irisCloseTween_, kFixedDt_);

			if (irisCloseTween_.Finished()) {
				return irisToTitle_ ? TransitionRequest::ToTitle : TransitionRequest::ToGameOver;
			}
		}

		return TransitionRequest::None;
	}

	void GameFlowController::Draw() const {
		if (intro_) {
			// IntroSequence側が irisClosing を見て Iris/Start を描く想定
			intro_->Draw(irisClosing_);
		}
	}

	bool GameFlowController::IsGameplayLocked() const {
		// Intro中ロック（いまの責務）
		return gameplayLocked_;
	}

	Sprite* GameFlowController::GetIrisSprite() const {
		return intro_ ? intro_->GetIrisSprite() : nullptr;
	}

	float GameFlowController::GetIrisMaxScale() const {
		return intro_ ? intro_->GetIrisMaxScale() : 0.0f;
	}
} // namespace TKM