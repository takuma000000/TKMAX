#include "IntroSequence.h"
#include "IntroFlowStates.h"
#include <algorithm>
#include "ParticleManager.h"

namespace TKM {
	void IntroSequence::Initialize(DirectXCommon* dxCommon, TKM::Object3dCommon* object3dCommon) {
		object3dCommon_ = object3dCommon;
		dxCommon_ = dxCommon;

		// Iris（開始は画面を覆った状態→開く）
		iris_ = CreateCenteredIrisSprite(dxCommon, irisMaxScale_);
		irisTween_.Reset(irisMaxScale_, 0.0f, kIrisDurationSec_, Ease::Type::OutBack); // 最初は画面全体を覆う状態にセット

		// start.png（最初は非表示）
		startBanner_.Initialize(dxCommon);
		startBanner_.Reset(); // 非表示状態にリセット

		// 初期状態
		gameplayLocked_ = true;
		irisOpening_ = true;
		emitOpenBurst_ = true;
		emitOpenElapsed_ = 0.0f;
		emitFireworkPending_ = false;
		lastEmitPos_ = { 0,0,0 };
		phase_ = Phase::IrisOpen;
		introBoss_.reset();
		introBossPhaseElapsed_ = 0.0f;
		introBossPos_ = { 0.0f, 6.0f, introBossAppearStartZ_ };
		introBossBasePos_ = introBossPos_;
		introBossEscapeTargetX_ = 0.0f;
		introBossEscapeTargetTimer_ = 0.0f;
		camBlendToBossActive_ = false;
		camBlendBackActive_ = false;
		camSavedRot_ = { 0.0f, 0.0f, 0.0f };
		camBossStartRot_ = { 0.0f, 0.0f, 0.0f };
		camBossTargetRot_ = { 0.0f, 0.0f, 0.0f };
		camReturnStartRot_ = { 0.0f, 0.0f, 0.0f };
		introBossPreSpawnElapsed_ = 0.0f;
		introBossPreSpawnEmitAccum_ = 0.0f;
		introBossSpawnFxFinished_ = false;
		introBossNoticeMarkEmitted_ = false;
		introBossEscapeWarpBurstEmitted_ = false;

		flowSM_.Initialize(this);
		flowSM_.Change(std::make_unique<IntroIrisOpenState>());
	}

	void IntroSequence::Update(float dt, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies) {
		(void)dt;

		if (!camera || !iris_) { return; }

		currentCamera_ = camera;
		currentEnemiesInitialized_ = enemiesInitialized;
		currentOutRequestInitEnemies_ = &outRequestInitEnemies;

		flowSM_.Update(kFixedDt_);

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

	void IntroSequence::Draw(DirectXCommon* dxCommon, bool irisClosing) const {
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
		if (introBoss_) {
			introBoss_->Draw(dxCommon); // イントロ用ボスの3D描画
		}
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