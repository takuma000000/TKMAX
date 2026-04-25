#include "ClearComedyStates.h"
#include "GameClearScene.h"
#include "ParticleManager.h"
#include "AudioManager.h"
#include <algorithm>

namespace {
	/// <summary>
	/// 共通のStateContextをGameClearSceneとして扱えるように変換します。
	/// </summary>
	/// <param name="ctx">ステートマシンから渡される共通コンテキスト</param>
	/// <returns>GameClearScene参照</returns>
	GameClearScene& AsClear_(TKM::IStateContext& ctx) {
		return static_cast<GameClearScene&>(ctx);
	}
}

//=====================================================
// WaitAfterClear
//=====================================================
void ClearComedyWaitAfterClearState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// クリア後の待機時間を初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyWaitAfterClearState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	// コミカル演出用のタイムスケールを更新する
	s.clearComedyTimeScale_.Update(s.dt_);

	// スロー演出を考慮した演出用deltaTimeを作る
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	// クリア後の待機時間を進める
	s.clearComedyTimer_ += comedyDt;

	// 少し間を置いてから敵キャラを出現させる
	if (s.clearComedyTimer_ >= 0.85f) {
		s.SpawnClearComedyActors_();
		s.clearComedySM_.Change(std::make_unique<ClearComedySpawnState>());
	}
}

//=====================================================
// Spawn
//=====================================================
void ClearComedySpawnState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// 出現直後の演出時間を初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedySpawnState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	// コミカル演出用のタイムスケールを更新する
	s.clearComedyTimeScale_.Update(s.dt_);

	// スロー演出を考慮した演出用deltaTimeを作る
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	// 出現後の経過時間を進める
	s.clearComedyTimer_ += comedyDt;

	// ボスは驚き演出を入れながら更新する
	if (s.clearComedyBoss_) {
		s.clearComedyBoss_->SetIntroPanic(true, 0.35f);
		s.clearComedyBoss_->Update(comedyDt);
	}

	// 雑魚Aを更新する
	if (s.clearComedyMobA_) {
		s.clearComedyMobA_->Update(comedyDt);
	}

	// 雑魚Bを更新する
	if (s.clearComedyMobB_) {
		s.clearComedyMobB_->Update(comedyDt);
	}

	// 出現して少し経ったら、全員が気づく演出へ進む
	if (s.clearComedyTimer_ >= 0.65f) {

		// 注意マークは一度だけ出す
		if (!s.clearComedyNoticeMarkPlayed_) {
			auto* pm = TKM::ParticleManager::GetInstance();

			// ボスの頭上に注意マークを出す
			if (s.clearComedyBoss_) {
				Vector3 p = s.clearComedyBoss_->GetWorldPosition() + Vector3{ 0.0f, 6.0f, 0.0f } + s.clearParticleGlobalOffset_;
				pm->Emit("bossNoticeMark", p, 1);
			}

			// 雑魚Aの頭上に注意マークを出す
			if (s.clearComedyMobA_) {
				Vector3 p = s.clearComedyMobA_->GetWorldPosition() + Vector3{ 0.0f, 3.0f, 0.0f };
				pm->Emit("bossNoticeMark", p, 1);
			}

			// 雑魚Bの頭上に注意マークを出す
			if (s.clearComedyMobB_) {
				Vector3 p = s.clearComedyMobB_->GetWorldPosition() + Vector3{ 0.0f, 3.0f, 0.0f };
				pm->Emit("bossNoticeMark", p, 1);
			}

			// 注意マークを出し終えたので再発生を防ぐ
			s.clearComedyNoticeMarkPlayed_ = true;
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedySlowNoticeState>());
	}
}

