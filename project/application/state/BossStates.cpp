#include "BossStates.h"
#include "BossController.h"
#include <cmath>
#include <algorithm>

static BossController& AsBoss_(TKM::IStateContext& ctx) {
	return static_cast<BossController&>(ctx);
}

//=====================================================
// Enter
//=====================================================
void BossEnterState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);
	c.timer_ = 0.0f;
	c.state_ = BossController::State::Enter;
}

void BossEnterState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;
	Vector3& pos = c.posWork_;

	const float targetZ_ = c.orbitZ_;
	const float speed_ = 18.0f;

	pos.z = BossController::Approach(pos.z, targetZ_, speed_ * dt);
	pos.x = BossController::Approach(pos.x, 0.0f, 10.0f * dt);
	pos.y = BossController::Approach(pos.y, c.orbitY_, 10.0f * dt);

	if (std::abs(pos.z - targetZ_) < 0.05f) {
		c.ChangeState(BossController::State::Orbit);
	}
}

//=====================================================
// Orbit
//=====================================================
void BossOrbitState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);
	c.timer_ = 0.0f;
	c.state_ = BossController::State::Orbit;
}

void BossOrbitState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;
	Vector3& pos = c.posWork_;
	const Vector3& playerPos = c.playerPos_;

	float t_ = c.timer_;
	float angle_ = t_ * c.orbitAngularSpeed_;
	float ox_ = std::cos(angle_) * c.orbitRadiusX_;
	float oy_ = std::sin(angle_ * 0.9f) * c.orbitRadiusY_;

	Vector3 target_;
	target_.x = playerPos.x * c.orbitPlayerInfluence_ + ox_;
	target_.y = c.orbitY_ + playerPos.y * 0.2f + oy_;
	target_.z = c.orbitZ_;

	pos = BossController::SmoothDamp(pos, target_, c.orbitFollow_, dt);

	if (c.timer_ < c.orbitDuration_) { return; }

	// -----------------------------
	// 通常時：ミサイル or スラッシュ
	// 怒り中：レーザー
	// -----------------------------
	if (!c.rageActive_) {

		const bool canSlash_ = (c.slashCooldownT_ <= 0.0f);
		std::uniform_real_distribution<float> u01(0.0f, 1.0f);

		const float slashRate_ = canSlash_ ? 0.45f : 0.0f;
		const bool doSlash_ = (u01(c.rng_) < slashRate_);

		if (doSlash_) {
			// スラッシュ（溜め→発射）
			c.slashCharging_ = true;
			c.slashChargeTimer_ = c.slashChargeTime_;
			c.slashChargeFrame_ = 0;

			if (boss.GetPlayer()) {
				c.slashTargetSnap_ = boss.GetPlayer()();
				c.slashTargetValid_ = true;
			} else {
				c.slashTargetSnap_ = playerPos;
				c.slashTargetValid_ = true;
			}

			c.slashCooldownT_ = c.slashCooldown_;

			// --- スラッシュ選択時：ミサイル系は完全に止める（混在防止）---
			c.burstLeft_ = 0;
			c.burstTimer_ = 0.0f;
			c.burstTargetValid_ = false;
			c.burstCharged_ = false;
			c.missileCharging_ = false;
			c.missileChargeTimer_ = 0.0f;
			c.missileChargeFrame_ = 0;

			c.ChangeState(BossController::State::Recover);
			return;
		}

		// --- ミサイル選択時：スラッシュ系は完全に止める（混在防止）---
		c.slashCharging_ = false;
		c.slashChargeTimer_ = 0.0f;
		c.slashChargeFrame_ = 0;
		c.slashFireReq_ = false;
		// ミサイル（1回溜め→3連射）
		c.burstLeft_ = 3;
		c.burstTimer_ = 0.0f;
		c.burstTargetValid_ = false;
		c.burstCharged_ = false;

		c.missileCharging_ = true;
		c.missileChargeTimer_ = c.missileChargeTime_;
		c.missileChargeFrame_ = 0;

		if (boss.GetPlayer()) {
			c.burstTargetSnap_ = boss.GetPlayer()();
			c.burstTargetValid_ = true;
		} else {
			c.burstTargetSnap_ = playerPos;
			c.burstTargetValid_ = true;
		}

		c.ChangeState(BossController::State::Recover);
		return;
	}

	// 怒り中のみレーザーへ
	c.laserAimFixed_ = playerPos + c.playerVel_ * c.predictLeadTime_;
	c.laserAimFixed_.x = std::clamp(c.laserAimFixed_.x, c.arenaMin_.x, c.arenaMax_.x);
	c.laserAimFixed_.y = std::clamp(c.laserAimFixed_.y, c.arenaMin_.y, c.arenaMax_.y);
	c.laserAimFixed_.z = std::clamp(c.laserAimFixed_.z, c.arenaMin_.z, c.arenaMax_.z);

	c.laserBasePos_ = pos;
	c.ChangeState(BossController::State::LaserWindup);
}

