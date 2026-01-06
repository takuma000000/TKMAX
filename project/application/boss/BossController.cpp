#include "BossController.h"
#include <cmath>

void BossController::Initialize(const Vector3& arenaMin, const Vector3& arenaMax) {
	arenaMin_ = arenaMin;
	arenaMax_ = arenaMax;
	state_ = State::Enter;
	timer_ = 0.0f;
	dashCount_ = 0;
	lastDashDir_ = 1;
	rng_.seed(std::random_device{}());

	// --- Rage Gauge init ---
	rageGauge_ = 0.0f;
	rageActive_ = false;
	lastHpForRage_ = -1;
	noDamageTime_ = 0.0f;

	// laser
	laserCooldownT_ = 0.0f;
	laserActive_ = false;
	laserTelegraph_ = false;
}

void BossController::Reset() {
	state_ = State::Enter;
	timer_ = 0.0f;
	dashCount_ = 0;
	lastDashDir_ = 1;

	// --- Rage Gauge reset ---
	rageGauge_ = 0.0f;
	rageActive_ = false;
	lastHpForRage_ = -1;
	noDamageTime_ = 0.0f;

	// laser
	laserCooldownT_ = 0.0f;
	laserActive_ = false;
	laserTelegraph_ = false;

	// 予測系もリセットしておくと安全
	hasPrevPlayerPos_ = false;
	prevPlayerPos_ = { 0.0f, 0.0f, 0.0f };
	playerVel = { 0.0f, 0.0f, 0.0f };
}

void BossController::Update(float dt, Enemy& boss) {
	timer_ += dt;

	// レーザークールタイム
	if (laserCooldownT_ > 0.0f) {
		laserCooldownT_ = std::max(0.0f, laserCooldownT_ - dt);
	}

	// ============================================================
	// Rage Gauge Update（毎フレーム）
	// ============================================================
	if (lastHpForRage_ < 0) {
		lastHpForRage_ = boss.GetHP();
		noDamageTime_ = 0.0f;
	}
	int hpNow = boss.GetHP();
	int dmg = std::max(0, lastHpForRage_ - hpNow);
	lastHpForRage_ = hpNow;

	if (dmg > 0) {
		rageGauge_ += static_cast<float>(dmg) * rageGainPerHp_;
		noDamageTime_ = 0.0f;
	} else {
		noDamageTime_ += dt;
		if (noDamageTime_ >= rageDecayDelay_) {
			rageGauge_ -= rageDecayPerSec_ * dt;
		}
	}
	rageGauge_ = std::clamp(rageGauge_, 0.0f, 1.5f);

	if (!rageActive_) {
		if (rageGauge_ >= rageOnThreshold_) { rageActive_ = true; }
	} else {
		if (rageGauge_ <= rageOffThreshold_) { rageActive_ = false; }
	}

	// ① 最新playerPos
	if (boss.GetPlayer()) {
		playerPos = boss.GetPlayer()();
	}
	// ② playerVel
	if (hasPrevPlayerPos_) {
		playerVel = (playerPos - prevPlayerPos_) * (1.0f / std::max(0.0001f, dt));
	}
	playerVel.y = 0.0f;
	prevPlayerPos_ = playerPos;
	hasPrevPlayerPos_ = true;

	Vector3 pos = boss.GetWorldPosition();

	switch (state_) {
	case State::Enter:       UpdateEnter(dt, boss, pos); break;
	case State::Orbit:       UpdateOrbit(dt, boss, pos, playerPos); break;
	case State::DashWindup:  UpdateDashWindup(dt, boss, pos, playerPos); break;
	case State::DashRun:     UpdateDashRun(dt, boss, pos); break;
	case State::Recover:     UpdateRecover(dt, boss, pos); break;
	case State::LaserWindup: UpdateLaserWindup(dt, boss, pos, playerPos); break;
	case State::LaserFire:   UpdateLaserFire(dt, boss, pos, playerPos); break;
	case State::LaserRecover:UpdateLaserRecover(dt, boss, pos); break;
	}

	// オーラ情報更新（今はDashWindupのみ）
	if (state_ == State::DashWindup) {
		auraActive_ = true;
		auraT_ += dt;
		auraPos_ = windupBasePos_;
	} else {
		auraActive_ = false;
		auraT_ = 0.0f;
	}

	// Laser状態以外ではレーザー無効
	if (state_ != State::LaserWindup && state_ != State::LaserFire) {
		laserActive_ = false;
		laserTelegraph_ = false;
	}

	ClampToArena(pos);
	boss.SetPosition(pos);
	boss.SyncTransform();
}