//=====================================================
// SlowNotice
//=====================================================
void ClearComedySlowNoticeState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// 気づき演出用の時間を初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedySlowNoticeState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	// コミカル演出用のタイムスケールを更新する
	s.clearComedyTimeScale_.Update(s.dt_);

	// スロー演出を考慮した演出用deltaTimeを作る
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	// 気づき演出の経過時間を進める
	s.clearComedyTimer_ += comedyDt;

	// ボスは強めのパニック状態で更新する
	if (s.clearComedyBoss_) {
		s.clearComedyBoss_->SetIntroPanic(true, 1.0f);
		s.clearComedyBoss_->Update(comedyDt);
	}

	// 雑魚Aを更新する
	if (s.clearComedyMobA_) {
		s.clearComedyMobA_->Update(comedyDt);
	}

	// 雑魚Bを更新する
	if (s.clearComedyMobB_) {
		s.clearComedyMobB_->Update(comedyDt);
	}

	// 気づき演出が終わったら逃走開始へ進む
	if (s.clearComedyTimer_ >= 0.75f) {
		// RunAwayの開始位置を、この瞬間の見た目位置で確定する
		if (s.clearComedyBoss_) {
			s.clearComedyBossRunStartPos_ = s.clearComedyBoss_->GetWorldPosition();
		}

		if (s.clearComedyMobA_) {
			s.clearComedyMobARunStartPos_ = s.clearComedyMobA_->GetWorldPosition();
		}

		if (s.clearComedyMobB_) {
			s.clearComedyMobBRunStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		// 転倒時のスロー演出リクエストをまだ出していない状態に戻す
		s.clearComedyFallSlowRequested_ = false;

		s.clearComedySM_.Change(std::make_unique<ClearComedyRunAwayState>());
	}
}

//=====================================================
// RunAway
//=====================================================
void ClearComedyRunAwayState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// 逃走演出用の時間を初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyRunAwayState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	// コミカル演出用のタイムスケールを更新する
	s.clearComedyTimeScale_.Update(s.dt_);

	// スロー演出を考慮した演出用deltaTimeを作る
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	// 逃走演出の経過時間を進める
	s.clearComedyTimer_ += comedyDt;

	// 雑魚側の逃走進行率
	float t = std::clamp(s.clearComedyTimer_ / 1.35f, 0.0f, 1.0f);

	// ボスは少し長めの時間で奥へ逃げる
	float bossMoveT = Ease::Eval(Ease::Type::InQuad, std::clamp(s.clearComedyTimer_ / 1.80f, 0.0f, 1.0f));

	// 雑魚は短めの時間で逃走・転倒位置へ動かす
	float mobMoveT = Ease::Eval(Ease::Type::InQuad, t);

	// ボスを逃走先へ移動させる
	if (s.clearComedyBoss_) {
		Vector3 pos = MyMath::Vector3Lerp(s.clearComedyBossRunStartPos_, s.clearComedyBossEscapePos_, bossMoveT);

		s.clearComedyBoss_->SetPosition(pos);
		s.clearComedyBoss_->SetRotate({ 0.0f, -0.9f, 0.0f });
		s.clearComedyBoss_->SetIntroPanic(true, 0.75f);
		s.clearComedyBoss_->SyncTransform();
		s.clearComedyBoss_->Update(comedyDt);
	}

	// 雑魚Aを逃走先へ移動させる
	if (s.clearComedyMobA_) {
		Vector3 pos = MyMath::Vector3Lerp(s.clearComedyMobARunStartPos_, s.clearComedyMobAEscapePos_, mobMoveT);

		s.clearComedyMobA_->SetPosition(pos);
		s.clearComedyMobA_->SetRotate({ 0.0f, -0.9f, 0.0f });
		s.clearComedyMobA_->SyncTransform();
		s.clearComedyMobA_->Update(comedyDt);
	}

	// 雑魚Bは逃げ切らず、転倒位置へ向かわせる
	if (s.clearComedyMobB_) {
		Vector3 pos = MyMath::Vector3Lerp(s.clearComedyMobBRunStartPos_, s.clearComedyMobBFallPos_, mobMoveT);

		s.clearComedyMobB_->SetPosition(pos);
		s.clearComedyMobB_->SetRotate({ 0.0f, -0.9f, 0.0f });
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);
	}

	// 逃走移動が終わったら、次の転倒演出に使う開始位置を保存する
	if (s.clearComedyTimer_ >= 1.35f) {
		if (s.clearComedyBoss_) {
			s.clearComedyBossRecoverStartPos_ = s.clearComedyBoss_->GetWorldPosition();
		}

		if (s.clearComedyMobA_) {
			s.clearComedyMobARecoverStartPos_ = s.clearComedyMobA_->GetWorldPosition();
		}

		if (s.clearComedyMobB_) {
			s.clearComedyMobBRecoverStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedyFallDownState>());
	}
}

