#include "BossEntranceStates.h"
#include "BossEntranceSequence.h"
#include "manager/BossManager.h"
#include "ParticleManager.h"
#include <algorithm>

namespace {
	/// <summary>
	/// 共通のStateContextをBossEntranceSequenceとして扱えるように変換します。
	/// </summary>
	/// <param name="ctx">ステートマシンから渡される共通コンテキスト</param>
	/// <returns>BossEntranceSequence参照</returns>
	BossEntranceSequence& AsBossEntrance_(TKM::IStateContext& ctx) {
		return static_cast<BossEntranceSequence&>(ctx);
	}
}

//=====================================================
// Wait
//=====================================================
void BossEntranceWaitState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);

	// 待機フェーズ用のタイマーを初期化する
	s.phaseTimer_ = 0.0f;

	// 待機中は通常時の空色に戻しておく
	s.currentSkyColor_ = s.kBaseSkyColor_;
}

void BossEntranceWaitState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);

	// 待機時間を進める
	s.phaseTimer_ += dt;

	// 待機中は通常時の空色を維持する
	s.currentSkyColor_ = s.kBaseSkyColor_;

	// 指定時間待ったら、空を赤く変化させるフェーズへ進む
	if (s.phaseTimer_ >= s.kWaitSec_) {
		s.sm_.Change(std::make_unique<BossEntranceSkyFadeInState>());
	}
}

//=====================================================
// SkyFadeIn
//=====================================================
void BossEntranceSkyFadeInState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);

	// 空色フェード用のタイマーを初期化する
	s.phaseTimer_ = 0.0f;
}

void BossEntranceSkyFadeInState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);

	// フェード時間を進める
	s.phaseTimer_ += dt;

	// フェード進行率を0.0f～1.0fに収める
	const float t = std::clamp(s.phaseTimer_ / s.kSkyFadeInSec_, 0.0f, 1.0f);

	// 通常の空色から赤い空色へ補間する
	s.currentSkyColor_ = BossEntranceSequence::LerpColor_(s.kBaseSkyColor_, s.kRedSkyColor_, t);

	// 空色が赤くなり切ったら、収束演出フェーズへ進む
	if (s.phaseTimer_ >= s.kSkyFadeInSec_) {
		// 収束パーティクル用タイマーを初期化する
		s.convergeEmitTimer_ = 0.0f;

		// リング発生用タイマーを初期化する
		s.glowEmitTimer_ = 0.0f;

		// 念のため完全な赤空に固定する
		s.currentSkyColor_ = s.kRedSkyColor_;

		s.sm_.Change(std::make_unique<BossEntranceGatherState>());
	}
}

//=====================================================
// Gather
//=====================================================
void BossEntranceGatherState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);

	// 収束フェーズ用のタイマーを初期化する
	s.phaseTimer_ = 0.0f;

	// 収束パーティクル発生タイマーを初期化する
	s.convergeEmitTimer_ = 0.0f;

	// リング発生タイマーを初期化する
	s.glowEmitTimer_ = 0.0f;

	// 収束中は赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceGatherState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);

	// 収束フェーズの経過時間を進める
	s.phaseTimer_ += dt;

	// 収束中は赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;

	// 各パーティクルの発生タイマーを進める
	s.convergeEmitTimer_ += dt;
	s.glowEmitTimer_ += dt;

	// 収束パーティクルを一定間隔で発生させる
	while (s.convergeEmitTimer_ >= s.kGatherEmitInterval_) {
		s.convergeEmitTimer_ -= s.kGatherEmitInterval_;
		s.EmitGather_();
	}

	// 薄いリングを一定間隔で発生させる
	while (s.glowEmitTimer_ >= s.kRingEmitInterval_) {
		s.glowEmitTimer_ -= s.kRingEmitInterval_;

		auto* pm = TKM::ParticleManager::GetInstance();

		// パーティクルマネージャが取得できた場合のみ発生させる
		if (pm) {
			pm->Emit("bossEntrance_ringThin", s.spawnPos_, 1);
		}
	}

	// 収束時間が終わったら、ボスを覆う演出へ進む
	if (s.phaseTimer_ >= s.kGatherSec_) {
		s.sm_.Change(std::make_unique<BossEntranceCoverState>());
	}
}

//=====================================================
// Cover
//=====================================================
void BossEntranceCoverState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);

	// カバー演出用タイマーを初期化する
	s.phaseTimer_ = 0.0f;

	// カバー用パーティクル発生タイマーを初期化する
	s.coverEmitTimer_ = 0.0f;

	// カバー演出の発生回数を初期化する
	s.coverEmitCount_ = 0;

	// カバー演出中も赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceCoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);

	// カバー演出の経過時間を進める
	s.phaseTimer_ += dt;

	// カバー演出中も赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;

	// カバー用パーティクルの発生タイマーを進める
	s.coverEmitTimer_ += dt;

	// カバー用パーティクルを一定間隔で発生させる
	while (s.coverEmitTimer_ >= s.kCoverEmitInterval_) {
		s.coverEmitTimer_ -= s.kCoverEmitInterval_;

		// ボス出現位置を覆うような演出を発生させる
		s.EmitCover_();

		// カバー演出の発生回数を数える
		++s.coverEmitCount_;

		// 一定回数カバー演出を出したタイミングでボスを生成する
		if (!s.bossSpawned_ && s.coverEmitCount_ >= s.kCoverSpawnEmitCount_) {
			if (s.bossManager_) {
				// 登場演出用としてボスを生成する
				s.bossManager_->SpawnForEntrance();

				if (BossEnemy* boss = s.bossManager_->GetBoss()) {
					// バースト演出開始時の位置は、最終位置より少し上・奥に置く
					s.burstStartPos_ = s.spawnPos_ + Vector3{ 0.0f, 2.2f, -10.0f };

					// バースト演出終了時は本来の出現位置にする
					s.burstEndPos_ = s.spawnPos_;

					// 最初は小さめにして、バースト中に大きく見せる
					s.burstStartScale_ = { 1.6f, 1.6f, 1.6f };
					s.burstEndScale_ = { 5.0f, 5.0f, 5.0f };

					// ボスをバースト開始位置・開始スケールに配置する
					boss->SetPosition(s.burstStartPos_);
					boss->SetScale(s.burstStartScale_);

					// 変更したTransformを反映する
					boss->SyncTransform();
				}
			}

			// ボス生成済みとして扱い、二重生成を防ぐ
			s.bossSpawned_ = true;
		}
	}

	// カバー演出が終わったら、ボスを一気に見せるバースト演出へ進む
	if (s.phaseTimer_ >= s.kCoverSec_) {
		s.burstFxEmitted_ = false;
		s.sm_.Change(std::make_unique<BossEntranceBurstState>());
	}
}

