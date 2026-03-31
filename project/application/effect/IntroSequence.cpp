#include "IntroSequence.h"
#include "IntroFlowStates.h"

namespace TKM {
	void IntroSequence::Initialize(DirectXCommon* dxCommon, TKM::Object3dCommon* object3dCommon) {
		// Iris（開始は画面を覆った状態→開く）
		iris_ = CreateCenteredIrisSprite(dxCommon, irisMaxScale_);
		irisTween_.Reset(irisMaxScale_, 0.0f, kIrisDurationSec_, Ease::Type::OutBack); // 最初は画面全体を覆う状態にセット

		// start.png（最初は非表示）
		startBanner_.Initialize(dxCommon);

		// イントロ用ボス
		introBossActor_.Initialize(object3dCommon, dxCommon);
		introBossActor_.Reset();

		// 状態遷移の初期化
		flowSM_.Initialize(this);
		flowSM_.Change(std::make_unique<IntroIrisOpenState>());

		// 初期状態
		gameplayLocked_ = true;
		irisOpening_ = true;
		emitOpenBurst_ = true;
		emitOpenElapsed_ = 0.0f;
		emitFireworkPending_ = false;
		lastEmitPos_ = { 0,0,0 };
		phase_ = Phase::IrisOpen;
		
		camBlendToBossActive_ = false;
		camBlendBackActive_ = false;
		camSavedRot_ = { 0.0f, 0.0f, 0.0f };
		camBossStartRot_ = { 0.0f, 0.0f, 0.0f };
		camBossTargetRot_ = { 0.0f, 0.0f, 0.0f };
		camReturnStartRot_ = { 0.0f, 0.0f, 0.0f };
	}

	void IntroSequence::Update(float dt, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies) {
		(void)dt;

		if (!camera || !iris_) { return; }

		currentCamera_ = camera;
		currentEnemiesInitialized_ = enemiesInitialized;
		currentOutRequestInitEnemies_ = &outRequestInitEnemies;

		flowSM_.Update(kFixedDt_); // 状態の更新

		// 状態の更新後に、スキップ入力の処理やカメラブレンドの更新を行う
		if (CanSkipBossIntro()) {

			const bool isSkipPressed =
				Input::GetInstance()->PushKey(DIK_SPACE) ||
				Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A);

			if (isSkipPressed) {
				skipHoldTimer_ += kFixedDt_;

				if (skipHoldTimer_ >= kSkipHoldSec_) {
					SkipBossIntroToShowStart();
					skipHoldTimer_ = 0.0f;
				}
			} else {
				skipHoldTimer_ = 0.0f;
			}
		} else {
			skipHoldTimer_ = 0.0f;
		}

		// --- Boss camera blend in ---
		if (camBlendToBossActive_) {
			float t = camBlendToBossTween_.Update(kFixedDt_);

			Vector3 rot{};
			rot.x = MyMath::Lerp(camBossStartRot_.x, camBossTargetRot_.x, t);
			rot.y = MyMath::Lerp(camBossStartRot_.y, camBossTargetRot_.y, t);
			rot.z = MyMath::Lerp(camBossStartRot_.z, camBossTargetRot_.z, t);

			camera->SetRotate(rot);

			if (camBlendToBossTween_.Finished()) {
				camBlendToBossActive_ = false;
				camera->SetRotate(camBossTargetRot_);
			}
		}

		// --- Boss camera blend back ---
		if (camBlendBackActive_) {
			float t = camBlendBackTween_.Update(kFixedDt_);

			Vector3 rot{};
			rot.x = MyMath::Lerp(camReturnStartRot_.x, camSavedRot_.x, t);
			rot.y = MyMath::Lerp(camReturnStartRot_.y, camSavedRot_.y, t);
			rot.z = MyMath::Lerp(camReturnStartRot_.z, camSavedRot_.z, t);

			camera->SetRotate(rot);

			if (camBlendBackTween_.Finished()) {
				camBlendBackActive_ = false;
				camera->SetRotate(camSavedRot_);
			}
		}

		// --- Boss phase camera follow ---
		if (!camBlendToBossActive_ && !camBlendBackActive_) {
			if (phase_ == Phase::BossAppear ||
				phase_ == Phase::BossPause ||
				phase_ == Phase::BossNoticeHop ||
				phase_ == Phase::BossPanic ||
				phase_ == Phase::BossEscape) {

				Vector3 nowRot = camera->GetRotate();
				Vector3 nextRot{};
				float follow = 8.0f * kFixedDt_;

				nextRot.x = MyMath::Lerp(nowRot.x, camBossTargetRot_.x, follow);
				nextRot.y = MyMath::Lerp(nowRot.y, camBossTargetRot_.y, follow);
				nextRot.z = MyMath::Lerp(nowRot.z, camBossTargetRot_.z, follow);

				camera->SetRotate(nextRot);
			}
		}

		currentCamera_ = nullptr;
		currentOutRequestInitEnemies_ = nullptr;
	}

	void IntroSequence::Draw(bool irisClosing) const {
		// Iris（開いているとき、または閉じる演出中は描画）
		if (irisOpening_ && iris_) {
			iris_->Draw();
		}
		// アイリスが閉じる演出中は描画
		if (irisClosing && iris_) {
			iris_->Draw();
		}
		// 「ゲームスタート」表示
		startBanner_.Draw();
	}

	void IntroSequence::DrawIntroBoss3D(DirectXCommon* dxCommon) const {
		introBossActor_.Draw(dxCommon); // ボスが存在するフェーズのみ描画
	}

	bool IntroSequence::CanSkipBossIntro() const {
		return
			phase_ == Phase::BossPreSpawn ||
			phase_ == Phase::BossAppear ||
			phase_ == Phase::BossPause ||
			phase_ == Phase::BossNoticeHop ||
			phase_ == Phase::BossPanic ||
			phase_ == Phase::BossEscape;
	}

	void IntroSequence::SkipBossIntroToShowStart() {
		if (!CanSkipBossIntro()) {
			return;
		}

		// イントロ用ボスを消す
		introBossActor_.Reset();

		// カメラを通常側へ戻す
		camBlendToBossActive_ = false;
		camBlendBackActive_ = false;

		if (currentCamera_) {
			currentCamera_->SetRotate(camSavedRot_);
		}

		// 「ゲームスタート」へ進める
		phase_ = Phase::ShowStart;
		flowSM_.Change(std::make_unique<IntroShowStartState>());
	}

	bool IntroSequence::IsBossSkyRedPhase() const {
		//return phase_ == Phase::BossPreSpawn ||
			return phase_ == Phase::BossAppear ||
			phase_ == Phase::BossPause ||
			phase_ == Phase::BossNoticeHop ||
			phase_ == Phase::BossPanic ||
			phase_ == Phase::BossEscape;
	}
} // namespace TKM