//=====================================================
// FallDown
//=====================================================
void ClearComedyFallDownState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// 転倒演出用の時間を初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyFallDownState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	// コミカル演出用のタイムスケールを更新する
	s.clearComedyTimeScale_.Update(s.dt_);

	// スロー演出を考慮した演出用deltaTimeを作る
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	// 転倒演出の経過時間を進める
	s.clearComedyTimer_ += comedyDt;

	// 転倒演出の進行率
	float t = std::clamp(s.clearComedyTimer_ / 0.85f, 0.0f, 1.0f);

	// ボスは待たずにそのまま退場方向へ進む
	if (s.clearComedyBoss_) {
		float bossMoveT = Ease::Eval(Ease::Type::InQuad, t);

		Vector3 bossPos = MyMath::Vector3Lerp(
			s.clearComedyBossRecoverStartPos_,
			s.clearComedyBossExitPos_,
			bossMoveT
		);

		s.clearComedyBoss_->SetPosition(bossPos);
		s.clearComedyBoss_->SetRotate({ 0.0f, -1.00f, 0.0f });
		s.clearComedyBoss_->SetIntroPanic(false, 0.0f);
		s.clearComedyBoss_->SyncTransform();
		s.clearComedyBoss_->Update(comedyDt);
	}

	// 雑魚Aも待たずにそのまま退場方向へ進む
	if (s.clearComedyMobA_) {
		float mobAMoveT = Ease::Eval(Ease::Type::InQuad, t);

		Vector3 mobAPos = MyMath::Vector3Lerp(
			s.clearComedyMobARecoverStartPos_,
			s.clearComedyMobAExitPos_,
			mobAMoveT
		);

		s.clearComedyMobA_->SetPosition(mobAPos);
		s.clearComedyMobA_->SetRotate({ 0.0f, -1.00f, 0.0f });
		s.clearComedyMobA_->SyncTransform();
		s.clearComedyMobA_->Update(comedyDt);
	}

	// 雑魚Bだけ派手に転ぶ
	if (s.clearComedyMobB_) {
		Vector3 startPos = s.clearComedyMobBRecoverStartPos_;

		// まず一瞬浮くターゲット位置
		Vector3 popPos = startPos + Vector3{ 0.0f, 1.4f, 0.8f };

		// 最終的な転倒位置
		Vector3 slamPos = startPos + Vector3{ 0.0f, -1.8f, 2.8f };

		Vector3 pos{};
		Vector3 rot{};

		if (t < 0.35f) {
			// 転び始めの瞬間だけスローを入れる
			if (!s.clearComedyFallSlowRequested_) {
				s.clearComedyTimeScale_.RequestSlowAdvanced(0.20f, 1.7f, 0.05f, 0.25f);
				s.clearComedyFallSlowRequested_ = true;
			}

			// 滑り出しエフェクトは一度だけ発生させる
			if (!s.clearComedyMobBSlipEffectPlayed_) {
				Vector3 slipPos = startPos + Vector3{ 1.0f, 0.1f, 0.35f } + s.clearParticleGlobalOffset_;

				auto* pm = TKM::ParticleManager::GetInstance();
				pm->Emit("clearComedySlip_streak", slipPos, 8);
				pm->Emit("clearComedySlip_spark", slipPos, 10);
				pm->Emit("clearComedySlip_ring", slipPos, 2);
				pm->Emit("clearComedySlip_chip", slipPos, 8);

				s.clearComedyMobBSlipEffectPlayed_ = true;

				TKM::AudioManager::GetInstance()->PlaySound("slip", 0.3f);
			}

			// 前半：一瞬ふわっと浮く
			float u = t / 0.35f;
			float jumpT = Ease::Eval(Ease::Type::OutQuad, u);

			pos = MyMath::Vector3Lerp(startPos, popPos, jumpT);

			// 少し前のめりになりながら浮く
			rot.x = MyMath::Lerp(0.0f, -0.35f, jumpT);
			rot.y = MyMath::Lerp(-0.9f, -0.75f, jumpT);
			rot.z = MyMath::Lerp(0.0f, 0.35f, jumpT);
		} else {
			// 後半：ズコーーーーっと落ちる
			float u = (t - 0.35f) / 0.65f;
			float slamT = Ease::Eval(Ease::Type::InExpo, u);

			pos = MyMath::Vector3Lerp(popPos, slamPos, slamT);

			// 一気に横倒れする回転へ補間する
			rot.x = MyMath::Lerp(-0.35f, 0.15f, slamT);
			rot.y = MyMath::Lerp(-0.75f, s.clearComedyMobBFallRot_.y, slamT);
			rot.z = MyMath::Lerp(0.35f, 1.95f, slamT);
		}

		// 転倒中の位置・回転を雑魚Bへ反映する
		s.clearComedyMobB_->SetPosition(pos);
		s.clearComedyMobB_->SetRotate(rot);
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);

		// 着地時の転倒エフェクトは一度だけ発生させる
		if (!s.clearComedyMobBFallEffectPlayed_ && t >= 0.92f) {
			Vector3 fallFxPos = s.clearComedyMobB_->GetWorldPosition() + s.clearParticleGlobalOffset_;

			auto* pm = TKM::ParticleManager::GetInstance();
			pm->Emit("clearComedyFall_dust", fallFxPos, 10);
			pm->Emit("clearComedyFall_star", fallFxPos, 8);
			pm->Emit("clearComedyFall_line", fallFxPos, 8);
			pm->Emit("clearComedyFall_puff", fallFxPos, 6);

			s.clearComedyMobBFallEffectPlayed_ = true;
			TKM::AudioManager::GetInstance()->PlaySound("comedy", 0.35f);
		}
	}

	// 転倒演出が終わったら、ボスと雑魚Aは消して、雑魚Bだけ起き上がり演出へ進める
	if (s.clearComedyTimer_ >= 0.85f) {
		s.clearComedyBoss_.reset();
		s.clearComedyMobA_.reset();

		if (s.clearComedyMobB_) {
			s.clearComedyMobBRecoverStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedyStandUpState>());
	}
}