void BossController::ImGuiDebug(Enemy& boss) {
#ifdef USE_IMGUI
	ImGui::Begin("ボスコントローラ");
	static const char* kStateName[] = {
		"登場",
		"旋回",
		"突進予備動作",
		"突進中",
		"復帰",
		"レーザー予告",
		"レーザー発射",
		"レーザー復帰"
	};

	int si = static_cast<int>(state_);
	si = std::clamp(si, 0, (int)(sizeof(kStateName) / sizeof(kStateName[0])) - 1);
	ImGui::Text("状態: %s", kStateName[si]);
	ImGui::Text("怒り: %s", rageActive_ ? "怒りモード" : "怒ってない");

	ImGui::Text("怒りゲージ: %.2f", rageGauge_);
	ImGui::ProgressBar(std::clamp(rageGauge_, 0.0f, 1.0f), ImVec2(0.0f, 0.0f));
	ImGui::Separator();
	ImGui::Text("状態タイマー: %.2f", timer_);
	ImGui::Separator();

	ImGui::Text("ボスHP: %d", boss.GetHP());
	ImGui::Separator();

	if (ImGui::CollapsingHeader("オーラ(Aura)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("AuraActive: %s", auraActive_ ? "true" : "false");
		ImGui::Text("AuraT: %.2f", auraT_);

		ImGui::Checkbox("足元リング", &auraUseRing_);
		ImGui::DragFloat("強さ(Intensity)", &auraIntensity_, 0.01f, 0.0f, 10.0f);
		ImGui::DragFloat("広がり倍率(ScaleMul)", &auraScaleMul_, 0.01f, 0.1f, 10.0f);
		ImGui::ColorEdit3("色(Color)", &auraColor_.x);
	}

	if (auraVolume_) {
		auraVolume_->DrawImGui("Aura Volume (3D)");
	}

	ImGui::End();
#endif
}

void BossController::UpdateEnter(float dt, Enemy& boss, Vector3& pos) {
	const float targetZ = orbitZ_;
	const float speed = 18.0f;

	pos.z = Approach(pos.z, targetZ, speed * dt);
	pos.x = Approach(pos.x, 0.0f, 10.0f * dt);
	pos.y = Approach(pos.y, orbitY_, 10.0f * dt);

	if (std::abs(pos.z - targetZ) < 0.05f) {
		ChangeState(State::Orbit);
	}
}

void BossController::UpdateOrbit(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	float t = timer_;
	float angle = t * orbitAngularSpeed_;
	float ox = std::cos(angle) * orbitRadiusX_;
	float oy = std::sin(angle * 0.9f) * orbitRadiusY_;

	Vector3 target;
	target.x = playerPos.x * orbitPlayerInfluence_ + ox;
	target.y = orbitY_ + playerPos.y * 0.2f + oy;
	target.z = orbitZ_;

	pos = SmoothDamp(pos, target, orbitFollow_, dt);

	if (timer_ >= orbitDuration_) {
		// 怒り中は一定確率でレーザーへ（クールタイムあり）
		auto fr01 = [&]() { return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_); };
		// 怒り中は必ずレーザーへ（100% / クールタイム無し）
		if (rageActive_) {

			// ここで毎回狙いを作り直す（固定狙い）
			laserAimFixed_ = playerPos + playerVel * predictLeadTime_;
			laserAimFixed_.x = std::clamp(laserAimFixed_.x, arenaMin_.x, arenaMax_.x);
			laserAimFixed_.y = std::clamp(laserAimFixed_.y, arenaMin_.y, arenaMax_.y);
			laserAimFixed_.z = std::clamp(laserAimFixed_.z, arenaMin_.z, arenaMax_.z);

			laserBasePos_ = pos;

			// クールタイムを強制解除（絶対出す）
			laserCooldownT_ = 0.0f;

			ChangeState(State::LaserWindup);
			return;
		}

		// 通常はダッシュへ
		Vector3 aimPos = playerPos + playerVel * predictLeadTime_;

		auto frand = [&](float a, float b) {
			return std::uniform_real_distribution<float>(a, b)(rng_);
			};

		aimPos.x += frand(-aimJitterX_, aimJitterX_);
		aimPos.y += frand(-aimJitterY_, aimJitterY_);
		aimPos.z += frand(-aimJitterZ_, aimJitterZ_);

		aimPos.x = std::clamp(aimPos.x, arenaMin_.x, arenaMax_.x);
		aimPos.y = std::clamp(aimPos.y, arenaMin_.y, arenaMax_.y);
		aimPos.z = std::clamp(aimPos.z, arenaMin_.z, arenaMax_.z);

		if (rageActive_) {
			dashType_ = DashType::Hook;
		} else if (nextDashFixed_) {
			dashType_ = nextDashType_;
			nextDashFixed_ = false;
		} else {
			dashType_ =
				(std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) < 0.5f)
				? DashType::Cross
				: DashType::Hook;
		}

		dashSpeedNow_ = dashSpeed_ * (rageActive_ ? 1.35f : 1.0f);

		PrepareDash(pos, aimPos);
		ChangeState(State::DashWindup);
	}
}

