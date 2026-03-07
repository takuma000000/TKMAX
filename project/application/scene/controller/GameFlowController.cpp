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

	void GameFlowController::Update(float rawDeltaTime, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies) {
		if (intro_) {
			intro_->Update(kFixedDeltaTime_, camera, enemiesInitialized, outRequestInitEnemies);
			gameplayLocked_ = intro_->IsGameplayLocked();
		} else {
			gameplayLocked_ = false;
		}
	}

	GameFlowController::TransitionRequest GameFlowController::UpdateTransitions(float rawDeltaTime, Player* player) {
		(void)rawDeltaTime; // 現状：固定dt（kFixedDeltaTime_）で進行

		// 1) 保留中リクエストがあれば最優先で返す
		{
			const auto req = ConsumePendingRequest_();
			if (req != TransitionRequest::None) { return req; }
		}

		// 2) 死亡 →（4秒後）GameOver 遷移のため Iris 閉じ開始
		HandlePlayerDeathTransition_(player);

		// 3) Tキーでタイトルへ（Iris閉じ開始）
		TryStartTitleTransitionByKey_();

		// 4) Iris閉じ進行（閉じ終わったら遷移要求を返す）
		{
			const auto req = StepIrisClosing_();
			if (req != TransitionRequest::None) { return req; }
		}

		return TransitionRequest::None;
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

	void GameFlowController::BindClearSequence(ClearSequenceController* clearSeq) {
		clearSeq_ = clearSeq; // クリアシーケンスコントローラをバインド
	}

	bool GameFlowController::IsInClear() const {
		return (clearSeq_ && clearSeq_->IsActive()); // クリアシーケンスがアクティブか？
	}

	bool GameFlowController::UpdateClear(float rawDeltaTime, float scaledDeltaTime, PostEffectController* postFx, UIController* ui, BossManager* bossManager, Camera* camera, Player* player) {
		if (!clearSeq_ || !clearSeq_->IsActive()) {
			return false; // クリア中じゃない
		}

		// クリア演出本体（スロー非依存）
		const bool finished = clearSeq_->Update(rawDeltaTime);

		// クリア中でも動かしたいもの（止めない）
		ParticleManager::GetInstance()->Update(scaledDeltaTime);

		if (postFx) { // ポストエフェクト更新
			postFx->Update(scaledDeltaTime, bossManager); // ボスマネージャ参照
			if (camera) { // カメラ更新通知
				postFx->OnCameraUpdated(camera); // カメラ更新通知
			}
		}
		if (ui) { // UI更新
			ui->Update(scaledDeltaTime, player); // プレイヤー参照
		}
		if (finished) { // クリアシーケンス完了
			pendingRequest_ = TransitionRequest::ToGameClear; // 遷移要求セット
		}

		return true; // クリア中なので “処理済み”
	}

	void GameFlowController::RequestStartClear() {
		if (!clearSeq_) { return; } // バインドされていない
		clearSeq_->Start(); // クリアシーケンス開始
	}

	bool GameFlowController::IsGameplayLocked() const {
		return gameplayLocked_ || IsInClear(); // クリア中もロック
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

	GameFlowController::TransitionRequest GameFlowController::ConsumePendingRequest_() {
		if (pendingRequest_ == TransitionRequest::None) { return TransitionRequest::None; }

		const auto req = pendingRequest_;
		pendingRequest_ = TransitionRequest::None;
		return req;
	}

	void GameFlowController::HandlePlayerDeathTransition_(Player* player) {
		if (!player) { return; }
		if (!player->IsDead()) { return; }

		// 初回：開始だけして終わり（ネストを浅くする）
		if (!playerDeathStarted_) {
			playerDeathStarted_ = true;
			playerDeathElapsed_ = 0.0f;
			return;
		}

		playerDeathElapsed_ += kFixedDeltaTime_;

		// 4秒経過したら GameOver 用 Iris 閉じ開始（1回だけ）
		if (playerDeathElapsed_ < 4.0f) { return; }
		BeginIrisClosing_(false);
	}

	void GameFlowController::TryStartTitleTransitionByKey_() {
		if (irisClosing_) { return; }
		if (!Input::GetInstance()->TriggerKey(DIK_T)) { return; }

		BeginIrisClosing_(true);
	}

	void GameFlowController::BeginIrisClosing_(bool toTitle) {
		if (irisClosing_) { return; } // 多重開始防止

		irisClosing_ = true;
		irisToTitle_ = toTitle;

		irisCloseTween_.Reset(
			0.0f,
			intro_ ? intro_->GetIrisMaxScale() : 0.0f,
			kIrisDurationSec_,
			Ease::Type::InBack
		);
	}

	GameFlowController::TransitionRequest GameFlowController::StepIrisClosing_() {
		if (!irisClosing_) { return TransitionRequest::None; }

		// アイリスのスケールを更新
		UpdateIrisScale(intro_ ? intro_->GetIrisSprite() : nullptr, irisCloseTween_, kFixedDeltaTime_);

		// 閉じ終わってなければ遷移要求はまだ出さない
		if (!irisCloseTween_.Finished()) { return TransitionRequest::None; }

		return irisToTitle_ ? TransitionRequest::ToTitle : TransitionRequest::ToGameOver; // 閉じ終わったらタイトルへ or ゲームオーバーへ遷移要求
	}
} // namespace TKM