//=====================================================
// StandUp
//=====================================================
void ClearComedyStandUpState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// 起き上がり演出用の時間を初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyStandUpState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	// コミカル演出用のタイムスケールを更新する
	s.clearComedyTimeScale_.Update(s.dt_);

	// スロー演出を考慮した演出用deltaTimeを作る
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	// 起き上がり演出の経過時間を進める
	s.clearComedyTimer_ += comedyDt;

	// OutBackで少し反動をつけながら起き上がる
	float t = std::clamp(s.clearComedyTimer_ / 1.0f, 0.0f, 1.0f);
	float standT = Ease::Eval(Ease::Type::OutBack, t);

	if (s.clearComedyMobB_) {
		// 位置は動かさない。その場で起き上がる
		s.clearComedyMobB_->SetPosition(s.clearComedyMobBRecoverStartPos_);

		// 横倒れ状態から通常姿勢へ戻す
		Vector3 rot = {
			0.0f,
			MyMath::Lerp(s.clearComedyMobBFallRot_.y, -1.05f, standT),
			MyMath::Lerp(1.95f, 0.0f, standT)
		};

		s.clearComedyMobB_->SetRotate(rot);
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);
	}

	// 起き上がり終わったら、そこを逃走開始位置にする
	if (s.clearComedyTimer_ >= 1.0f) {
		if (s.clearComedyMobB_) {
			s.clearComedyMobBRecoverStartPos_ = s.clearComedyMobB_->GetWorldPosition();
		}

		s.clearComedySM_.Change(std::make_unique<ClearComedyRecoverRunState>());
	}
}