void BossController::UpdateDashWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	if (timer_ <= dt) {
		windupBasePos_ = pos;
		windupFxTimer_ = 0.0f;

		auraActive_ = true;
		auraT_ = 0.0f;
		auraPos_ = windupBasePos_;

		TKM::ParticleManager::GetInstance()->Emit("boss_windup_shell", windupBasePos_, 8);
	}

	windupFxTimer_ += dt;
	if (windupFxTimer_ >= 0.06f) {
		windupFxTimer_ = 0.0f;
		TKM::ParticleManager::GetInstance()->Emit("boss_windup_crackle", windupBasePos_, 2);
		TKM::ParticleManager::GetInstance()->Emit("boss_windup_inward", windupBasePos_, 2);
	}

	pos = windupBasePos_;

	float t = (dashWindup_ > 0.0001f) ? (timer_ / dashWindup_) : 1.0f;
	t = std::clamp(t, 0.0f, 1.0f);

	float ramp = t * t;
	float amp = windupShakeAmp_ * (0.25f + 0.75f * ramp);
	if (rageActive_) { amp *= 1.25f; }

	float sx = std::sin(timer_ * windupShakeFreq1_) * amp;
	float sy = std::sin(timer_ * windupShakeFreq2_ + 1.7f) * (amp * 0.55f);

	pos.x += sx;
	pos.y += sy;

	if (timer_ >= dashWindup_) {
		ChangeState(State::DashRun);
	}
}

void BossController::UpdateDashRun(float dt, Enemy& boss, Vector3& pos) {
	Vector3 to = dashEndPos_ - pos;
	float len = MyMath::Length(to);

	float step = dashSpeedNow_ * dt;

	if (len <= step) {
		pos = dashEndPos_;

		dashCount_++;
		if (dashCount_ < dashRepeat_) {
			lastDashDir_ *= -1;
			PrepareDash(pos, lastPlayerPos_);
			ChangeState(State::DashWindup);
		} else {
			recoverAngle_ = std::atan2(pos.y - orbitY_, pos.x);
			ChangeState(State::Recover);
		}
		return;
	}

	Vector3 dir = MyMath::Normalize(to);
	pos += dir * step;
}

void BossController::UpdateRecover(float dt, Enemy& boss, Vector3& pos) {
	Vector3 playerPos{ 0.0f, 0.0f, 0.0f };
	if (boss.GetPlayer()) { playerPos = boss.GetPlayer()(); }

	float angle = recoverAngle_ + timer_ * orbitAngularSpeed_;

	float ox = std::cos(angle) * orbitRadiusX_;
	float oy = std::sin(angle * 0.9f) * orbitRadiusY_;

	Vector3 target;
	target.x = playerPos.x * orbitPlayerInfluence_ + ox;
	target.y = orbitY_ + playerPos.y * 0.2f + oy;
	target.z = orbitZ_;

	pos = SmoothDamp(pos, target, 0.14f, dt);

	if (timer_ >= recoverDuration_) {
		ChangeState(State::Orbit);
	}
}

