#include "IntroSequence.h"
#include "IntroFlowStates.h"

namespace TKM {
	void IntroSequence::Initialize(DirectXCommon* dxCommon, TKM::Object3dCommon* object3dCommon) {
		//=========================================================
		// Iris初期化（最初は画面全体を覆った状態）
		//=========================================================
		iris_ = CreateCenteredIrisSprite(dxCommon, irisMaxScale_);

		// 画面全体 → 開くアニメーションを設定する
		irisTween_.Reset(irisMaxScale_, 0.0f, kIrisDurationSec_, Ease::Type::OutBack);

		//=========================================================
		// 「ゲームスタート」バナー初期化（最初は非表示）
		//=========================================================
		startBanner_.Initialize(dxCommon);

		//=========================================================
		// イントロ用ボス初期化
		//=========================================================
		introBossActor_.Initialize(object3dCommon, dxCommon);
		introBossActor_.Reset();

		//=========================================================
		// ステートマシン初期化
		//=========================================================
		flowSM_.Initialize(this);

		// 最初はアイリスオープン状態から開始
		flowSM_.Change(std::make_unique<IntroIrisOpenState>());

		//=========================================================
		// 初期状態フラグ
		//=========================================================
		gameplayLocked_ = true;     // ゲーム操作をロック
		irisOpening_ = true;       // Iris開いている状態
		emitOpenBurst_ = true;     // 開幕エフェクトON
		emitOpenElapsed_ = 0.0f;   // エフェクトタイマー初期化
		emitFireworkPending_ = false;
		lastEmitPos_ = { 0,0,0 };

		// 現在フェーズをIrisOpenにする
		phase_ = Phase::IrisOpen;

		//=========================================================
		// カメラブレンド初期化
		//=========================================================
		camBlendToBossActive_ = false;
		camBlendBackActive_ = false;

		camSavedRot_ = { 0.0f, 0.0f, 0.0f };       // 元のカメラ回転
		camBossStartRot_ = { 0.0f, 0.0f, 0.0f };   // ボス用開始回転
		camBossTargetRot_ = { 0.0f, 0.0f, 0.0f };  // ボス用目標回転
		camReturnStartRot_ = { 0.0f, 0.0f, 0.0f }; // 戻り開始回転
	}

