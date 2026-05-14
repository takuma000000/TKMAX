#include "IntroFlowStates.h"
#include "IntroSequence.h"
#include "ParticleManager.h"
#include "AudioManager.h"
#include "PostEffectController.h"
#include <algorithm>

namespace {
	/// <summary>
	/// 共通のStateContextをIntroSequenceとして扱えるように変換します。
	/// </summary>
	/// <param name="ctx">ステートマシンから渡される共通コンテキスト</param>
	/// <returns>IntroSequence参照</returns>
	TKM::IntroSequence& AsIntro_(TKM::IStateContext& ctx) {
		return static_cast<TKM::IntroSequence&>(ctx);
	}
}

namespace TKM {

	void IntroIrisOpenState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをアイリスオープンに設定する
		s.phase_ = IntroSequence::Phase::IrisOpen;

		// アイリス開き中フラグを立てる
		s.irisOpening_ = true;
	}

	void IntroIrisOpenState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラまたはアイリスがない場合は更新できないため終了する
		if (!camera || !s.iris_) { return; }

		// アイリス演出用の固定時間を進める
		s.emitOpenElapsed_ += IntroSequence::kFixedDt_;

		// リング演出は指定ディレイ後に一度だけ発生させる
		if (s.emitOpenBurst_ && s.emitOpenElapsed_ >= s.emitOpenDelaySec_) {
			s.emitOpenBurst_ = false;

			// カメラのワールド行列から位置を取得する
			const Matrix4x4 camW = camera->GetWorldMatrix();
			Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };

			// カメラの前方向を取得する
			Vector3 camFwd = MyMath::Normalize(Vector3{ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

			// カメラの少し前に演出発生位置を作る
			const float depth = 20.0f;
			Vector3 centerInFront = camPos + camFwd * depth;
			centerInFront.y -= 0.1f;

			// アイリスオープン時のリング演出を出す
			ParticleManager::GetInstance()->Emit("irisOpen", centerInFront, 60);

			// 後続の花火演出用に発生位置を保存する
			s.lastEmitPos_ = centerInFront;
			s.emitFireworkPending_ = true;
		}

		// 花火演出はリング演出の後、指定ディレイで一度だけ発生させる
		if (s.emitFireworkPending_ && s.emitOpenElapsed_ >= s.emitFireworkDelaySec_) {
			s.emitFireworkPending_ = false;
			ParticleManager::GetInstance()->Emit("irisFire", s.lastEmitPos_, 80);
		}

		// アイリスTweenを進めてサイズを更新する
		float irisScale = s.irisTween_.Update(IntroSequence::kFixedDt_);
		s.iris_->SetSize({ irisScale, irisScale });
		s.iris_->Update();

		// アイリスが開き終わったらカメラ演出へ進む
		if (s.irisTween_.Finished()) {
			s.irisOpening_ = false;
			s.flowSM_.Change(std::make_unique<IntroCameraIntroState>());
		}
	}

	void IntroCameraIntroState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをカメラ演出に設定する
		s.phase_ = IntroSequence::Phase::CameraIntro;

		// カメラのYaw回転Tweenを開始する
		s.camYawTween_.Reset(s.camYawStart_, s.camYawEnd_, s.camIntroDuration_, Ease::Type::OutBack);
	}

	void IntroCameraIntroState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラがない場合は更新できないため終了する
		if (!camera) { return; }

		// YawをTweenで更新する
		float yawNow = s.camYawTween_.Update(IntroSequence::kFixedDt_);

		// Yawの進行率からPitchも補間する
		float denom = std::max(0.0001f, (s.camYawEnd_ - s.camYawStart_));
		float t01 = std::clamp((yawNow - s.camYawStart_) / denom, 0.0f, 1.0f);
		float pitchNow = MyMath::Lerp(s.camPitchStart_, s.camPitchEnd_, t01);

		// 補間した回転をカメラへ反映する
		camera->SetRotate({ pitchNow, yawNow, 0.0f });

		// カメラ演出が終わったらボス出現前演出へ進む
		if (s.camYawTween_.Finished()) {
			camera->SetRotate({ s.camPitchEnd_, s.camYawEnd_, 0.0f });
			s.flowSM_.Change(std::make_unique<IntroBossPreSpawnState>());
		}
	}

	void IntroBossPreSpawnState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラがない場合はボス出現前演出を開始できない
		if (!camera) { return; }

		// イントロの現在フェーズをボス出現前に設定する
		s.phase_ = IntroSequence::Phase::BossPreSpawn;

		// ボス出現前演出を開始する
		s.introBossActor_.BeginPreSpawn();

		// 現在のカメラ回転を保存して、ボス側へ向ける補間を始める
		s.camSavedRot_ = camera->GetRotate();
		s.camBossStartRot_ = s.camSavedRot_;
		s.camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };

		// ボス方向へカメラをブレンドする
		s.camBlendToBossActive_ = true;
		s.camBlendBackActive_ = false;
		s.camBlendToBossTween_.Reset(0.0f, 1.0f, s.camBlendToBossSec_, Ease::Type::InOutSine);

		// ボス出現位置にワープ予兆を出す
		const Vector3& pos = s.introBossActor_.GetPosition();
		ParticleManager::GetInstance()->Emit("bossWarp_core", pos, 4);
		ParticleManager::GetInstance()->Emit("bossWarp_swirl", pos, 18);
		ParticleManager::GetInstance()->Emit("bossWarp_dust", pos, 8);
	}

	void IntroBossPreSpawnState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラがない場合は更新できないため終了する
		if (!camera) { return; }

		// ボス出現前演出を更新する
		s.introBossActor_.UpdatePreSpawn(IntroSequence::kFixedDt_);

		// 出現前演出の進行率を作る
		float t = std::clamp(s.introBossActor_.GetPreSpawnElapsed() / 1.8f, 0.0f, 1.0f);

		// Actor側で持っている発生蓄積時間を取得する
		float emitAccum = s.introBossActor_.GetPreSpawnEmitAccum();

		// 一定間隔でワープ演出を発生させる
		while (emitAccum >= 0.08f) {
			emitAccum -= 0.08f;

			// 序盤は軽めの渦と砂煙
			if (t < 0.45f) {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 6);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossActor_.GetPosition(), 3);
			}
			// 中盤は密度を上げてコアも混ぜる
			else if (t < 0.80f) {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 12);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossActor_.GetPosition(), 6);
				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 2);
			}
			// 終盤は出現直前としてさらに密度を上げる
			else {
				ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 16);
				ParticleManager::GetInstance()->Emit("bossWarp_dust", s.introBossActor_.GetPosition(), 8);
				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 4);
			}
		}

		// 減算後の蓄積時間をActor側へ戻す
		s.introBossActor_.SetPreSpawnEmitAccum(emitAccum);

		// 出現直前の大きめワープ演出を一度だけ出す
		if (!s.introBossActor_.IsSpawnFxFinished() && t >= 0.82f) {
			s.introBossActor_.SetSpawnFxFinished(true);
			ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 20);
			ParticleManager::GetInstance()->Emit("bossWarp_swirl", s.introBossActor_.GetPosition(), 40);
		}

		// 出現前演出が終わったら実体のボスを生成する
		if (s.introBossActor_.IsPreSpawnFinished()) {
			if (!s.introBossActor_.Exists()) {
				s.introBossActor_.Spawn(camera);

				// フェーズをボス登場へ変更する
				s.phase_ = IntroSequence::Phase::BossAppear;

				// ボス出現中のカメラ目標回転を設定する
				s.camBossTargetRot_ = { 0.10f, -0.10f, 0.0f };
				s.camBlendBackActive_ = false;

				// 出現時の強めワープ演出を出す
				ParticleManager::GetInstance()->Emit("bossWarp_core", s.introBossActor_.GetPosition(), 12);

				s.flowSM_.Change(std::make_unique<IntroBossAppearState>());
			}
		}
	}

	void IntroBossAppearState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをボス登場に設定する
		s.phase_ = IntroSequence::Phase::BossAppear;

		// ボス登場演出を開始する
		s.introBossActor_.BeginAppear();
	}

	void IntroBossAppearState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラまたはボス実体がない場合は更新できないため終了する
		if (!camera || !s.introBossActor_.Exists()) { return; }

		// ボス登場演出の現在進行率を取得する
		float t = s.introBossActor_.GetAppearRatio();

		// ボス登場演出を更新し、完了したかを受け取る
		bool finished = s.introBossActor_.UpdateAppear(IntroSequence::kFixedDt_);

		// 登場に合わせてカメラ目標回転を少し正面寄りに変える
		s.camBossTargetRot_ = {
			MyMath::Lerp(0.10f, 0.06f, t),
			MyMath::Lerp(-0.10f, 0.0f, t),
			0.0f
		};

		// 登場演出が終わったら少し見せる停止演出へ進む
		if (finished) {
			s.phase_ = IntroSequence::Phase::BossPause;
			s.flowSM_.Change(std::make_unique<IntroBossPauseState>());
		}
	}

	void IntroBossPauseState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをボス停止に設定する
		s.phase_ = IntroSequence::Phase::BossPause;

		// ボス停止演出を開始する
		s.introBossActor_.BeginPause();
	}

	void IntroBossPauseState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラまたはボス実体がない場合は更新できないため終了する
		if (!camera || !s.introBossActor_.Exists()) { return; }

		// ボス停止演出を更新する
		bool finished = s.introBossActor_.UpdatePause(IntroSequence::kFixedDt_);

		// 停止中はボスを見せる角度に固定する
		s.camBossTargetRot_ = { 0.055f, 0.0f, 0.0f };

		// 停止が終わったら驚きホップへ進む
		if (finished) {
			s.phase_ = IntroSequence::Phase::BossNoticeHop;
			s.flowSM_.Change(std::make_unique<IntroBossNoticeHopState>());
		}
	}

	void IntroBossNoticeHopState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをボス驚きホップに設定する
		s.phase_ = IntroSequence::Phase::BossNoticeHop;

		// ボス驚きホップ演出を開始する
		s.introBossActor_.BeginNoticeHop();
	}

	void IntroBossNoticeHopState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラまたはボス実体がない場合は更新できないため終了する
		if (!camera || !s.introBossActor_.Exists()) { return; }

		// 驚きホップ演出の進行率を取得する
		float t = s.introBossActor_.GetNoticeHopRatio();

		// 一定進行後に注意マークと驚きSEを一度だけ出す
		if (!s.introBossActor_.IsNoticeMarkEmitted() && t >= 0.20f) {
			s.introBossActor_.SetNoticeMarkEmitted(true);

			// 注意マークの中心位置をボス周辺に作る
			const Vector3 center = s.introBossActor_.GetBasePosition() + Vector3{ 0.0f, 3.2f, -3.0f };

			// 複数方向に注意マークを出して、驚いた感じを強調する
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -7.0f,  2.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 7.0f,  2.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ -9.0f,  0.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 9.0f,  0.0f, 0.0f }, 1);
			ParticleManager::GetInstance()->Emit("bossNoticeMark", center + Vector3{ 6.0f, -2.5f, 0.0f }, 1);

			AudioManager::GetInstance()->PlaySound("surprise", 0.4f);
		}

		// ボス驚きホップ演出を更新する
		bool finished = s.introBossActor_.UpdateNoticeHop(IntroSequence::kFixedDt_);

		// 驚き中は少しだけ角度をつけて見せる
		s.camBossTargetRot_ = {
			0.045f,
			0.015f,
			0.0f
		};

		// 驚きホップが終わったらパニック演出へ進む
		if (finished) {
			s.phase_ = IntroSequence::Phase::BossPanic;
			s.flowSM_.Change(std::make_unique<IntroBossPanicState>());
		}
	}

	void IntroBossPanicState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをボスパニックに設定する
		s.phase_ = IntroSequence::Phase::BossPanic;

		// ボスパニック演出を開始する
		s.introBossActor_.BeginPanic();
	}

	void IntroBossPanicState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラまたはボス実体がない場合は更新できないため終了する
		if (!camera || !s.introBossActor_.Exists()) { return; }

		// ボスパニック演出を更新する
		bool finished = s.introBossActor_.UpdatePanic(IntroSequence::kFixedDt_);

		// パニック中のカメラ角度
		s.camBossTargetRot_ = { 0.06f, 0.0f, 0.0f };

		// パニック演出が終わったら逃走演出へ進む
		if (finished) {
			s.phase_ = IntroSequence::Phase::BossEscape;
			s.flowSM_.Change(std::make_unique<IntroBossEscapeState>());
		}
	}

	void IntroBossEscapeState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをボス逃走に設定する
		s.phase_ = IntroSequence::Phase::BossEscape;

		// ボス逃走演出を開始する
		s.introBossActor_.BeginEscape();
	}

	void IntroBossEscapeState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);
		Camera* camera = s.currentCamera_;

		// カメラまたはボス実体がない場合は更新できないため終了する
		if (!camera || !s.introBossActor_.Exists()) { return; }

		// ボス逃走演出を更新する
		bool finished = s.introBossActor_.UpdateEscape(IntroSequence::kFixedDt_);

		// 逃走中のカメラ角度
		s.camBossTargetRot_ = { 0.05f, 0.0f, 0.0f };

		if (finished) {
			// 逃走ワープの締め演出は一度だけ出す
			if (!s.introBossActor_.IsEscapeWarpBurstEmitted()) {
				s.introBossActor_.SetEscapeWarpBurstEmitted(true);

				const Vector3 pos = s.introBossActor_.GetPosition();
				ParticleManager::GetInstance()->Emit("bossEscape_warpCore", pos, 10);
				ParticleManager::GetInstance()->Emit("bossEscape_warpSwirl", pos, 36);
				ParticleManager::GetInstance()->Emit("bossEscape_warpShred", pos, 20);
				ParticleManager::GetInstance()->Emit("bossEscape_warpRing", pos, 1);
			}

			// イントロ用ボスをリセットする
			s.introBossActor_.Reset();

			// カメラを通常位置へ戻すためのブレンドを開始する
			s.camReturnStartRot_ = camera->GetRotate();
			s.camBlendBackActive_ = true;
			s.camBlendToBossActive_ = false;
			s.camBlendBackTween_.Reset(0.0f, 1.0f, s.camBlendBackSec_, Ease::Type::InOutSine);

			// ゲーム開始表示へ進む
			s.phase_ = IntroSequence::Phase::ShowStart;
			s.flowSM_.Change(std::make_unique<IntroShowStartState>());
			return;
		}

		// 逃走中はワープ残像を継続的に出す
		const Vector3 pos = s.introBossActor_.GetPosition();
		ParticleManager::GetInstance()->Emit("bossEscape_warpSwirl", pos, 5);
		ParticleManager::GetInstance()->Emit("bossEscape_warpShred", pos, 3);
	}

	void IntroShowStartState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズをゲーム開始表示に設定する
		s.phase_ = IntroSequence::Phase::ShowStart;
	}

	void IntroShowStartState::Update(IStateContext& ctx, float dt) {
		(void)dt;
		auto& s = AsIntro_(ctx);

		// カメラが戻り終わったらSTART表示を開始する
		if (!s.camBlendBackActive_) {
			s.startBanner_.Start();
		}

		// START表示を更新する
		s.startBanner_.Update(IntroSequence::kFixedDt_);

		// START表示が終わったらイントロ完了
		if (s.startBanner_.IsFinished()) {

			// STARTバナー終了時に一瞬だけモーションブラーをかける
			if (s.postEffect_) {
				s.postEffect_->StartMotionBlurBurst(0.9f, 3.0f);
			}

			// イントロの現在フェーズを完了に設定する
			s.phase_ = IntroSequence::Phase::Done;

			// 敵初期化リクエストが必要なら外側へ通知する
			if (!s.currentEnemiesInitialized_ && s.currentOutRequestInitEnemies_) {
				*s.currentOutRequestInitEnemies_ = true;
			}

			// プレイヤー操作ロックを解除する
			s.gameplayLocked_ = false;
			// イントロ完了状態へ進む
			s.flowSM_.Change(std::make_unique<IntroDoneState>());
		}
	}

	void IntroDoneState::Enter(IStateContext& ctx) {
		auto& s = AsIntro_(ctx);

		// イントロの現在フェーズを完了に設定する
		s.phase_ = IntroSequence::Phase::Done;
	}

	void IntroDoneState::Update(IStateContext& ctx, float dt) {
		// 完了状態では更新処理を行わない
		(void)ctx;
		(void)dt;
	}

}