// ============================
// Laser（怒り中のみ）
// ============================
void BossController::UpdateLaserWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	if (timer_ <= dt) {
		laserActive_ = true;
		laserTelegraph_ = true;
		laserBasePos_ = pos;

		laserStartWS_ = laserBasePos_ + Vector3{ 0.0f, laserMuzzleYOffset_, 0.0f };
		laserEndWS_ = laserAimFixed_;
	}

	pos = laserBasePos_;
	float t = (laserWindup_ > 0.0001f) ? (timer_ / laserWindup_) : 1.0f;
	t = std::clamp(t, 0.0f, 1.0f);
	float ramp = t * t;
	float amp = 0.18f * (0.2f + 0.8f * ramp);
	pos.x += std::sin(timer_ * 60.0f) * amp;
	pos.y += std::sin(timer_ * 87.0f + 1.7f) * (amp * 0.55f);

	laserStartWS_ = laserBasePos_ + Vector3{ 0.0f, laserMuzzleYOffset_, 0.0f };
	laserEndWS_ = laserAimFixed_;

	if (timer_ >= laserWindup_) {
		ChangeState(State::LaserFire);
	}
}

void BossController::UpdateLaserFire(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	laserActive_ = true;
	laserTelegraph_ = false;
	pos = laserBasePos_;

	Vector3 aim = laserAimFixed_;
	if (boss.GetPlayer()) {
		Vector3 p = boss.GetPlayer()();
		Vector3 v = playerVel;
		Vector3 pred = p + v * predictLeadTime_;
		pred.x = std::clamp(pred.x, arenaMin_.x, arenaMax_.x);
		pred.y = std::clamp(pred.y, arenaMin_.y, arenaMax_.y);
		pred.z = std::clamp(pred.z, arenaMin_.z, arenaMax_.z);
		aim = MyMath::Vector3Lerp(aim, pred, laserTrackStrength_);
	}
	laserStartWS_ = laserBasePos_ + Vector3{ 0.0f, laserMuzzleYOffset_, 0.0f };
	laserEndWS_ = aim;

	if (timer_ >= laserFire_) {
		// 100%出すためクールタイム無し
		laserCooldownT_ = 0.0f;
		ChangeState(State::LaserRecover);
	}
}

void BossController::UpdateLaserRecover(float dt, Enemy& boss, Vector3& pos) {
	laserActive_ = false;
	laserTelegraph_ = false;

	pos = SmoothDamp(pos, Vector3{ pos.x, orbitY_, orbitZ_ }, 0.18f, dt);

	if (timer_ >= laserRecover_) {
		ChangeState(State::Orbit);
	}
}

void BossController::ChangeState(State s) {
	state_ = s;
	timer_ = 0.0f;

	if (s == State::DashWindup) {
		if (dashCount_ == 0) {
			// 何もしない（PrepareDash側で開始する想定）
		}
	}
	if (s == State::Orbit) {
		dashCount_ = 0;
	}
}

void BossController::PrepareDash(const Vector3& currentPos, const Vector3& playerPos) {
	lastPlayerPos_ = playerPos;

	float side = (playerPos.x >= 0.0f) ? -1.0f : +1.0f;
	side *= static_cast<float>(lastDashDir_);

	const DashOffsets& o = kDashOffsets_[static_cast<int>(dashType_)];

	dashStartPos_ = {
		side * dashStartX_,
		orbitY_,
		dashStartZ_ + o.startZOff
	};

	dashEndPos_ = {
		-side * dashEndX_,
		orbitY_,
		dashEndZ_ + o.endZOff
	};
}

void BossController::ClampToArena(Vector3& p) {
	p.x = std::clamp(p.x, arenaMin_.x, arenaMax_.x);
	p.y = std::clamp(p.y, arenaMin_.y, arenaMax_.y);
	p.z = std::clamp(p.z, arenaMin_.z, arenaMax_.z);
}

float BossController::Approach(float v, float target, float delta) {
	if (v < target) { return std::min(v + delta, target); }
	return std::max(v - delta, target);
}

Vector3 BossController::SmoothDamp(const Vector3& from, const Vector3& to, float factor, float dt) {
	float k = 1.0f - std::pow(1.0f - factor, dt * 60.0f);
	return MyMath::Vector3Lerp(from, to, k);
}