//=====================================================
// Recover（ここに Burst / Slash charge を移植）
//=====================================================
void BossRecoverState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);
	c.timer_ = 0.0f;
	c.state_ = BossController::State::Recover;
}

void BossRecoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;
	Vector3& pos = c.posWork_;

	// 目標：Orbitの高さとZへ戻す
	Vector3 target_{ pos.x, c.orbitY_, c.orbitZ_ };
	pos = BossController::SmoothDamp(pos, target_, 0.18f, dt);

	// ============================================================
	// Missile Burst Execute（Recover中のみ）
	// ============================================================
	if (c.missileCharging_ || c.burstLeft_ > 0 || (c.burstCharged_ && c.burstTimer_ > 0.0f)) {

		Vector3 muzzlePos_ = boss.GetWorldPosition();
		muzzlePos_.y += c.missileMuzzleYOffset_;
		c.missilePos_ = muzzlePos_;

		Vector3 target_ = c.burstTargetValid_ ? c.burstTargetSnap_ : c.playerPos_;
		c.missileTarget_ = target_;

		if (!c.burstCharged_) {
			auto* pm_ = TKM::ParticleManager::GetInstance();
			if (pm_) {
				Vector3 p_ = muzzlePos_;

				float t = 1.0f - (c.missileChargeTimer_ / c.missileChargeTime_);
				t = std::clamp(t, 0.0f, 1.0f);

				int inwardCount_ = 2 + (int)(t * 7);
				int crackleCount_ = 1 + (int)(t * 3);
				pm_->Emit("boss_windup_inward", p_, inwardCount_);
				pm_->Emit("boss_windup_crackle", p_, crackleCount_);

				int step_ = (t < 0.55f) ? 4 : 2;
				if ((c.missileChargeFrame_ % step_) == 0) {
					pm_->Emit("boss_windup_shell", p_, 1);
				}
			}
			++c.missileChargeFrame_;

			c.missileChargeTimer_ -= dt;
			if (c.missileChargeTimer_ <= 0.0f) {
				c.burstCharged_ = true;
				c.missileCharging_ = false;

				// 1発目即発射
				c.missileFireReq_ = true;
				c.burstLeft_--;
				c.burstTimer_ = c.burstInterval_;
			}
		} else {
			c.burstTimer_ -= dt;

			if (c.burstTimer_ <= 0.0f) {
				// まだ撃つ弾が残ってる時だけ発射
				if (c.burstLeft_ > 0) {
					c.missileFireReq_ = true;
					c.burstLeft_--;
					c.burstTimer_ = c.burstInterval_;
				} else {
					// 撃ち終わってる：タイマーだけ終わらせる（固まり防止）
					c.burstTimer_ = 0.0f;
				}
			}
		}
	}
	// ミサイルが完全に終わったら状態をクリア（Recover抜け用）
	if (c.burstCharged_ && c.burstLeft_ <= 0 && c.burstTimer_ <= 0.0f) {
		c.burstCharged_ = false;
		c.burstTargetValid_ = false;
		c.missileCharging_ = false;
	}

	// ============================================================
	// Slash Charge / Fire Execute（Recover中のみ）
	// ============================================================
	if (c.slashCooldownT_ > 0.0f) {
		c.slashCooldownT_ = std::max(0.0f, c.slashCooldownT_ - dt);
	}

	if (c.slashCharging_) {
		Vector3 p_ = boss.GetWorldPosition();
		p_.y += c.missileMuzzleYOffset_;
		c.slashPos_ = p_;

		if (boss.GetPlayer()) {
			c.slashTarget_ = boss.GetPlayer()();
		} else {
			c.slashTarget_ = c.playerPos_;
		}

		if (auto* pm_ = TKM::ParticleManager::GetInstance()) {
			float t = 1.0f - (c.slashChargeTimer_ / c.slashChargeTime_);
			t = std::clamp(t, 0.0f, 1.0f);

			int line_ = 2 + (int)(t * 8);
			int spark_ = 1 + (int)(t * 4);
			pm_->Emit("boss_slash_windup_line", p_, line_);
			pm_->Emit("boss_slash_windup_spark", p_, spark_);

			if ((c.slashChargeFrame_ % 3) == 0) {
				pm_->Emit("boss_slash_windup_arc", p_, 1);
			}
		}

		++c.slashChargeFrame_;
		c.slashChargeTimer_ -= dt;

		if (c.slashChargeTimer_ <= 0.0f) {
			c.slashCharging_ = false;
			c.slashFireReq_ = true;
			c.slashCooldownT_ = c.slashCooldown_;
		}
	}

	// ------------------------------
	// 攻撃が終わるまでOrbitへ戻さない
	//   - ミサイル：3連射が終わるまで
	//   - スラッシュ：溜め完了（発射要求発行）まで
	// ------------------------------
	const bool missileBusy_ =
		(c.missileCharging_) ||
		(c.burstLeft_ > 0) ||
		(c.burstCharged_ && c.burstTimer_ > 0.0f); // 連射の待ちが残ってる

	const bool slashBusy_ =
		(c.slashCharging_); // まだ溜め中

	// （安全）ミサイルが終わったら後始末しておく
	if (!missileBusy_) {
		c.burstCharged_ = false;
		c.burstTimer_ = 0.0f;
	}

	if (c.timer_ >= c.recoverDuration_ && !missileBusy_ && !slashBusy_) {
		c.ChangeState(BossController::State::Orbit);
	}
}