//=====================================================
// RecoverRun
//=====================================================
void ClearComedyRecoverRunState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// 起き上がり後の逃走演出用時間を初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyRecoverRunState::Update(TKM::IStateContext& ctx, float dt) {
	(void)dt;
	auto& s = AsClear_(ctx);

	// コミカル演出用のタイムスケールを更新する
	s.clearComedyTimeScale_.Update(s.dt_);

	// スロー演出を考慮した演出用deltaTimeを作る
	const float comedyDt = s.dt_ * s.clearComedyTimeScale_.GetScale();

	// 起き上がり後の逃走時間を進める
	s.clearComedyTimer_ += comedyDt;

	// 逃げるほど加速する動きにする
	float t = std::clamp(s.clearComedyTimer_ / 1.00f, 0.0f, 1.0f);
	float moveT = Ease::Eval(Ease::Type::InCubic, t);

	// 起き上がった後に雑魚Bを逃走させる
	if (s.clearComedyMobB_) {
		Vector3 pos = MyMath::Vector3Lerp(
			s.clearComedyMobBRecoverStartPos_,
			s.clearComedyMobBExitPos_,
			moveT
		);

		s.clearComedyMobB_->SetPosition(pos);
		s.clearComedyMobB_->SetRotate({ 0.0f, -1.05f, 0.0f });
		s.clearComedyMobB_->SyncTransform();
		s.clearComedyMobB_->Update(comedyDt);
	}

	// 雑魚Bが逃げ切ったら、GAME CLEAR表示とメニュー表示へ進める
	if (s.clearComedyTimer_ >= 1.00f) {
		// 最後に残っていた雑魚Bを消す
		s.clearComedyMobB_.reset();

		// 次回の二重生成防止フラグを戻す
		s.clearComedyActorsSpawned_ = false;

		{
			auto* pm = TKM::ParticleManager::GetInstance();

			// GAME CLEAR表示位置付近でバーストを出す
			Vector3 burstPos = s.playerDisplayPos_ + s.clearBannerBurstOffset_;

			pm->Emit("clearBannerBurst_core", burstPos, 6);
			pm->Emit("clearBannerBurst_confetti", burstPos, 70);
			pm->Emit("clearBannerBurst_ray", burstPos, 30);

			// GAME CLEAR表示SEを鳴らす
			TKM::AudioManager::GetInstance()->PlaySound("clear_display", 0.3f);
		}

		// GAME CLEARスプライトとメニュー表示を開始する
		s.isClearSpriteVisible_ = true;
		s.isClearMenuVisible_ = true;
		s.isClearSpritePopPlaying_ = true;
		s.clearSpritePopTime_ = 0.0f;

		// スプライト表示演出は開始位置から始める
		s.clearSprite_->SetPosition(s.clearSpriteStartPos_);

		// ライブ風ファイアー柱を開始する
		s.clearStageFireActive_ = true;
		s.clearStageFireTimer_ = 0.0f;

		s.clearComedySM_.Change(std::make_unique<ClearComedyDoneState>());
	}
}

//=====================================================
// Done
//=====================================================
void ClearComedyDoneState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsClear_(ctx);

	// 完了状態用のタイマーを初期化する
	s.clearComedyTimer_ = 0.0f;
}

void ClearComedyDoneState::Update(TKM::IStateContext& ctx, float dt) {
	// 完了状態では更新処理を行わない
	(void)ctx;
	(void)dt;
}