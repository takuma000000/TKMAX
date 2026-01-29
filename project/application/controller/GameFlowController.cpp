#include "GameFlowController.h"
#include "ClearSequenceController.h"
#include "PostEffectController.h"
#include "UIController.h"
#include "manager/BossManager.h"
#include "ParticleManager.h"

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
		// ─── 保留中の遷移要求があれば優先して返す ───
		if (pendingRequest_ != TransitionRequest::None) { // 保留中の遷移要求あり
			const auto req = pendingRequest_; // 退避
			pendingRequest_ = TransitionRequest::None; // 保留解除
			return req; // 要求返す
		}

		// ─── プレイヤー死亡 → GameOver ───
		if (player && player->IsDead()) {
			if (!playerDeathStarted_) { // 初回
				playerDeathStarted_ = true; // フラグ立て
				playerDeathElapsed_ = 0.0f; // 経過時間リセット
			} else {
				playerDeathElapsed_ += kFixedDt_; // 経過時間加算

				if (playerDeathElapsed_ >= 4.0f && !irisClosing_) { // 4秒経過したらアイリス閉じ開始
					irisClosing_ = true; // アイリス閉じ開始
					irisToTitle_ = false; // GameOverへ
					irisCloseTween_.Reset( // Tweenリセット
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
			irisClosing_ = true; // アイリス閉じ開始
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
			UpdateIrisScale(intro_ ? intro_->GetIrisSprite() : nullptr, irisCloseTween_, kFixedDt_); // アイリススケール更新

			if (irisCloseTween_.Finished()) { // 閉じ完了
				return irisToTitle_ ? TransitionRequest::ToTitle : TransitionRequest::ToGameOver;
			}
		}

		return TransitionRequest::None; // 遷移なし
	}

	void GameFlowController::Draw() const {
		if (intro_) {
			// Iris閉じ or 外部制御なら描画
			intro_->Draw(irisClosing_ || externalIrisDraw_);
		}
	}

	void GameFlowController::RequestToTitleByIris() {
		if (irisClosing_) { return; }
		
		// アイリス閉じ開始
		irisClosing_ = true;
		irisToTitle_ = true;
		irisCloseTween_.Reset(
			0.0f,
			intro_ ? intro_->GetIrisMaxScale() : 0.0f,
			kIrisDurationSec_,
			Ease::Type::InBack
		);
	}

	bool GameFlowController::UpdateClear(float rawDt, float scaledDt, ClearSequenceController* clearSeq, PostEffectController* postFx, UIController* ui, BossManager* bossManager, Camera* camera, Player* player) {
		if (!clearSeq || !clearSeq->IsActive()) {
			return false; // クリア中じゃない
		}

		// クリア演出本体（スロー非依存）
		const bool finished = clearSeq->Update(rawDt);

		// クリア中でも動かしたいもの（止めない）
		ParticleManager::GetInstance()->Update(scaledDt);

		if (postFx) {
			postFx->Update(scaledDt, bossManager);
			if (camera) {
				postFx->OnCameraUpdated(camera);
			}
		}

		if (ui) {
			ui->Update(scaledDt, player);
		}

		// 完了したら「遷移要求」を溜める（GameScene側でSetNextSceneする）
		if (finished) {
			pendingRequest_ = TransitionRequest::ToGameClear;
		}

		return true; // クリア中なので “処理済み”
	}

	void GameFlowController::RequestStartClear(ClearSequenceController* clearSeq) {
		if (!clearSeq) { return; }
		clearSeq->Start();
	}

	bool GameFlowController::IsGameplayLocked() const {
		// Intro中ロック（いまの責務）
		return gameplayLocked_;
	}

	void GameFlowController::SetExternalIrisDraw(bool enable) {
		externalIrisDraw_ = enable;
	}

	Sprite* GameFlowController::GetIrisSprite() const {
		return intro_ ? intro_->GetIrisSprite() : nullptr;
	}

	float GameFlowController::GetIrisMaxScale() const {
		return intro_ ? intro_->GetIrisMaxScale() : 0.0f;
	}
} // namespace TKM