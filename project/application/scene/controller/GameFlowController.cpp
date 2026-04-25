#include "GameFlowController.h"
#include "ClearSequenceController.h"
#include "PostEffectController.h"
#include "UIController.h"
#include "manager/BossManager.h"
#include "ParticleManager.h"

namespace TKM {
	void GameFlowController::Initialize(DirectXCommon* dxCommon, TKM::Object3dCommon* object3dCommon) {
		// イントロシーケンスを生成する
		intro_ = std::make_unique<IntroSequence>();

		// イントロシーケンスを初期化する
		intro_->Initialize(dxCommon, object3dCommon);

		// イントロ中はゲームプレイをロックする
		gameplayLocked_ = true;

		// アイリス閉じ状態を初期化する
		irisClosing_ = false;

		// タイトル遷移フラグを初期化する
		irisToTitle_ = false;

		// プレイヤー死亡遷移の開始フラグを初期化する
		playerDeathStarted_ = false;

		// プレイヤー死亡後の経過時間を初期化する
		playerDeathElapsed_ = 0.0f;
	}

	void GameFlowController::Update(float rawDeltaTime, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies) {
		// イントロがある場合はイントロ側の状態を更新する
		if (intro_) {
			intro_->Update(kFixedDeltaTime_, camera, enemiesInitialized, outRequestInitEnemies);

			// イントロ側のロック状態をゲームフロー側へ反映する
			gameplayLocked_ = intro_->IsGameplayLocked();
		} else {
			// イントロが無い場合はゲームプレイをロックしない
			gameplayLocked_ = false;
		}
	}

	GameFlowController::TransitionRequest GameFlowController::UpdateTransitions(float rawDeltaTime, Player* player) {
		// 現状は固定dtで遷移演出を進めるため、rawDeltaTimeは使わない
		(void)rawDeltaTime;

		// プレイヤー死亡からGameOverへ進む遷移を確認する
		HandlePlayerDeathTransition_(player);

		// Tキーによるタイトル遷移開始を確認する
		TryStartTitleTransitionByKey_();

		// アイリス閉じ中でなければ、保留中の遷移要求を先に返す
		if (pendingRequest_ != TransitionRequest::None && !irisClosing_) {
			return ConsumePendingRequest_();
		}

		// アイリス閉じ演出を進め、閉じ終わっていれば遷移要求を返す
		{
			const auto req = StepIrisClosing_();
			if (req != TransitionRequest::None) { return req; }
		}

		// このフレームでは遷移なし
		return TransitionRequest::None;
	}

	void GameFlowController::Draw() const {
		// アイリス閉じ中、または外部制御中ならイントロ側でアイリスを描画する
		intro_->Draw(irisClosing_ || externalIrisDraw_);
	}

	void GameFlowController::DrawIntroBoss3D(DirectXCommon* dxCommon) const {
		// イントロがある場合だけ、イントロ用ボスを描画する
		if (intro_) {
			intro_->DrawIntroBoss3D(dxCommon);
		}
	}

	void GameFlowController::RequestToTitleByIris() {
		// すでにアイリス閉じ中なら二重開始しない
		if (irisClosing_) { return; }

		// アイリス閉じを開始する
		irisClosing_ = true;

		// 閉じ終わった後はタイトルへ遷移する
		irisToTitle_ = true;

		// アイリス閉じ用Tweenを開始する
		irisCloseTween_.Reset(
			0.0f,
			intro_ ? intro_->GetIrisMaxScale() : 0.0f,
			kIrisDurationSec_,
			Ease::Type::InBack
		);
	}

	void GameFlowController::BindClearSequence(ClearSequenceController* clearSeq) {
		// クリアシーケンスコントローラー参照を保持する
		clearSeq_ = clearSeq;
	}

	bool GameFlowController::IsInClear() const {
		// クリアシーケンスが存在し、実行中かどうかを返す
		return (clearSeq_ && clearSeq_->IsActive());
	}

	bool GameFlowController::UpdateClear(float rawDeltaTime, float scaledDeltaTime, PostEffectController* postFx, UIController* ui, BossManager* bossManager, Camera* camera, Player* player) {
		// クリアシーケンスが無い、またはアクティブでなければ処理しない
		if (!clearSeq_ || !clearSeq_->IsActive()) {
			return false;
		}

		// クリア演出本体はスロー非依存のrawDeltaTimeで更新する
		const bool finished = clearSeq_->Update(rawDeltaTime);

		// クリア中でもパーティクルは更新し続ける
		ParticleManager::GetInstance()->Update(scaledDeltaTime);

		// ポストエフェクトがある場合は更新する
		if (postFx) {
			// ボスマネージャー参照も渡してポストエフェクトを更新する
			postFx->Update(scaledDeltaTime, bossManager);

			// カメラがある場合は、カメラ更新後の情報をポストエフェクトへ渡す
			if (camera) {
				postFx->OnCameraUpdated(camera);
			}
		}

		// UIがある場合はクリア中も更新する
		if (ui) {
			ui->Update(scaledDeltaTime, player);
		}

		// クリアシーケンスが完了したらGameClearへの遷移要求を保留する
		if (finished) {
			pendingRequest_ = TransitionRequest::ToGameClear;
		}

		// クリア中の処理を行ったことを返す
		return true;
	}