//=====================================================
// Burst
//=====================================================
void BossEntranceBurstState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);

	// バースト演出用タイマーを初期化する
	s.phaseTimer_ = 0.0f;

	// バーストエフェクトは一度だけ出すため、未発生状態に戻す
	s.burstFxEmitted_ = false;

	// バースト中も赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceBurstState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);

	// バースト演出の経過時間を進める
	s.phaseTimer_ += dt;

	// バースト中も赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;

	// バーストエフェクトはフェーズ開始後に一度だけ発生させる
	if (!s.burstFxEmitted_) {
		s.EmitBurst_();
		s.burstFxEmitted_ = true;
	}

	if (s.bossManager_) {
		if (BossEnemy* boss = s.bossManager_->GetBoss()) {
			// バースト演出の進行率を0.0f～1.0fに収める
			float t = std::clamp(s.phaseTimer_ / s.kBurstSec_, 0.0f, 1.0f);

			// 終盤に向かって勢いよく到達するイージング
			float e = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

			Vector3 pos{};

			// 開始位置から最終出現位置へ補間する
			pos.x = MyMath::Lerp(s.burstStartPos_.x, s.burstEndPos_.x, e);
			pos.y = MyMath::Lerp(s.burstStartPos_.y, s.burstEndPos_.y, e);
			pos.z = MyMath::Lerp(s.burstStartPos_.z, s.burstEndPos_.z, e);

			Vector3 scale{};

			// 開始スケールから最終スケールへ補間する
			scale.x = MyMath::Lerp(s.burstStartScale_.x, s.burstEndScale_.x, e);
			scale.y = MyMath::Lerp(s.burstStartScale_.y, s.burstEndScale_.y, e);
			scale.z = MyMath::Lerp(s.burstStartScale_.z, s.burstEndScale_.z, e);

			// 補間した位置・スケールをボスへ反映する
			boss->SetPosition(pos);
			boss->SetScale(scale);

			// Transformを反映する
			boss->SyncTransform();
		}
	}

	// バースト演出が終わったら、ボス戦開始と押し出し演出へ進む
	if (s.phaseTimer_ >= s.kBurstSec_) {
		if (s.bossManager_) {
			if (BossEnemy* boss = s.bossManager_->GetBoss()) {
				// 最後に誤差が残らないよう、最終位置・最終スケールへ固定する
				boss->SetPosition(s.burstEndPos_);
				boss->SetScale(s.burstEndScale_);
				boss->SyncTransform();
			}

			// ボスの通常戦闘処理を開始する
			s.bossManager_->BeginBattle();
		}

		// 押し出し波用の発生タイマーを初期化する
		s.holdEmitTimer_ = 0.0f;

		s.sm_.Change(std::make_unique<BossEntrancePushState>());
	}
}

//=====================================================
// Push
//=====================================================
void BossEntrancePushState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);

	// 押し出し演出用タイマーを初期化する
	s.phaseTimer_ = 0.0f;

	// 押し出し波の発生タイマーを初期化する
	s.holdEmitTimer_ = 0.0f;

	// 押し出し演出中も赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntrancePushState::Update(TKM::IStateContext& ctx, float dt) {
	auto& s = AsBossEntrance_(ctx);

	// 押し出し演出の経過時間を進める
	s.phaseTimer_ += dt;

	// 押し出し演出中も赤い空を維持する
	s.currentSkyColor_ = s.kRedSkyColor_;

	// 押し出し波の発生タイマーを進める
	s.holdEmitTimer_ += dt;

	// 一定間隔で押し出し波を発生させる
	while (s.holdEmitTimer_ >= s.kPushEmitInterval_) {
		s.holdEmitTimer_ -= s.kPushEmitInterval_;
		s.EmitPushWave_();
	}

	// 押し出し演出が終わったら登場演出完了へ進む
	if (s.phaseTimer_ >= s.kPushSec_) {
		s.sm_.Change(std::make_unique<BossEntranceDoneState>());
	}
}

//=====================================================
// Done
//=====================================================
void BossEntranceDoneState::Enter(TKM::IStateContext& ctx) {
	auto& s = AsBossEntrance_(ctx);

	// 完了状態用にタイマーを初期化する
	s.phaseTimer_ = 0.0f;

	// 登場演出全体を非アクティブにする
	s.isActive_ = false;

	// 登場演出後の空色は赤い状態で維持する
	s.currentSkyColor_ = s.kRedSkyColor_;
}

void BossEntranceDoneState::Update(TKM::IStateContext& ctx, float dt) {
	// 完了状態では更新処理を行わない
	(void)ctx;
	(void)dt;
}