	void IntroSequence::Update(float dt, Camera* camera, bool enemiesInitialized, bool& outRequestInitEnemies) {
		(void)dt;

		// カメラまたはIrisが無ければ更新しない
		if (!camera || !iris_) { return; }

		//=========================================================
		// 現在フレーム情報を保持
		//=========================================================
		currentCamera_ = camera;
		currentEnemiesInitialized_ = enemiesInitialized;
		currentOutRequestInitEnemies_ = &outRequestInitEnemies;

		//=========================================================
		// ステート更新（固定Δ時間）
		//=========================================================
		flowSM_.Update(kFixedDt_);

		//=========================================================
		// スキップ入力処理
		//=========================================================
		if (CanSkipBossIntro()) {

			const bool isSkipPressed =
				Input::GetInstance()->PushKey(DIK_SPACE) ||
				Input::GetInstance()->PushButton(XINPUT_GAMEPAD_A);

			// 押し続けている間カウント
			if (isSkipPressed) {
				skipHoldTimer_ += kFixedDt_;

				// 一定時間押したらスキップ発動
				if (skipHoldTimer_ >= kSkipHoldSec_) {
					SkipBossIntroToShowStart();
					skipHoldTimer_ = 0.0f;
				}
			} else {
				// 離したらリセット
				skipHoldTimer_ = 0.0f;
			}
		} else {
			// スキップ不可フェーズならリセット
			skipHoldTimer_ = 0.0f;
		}

		//=========================================================
		// ボスカメラへブレンド（遷移中）
		//=========================================================
		if (camBlendToBossActive_) {
			float t = camBlendToBossTween_.Update(kFixedDt_);

			Vector3 rot{};
			rot.x = MyMath::Lerp(camBossStartRot_.x, camBossTargetRot_.x, t);
			rot.y = MyMath::Lerp(camBossStartRot_.y, camBossTargetRot_.y, t);
			rot.z = MyMath::Lerp(camBossStartRot_.z, camBossTargetRot_.z, t);

			camera->SetRotate(rot);

			// 完了したら固定
			if (camBlendToBossTween_.Finished()) {
				camBlendToBossActive_ = false;
				camera->SetRotate(camBossTargetRot_);
			}
		}

		//=========================================================
		// カメラを元に戻すブレンド
		//=========================================================
		if (camBlendBackActive_) {
			float t = camBlendBackTween_.Update(kFixedDt_);

			Vector3 rot{};
			rot.x = MyMath::Lerp(camReturnStartRot_.x, camSavedRot_.x, t);
			rot.y = MyMath::Lerp(camReturnStartRot_.y, camSavedRot_.y, t);
			rot.z = MyMath::Lerp(camReturnStartRot_.z, camSavedRot_.z, t);

			camera->SetRotate(rot);

			// 完了したら固定
			if (camBlendBackTween_.Finished()) {
				camBlendBackActive_ = false;
				camera->SetRotate(camSavedRot_);
			}
		}

		//=========================================================
		// ボス演出中はカメラを追従させる
		//=========================================================
		if (!camBlendToBossActive_ && !camBlendBackActive_) {
			if (phase_ == Phase::BossAppear ||
				phase_ == Phase::BossPause ||
				phase_ == Phase::BossNoticeHop ||
				phase_ == Phase::BossPanic ||
				phase_ == Phase::BossEscape) {

				Vector3 nowRot = camera->GetRotate();
				Vector3 nextRot{};
				float follow = 8.0f * kFixedDt_;

				// 徐々にボスカメラへ寄せる
				nextRot.x = MyMath::Lerp(nowRot.x, camBossTargetRot_.x, follow);
				nextRot.y = MyMath::Lerp(nowRot.y, camBossTargetRot_.y, follow);
				nextRot.z = MyMath::Lerp(nowRot.z, camBossTargetRot_.z, follow);

				camera->SetRotate(nextRot);
			}
		}

		// フレーム参照リセット
		currentCamera_ = nullptr;
		currentOutRequestInitEnemies_ = nullptr;
	}

	void IntroSequence::Draw(bool irisClosing) const {
		// Irisが開いている or 閉じる演出中なら描画
		if (irisOpening_ && iris_) {
			iris_->Draw();
		}
		if (irisClosing && iris_) {
			iris_->Draw();
		}

		// 「ゲームスタート」バナー描画
		startBanner_.Draw();
	}

	void IntroSequence::DrawIntroBoss3D(DirectXCommon* dxCommon) const {
		// ボス描画（存在しているフェーズのみ）
		introBossActor_.Draw(dxCommon);
	}

	bool IntroSequence::CanSkipBossIntro() const {
		// スキップ可能フェーズ判定
		return
			phase_ == Phase::BossPreSpawn ||
			phase_ == Phase::BossAppear ||
			phase_ == Phase::BossPause ||
			phase_ == Phase::BossNoticeHop ||
			phase_ == Phase::BossPanic ||
			phase_ == Phase::BossEscape;
	}

	void IntroSequence::SkipBossIntroToShowStart() {
		// スキップ不可なら何もしない
		if (!CanSkipBossIntro()) {
			return;
		}

		// ボスを強制的に削除
		introBossActor_.Reset();

		// カメラブレンド停止
		camBlendToBossActive_ = false;
		camBlendBackActive_ = false;

		// カメラを元に戻す
		if (currentCamera_) {
			currentCamera_->SetRotate(camSavedRot_);
		}

		// スタート表示フェーズへ遷移
		phase_ = Phase::ShowStart;
		flowSM_.Change(std::make_unique<IntroShowStartState>());
	}

	bool IntroSequence::IsBossSkyRedPhase() const {
		// ボス演出中は空を赤くするフェーズ判定
		return phase_ == Phase::BossAppear ||
			phase_ == Phase::BossPause ||
			phase_ == Phase::BossNoticeHop ||
			phase_ == Phase::BossPanic ||
			phase_ == Phase::BossEscape;
	}
} // namespace TKM