	void GameFlowController::RequestStartClear() {
		// クリアシーケンスが未バインドなら開始できない
		if (!clearSeq_) { return; }

		// クリアシーケンスを開始する
		clearSeq_->Start();
	}

	bool GameFlowController::IsGameplayLocked() const {
		// イントロ中、またはクリア中ならゲームプレイをロックする
		return gameplayLocked_ || IsInClear();
	}

	void GameFlowController::SetExternalIrisDraw(bool enable) {
		// 外部からアイリス描画を行うかどうかを設定する
		externalIrisDraw_ = enable;
	}

	Sprite* GameFlowController::GetIrisSprite() const {
		// イントロがあればアイリススプライトを返す
		return intro_ ? intro_->GetIrisSprite() : nullptr;
	}

	float GameFlowController::GetIrisMaxScale() const {
		// イントロがあればアイリス最大スケールを返す
		return intro_ ? intro_->GetIrisMaxScale() : 0.0f;
	}

	GameFlowController::TransitionRequest GameFlowController::ConsumePendingRequest_() {
		// 保留中の遷移要求が無ければNoneを返す
		if (pendingRequest_ == TransitionRequest::None) { return TransitionRequest::None; }

		// 保留中の要求を退避する
		const auto req = pendingRequest_;

		// 一度返した要求は消す
		pendingRequest_ = TransitionRequest::None;

		// 退避した要求を返す
		return req;
	}

	void GameFlowController::HandlePlayerDeathTransition_(Player* player) {
		// プレイヤーが無ければ死亡遷移しない
		if (!player) { return; }

		// プレイヤーが死んでいなければ死亡遷移しない
		if (!player->IsDead()) { return; }

		// 死亡を初めて検知したタイミングでは、タイマー開始だけ行う
		if (!playerDeathStarted_) {
			playerDeathStarted_ = true;
			playerDeathElapsed_ = 0.0f;
			return;
		}

		// 死亡後の経過時間を進める
		playerDeathElapsed_ += kFixedDeltaTime_;

		// 2秒経過するまでは遷移開始しない
		if (playerDeathElapsed_ < 2.0f) { return; }

		// GameOver用のアイリス閉じを開始する
		BeginIrisClosing_(false);
	}

	void GameFlowController::TryStartTitleTransitionByKey_() {
		// すでにアイリス閉じ中なら開始しない
		if (irisClosing_) { return; }

		// Tキーが押されていなければ開始しない
		if (!Input::GetInstance()->TriggerKey(DIK_T)) { return; }

		// タイトル用のアイリス閉じを開始する
		BeginIrisClosing_(true);
	}

	void GameFlowController::BeginIrisClosing_(bool toTitle) {
		// すでにアイリス閉じ中なら二重開始しない
		if (irisClosing_) { return; }

		// アイリス閉じを開始する
		irisClosing_ = true;

		// 閉じ終わった後の遷移先がタイトルかどうかを保存する
		irisToTitle_ = toTitle;

		// アイリス閉じ用Tweenを開始する
		irisCloseTween_.Reset(
			0.0f,
			intro_ ? intro_->GetIrisMaxScale() : 0.0f,
			kIrisDurationSec_,
			Ease::Type::InBack
		);
	}

	void GameFlowController::RequestRestartByIris() {
		// すでにアイリス閉じ中なら二重開始しない
		if (irisClosing_) { return; }

		// アイリス閉じを開始する
		irisClosing_ = true;

		// タイトルではなくリスタート遷移として扱う
		irisToTitle_ = false;

		// 閉じ終わった後にリスタート遷移要求を返すようにする
		pendingRequest_ = TransitionRequest::ToRestart;

		// アイリス閉じ用Tweenを開始する
		irisCloseTween_.Reset(
			0.0f,
			intro_ ? intro_->GetIrisMaxScale() : 0.0f,
			kIrisDurationSec_,
			Ease::Type::InBack
		);
	}

	GameFlowController::TransitionRequest GameFlowController::StepIrisClosing_() {
		// アイリス閉じ中でなければ遷移なし
		if (!irisClosing_) { return TransitionRequest::None; }

		// アイリススプライトのスケールを更新する
		UpdateIrisScale(intro_ ? intro_->GetIrisSprite() : nullptr, irisCloseTween_, kFixedDeltaTime_);

		// まだ閉じ終わっていなければ遷移なし
		if (!irisCloseTween_.Finished()) { return TransitionRequest::None; }

		// 保留中の遷移要求があればそちらを優先して返す
		if (pendingRequest_ != TransitionRequest::None) {
			return ConsumePendingRequest_();
		}

		// 保留要求が無ければ、タイトルまたはゲームオーバーへ遷移する
		return irisToTitle_ ? TransitionRequest::ToTitle : TransitionRequest::ToGameOver;
	}
} // namespace TKM