//=====================================================
// LaserWindup
//=====================================================
void BossLaserWindupState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);
	c.timer_ = 0.0f;
	c.state_ = BossController::State::LaserWindup;

	// 初回処理をEnterに寄せる（元の timer_<=dt 相当）
	c.laserActive_ = true;
	c.laserTelegraph_ = true;
	c.laserBasePos_ = c.posWork_;

	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.laserMuzzleYOffset_, 0.0f };
	c.laserEndWS_ = c.laserAimFixed_;
}

void BossLaserWindupState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);

	if (!c.rageActive_) {
		c.laserActive_ = false;
		c.laserTelegraph_ = false;
		c.ChangeState(BossController::State::Recover);
		return;
	}

	Vector3& pos = c.posWork_;

	pos = c.laserBasePos_;

	float t_ = (c.laserWindup_ > 0.0001f) ? (c.timer_ / c.laserWindup_) : 1.0f;
	t_ = std::clamp(t_, 0.0f, 1.0f);
	float ramp_ = t_ * t_;
	float amp_ = 0.18f * (0.2f + 0.8f * ramp_);

	pos.x += std::sin(c.timer_ * 60.0f) * amp_;
	pos.y += std::sin(c.timer_ * 87.0f + 1.7f) * (amp_ * 0.55f);

	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.laserMuzzleYOffset_, 0.0f };
	c.laserEndWS_ = c.laserAimFixed_;

	if (c.timer_ >= c.laserWindup_) {
		c.ChangeState(BossController::State::LaserFire);
	}
}

//=====================================================
// LaserFire
//=====================================================
void BossLaserFireState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);
	c.timer_ = 0.0f;
	c.state_ = BossController::State::LaserFire;

	c.laserActive_ = true;
	c.laserTelegraph_ = false;

	// pos固定基準は windup で作った laserBasePos_ を使う
	c.posWork_ = c.laserBasePos_;
}

void BossLaserFireState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Enemy& boss = *c.boss_;

	if (!c.rageActive_) {
		c.laserActive_ = false;
		c.laserTelegraph_ = false;
		c.ChangeState(BossController::State::Recover);
		return;
	}

	c.laserActive_ = true;
	c.laserTelegraph_ = false;

	c.posWork_ = c.laserBasePos_;

	Vector3 aim_ = c.laserAimFixed_;
	if (boss.GetPlayer()) {
		Vector3 p_ = boss.GetPlayer()();
		Vector3 v_ = c.playerVel_;
		Vector3 pred_ = p_ + v_ * c.predictLeadTime_;

		pred_.x = std::clamp(pred_.x, c.arenaMin_.x, c.arenaMax_.x);
		pred_.y = std::clamp(pred_.y, c.arenaMin_.y, c.arenaMax_.y);
		pred_.z = std::clamp(pred_.z, c.arenaMin_.z, c.arenaMax_.z);

		aim_ = MyMath::Vector3Lerp(aim_, pred_, c.laserTrackStrength_);
	}

	c.laserStartWS_ = c.laserBasePos_ + Vector3{ 0.0f, c.laserMuzzleYOffset_, 0.0f };
	c.laserEndWS_ = aim_;

	if (c.timer_ >= c.laserFire_) {
		c.laserCooldownT_ = 0.0f;
		c.ChangeState(BossController::State::LaserRecover);
	}
}

//=====================================================
// LaserRecover
//=====================================================
void BossLaserRecoverState::Enter(TKM::IStateContext& ctx) {
	auto& c = AsBoss_(ctx);
	c.timer_ = 0.0f;
	c.state_ = BossController::State::LaserRecover;

	c.laserActive_ = false;
	c.laserTelegraph_ = false;
}

void BossLaserRecoverState::Update(TKM::IStateContext& ctx, float dt) {
	auto& c = AsBoss_(ctx);
	Vector3& pos = c.posWork_;

	c.laserActive_ = false;
	c.laserTelegraph_ = false;

	pos = BossController::SmoothDamp(pos, Vector3{ pos.x, c.orbitY_, c.orbitZ_ }, 0.18f, dt);

	if (c.timer_ >= c.laserRecover_) {
		c.ChangeState(BossController::State::Recover);
	}
}