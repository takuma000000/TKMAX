#include "BossController.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void BossController::Initialize(const Vector3& arenaMin, const Vector3& arenaMax) {
	arenaMin_ = arenaMin;
	arenaMax_ = arenaMax;

	state_ = State::Enter;
	timer_ = 0.0f;

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

	// 予測系
	hasPrevPlayerPos_ = false;
	prevPlayerPos_ = { 0.0f, 0.0f, 0.0f };
	playerVel_ = { 0.0f, 0.0f, 0.0f };
}

void BossController::Reset() {
	state_ = State::Enter;
	timer_ = 0.0f;

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
	playerVel_ = { 0.0f, 0.0f, 0.0f };
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
	int hpNow_ = boss.GetHP();
	int dmg_ = std::max(0, lastHpForRage_ - hpNow_);
	lastHpForRage_ = hpNow_;

	if (dmg_ > 0) {
		rageGauge_ += static_cast<float>(dmg_) * rageGainPerHp_;
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
		playerPos_ = boss.GetPlayer()();
	}
	// ② playerVel
	if (hasPrevPlayerPos_) {
		playerVel_ = (playerPos_ - prevPlayerPos_) * (1.0f / std::max(0.0001f, dt));
	}
	playerVel_.y = 0.0f;
	prevPlayerPos_ = playerPos_;
	hasPrevPlayerPos_ = true;

	Vector3 pos_ = boss.GetWorldPosition();

	switch (state_) {
	case State::Enter:       UpdateEnter(dt, boss, pos_); break;
	case State::Orbit:       UpdateOrbit(dt, boss, pos_, playerPos_); break;
	case State::LaserWindup: UpdateLaserWindup(dt, boss, pos_, playerPos_); break;
	case State::LaserFire:   UpdateLaserFire(dt, boss, pos_, playerPos_); break;
	case State::LaserRecover:UpdateLaserRecover(dt, boss, pos_); break;
	case State::Recover:     UpdateRecover(dt, boss, pos_); break;
	}

	// 今は突進撤廃なのでAuraは使わない（今後追加予定ならここで条件ONにする）
	auraActive_ = false;
	auraT_ = 0.0f;

	// Laser状態以外ではレーザー無効
	if (state_ != State::LaserWindup && state_ != State::LaserFire) {
		laserActive_ = false;
		laserTelegraph_ = false;
	}

	ClampToArena(pos_);
	boss.SetPosition(pos_);
	boss.SyncTransform();
}

void BossController::ImGuiDebug(Enemy& boss) {
#ifdef USE_IMGUI
	ImGui::Begin("ボスコントローラ");
	static const char* kStateName_[] = {
		"登場",
		"旋回",
		"レーザー予告",
		"レーザー発射",
		"レーザー復帰",
		"復帰",
	};

	int si = static_cast<int>(state_);
	si = std::clamp(si, 0, (int)(sizeof(kStateName_) / sizeof(kStateName_[0])) - 1);
	ImGui::Text("状態: %s", kStateName_[si]);
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
	const float targetZ_ = orbitZ_;
	const float speed_ = 18.0f;

	pos.z = Approach(pos.z, targetZ_, speed_ * dt);
	pos.x = Approach(pos.x, 0.0f, 10.0f * dt);
	pos.y = Approach(pos.y, orbitY_, 10.0f * dt);

	if (std::abs(pos.z - targetZ_) < 0.05f) {
		ChangeState(State::Orbit);
	}
}

void BossController::UpdateOrbit(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {
	float t_ = timer_;
	float angle_ = t_ * orbitAngularSpeed_;
	float ox_ = std::cos(angle_) * orbitRadiusX_;
	float oy_ = std::sin(angle_ * 0.9f) * orbitRadiusY_;

	Vector3 target_;
	target_.x = playerPos.x * orbitPlayerInfluence_ + ox_;
	target_.y = orbitY_ + playerPos.y * 0.2f + oy_;
	target_.z = orbitZ_;

	pos = SmoothDamp(pos, target_, orbitFollow_, dt);

	if (timer_ >= orbitDuration_) {
		// 旋回が終わったら必ずレーザー予告へ（突進撤廃）
		laserAimFixed_ = playerPos + playerVel_ * predictLeadTime_;
		laserAimFixed_.x = std::clamp(laserAimFixed_.x, arenaMin_.x, arenaMax_.x);
		laserAimFixed_.y = std::clamp(laserAimFixed_.y, arenaMin_.y, arenaMax_.y);
		laserAimFixed_.z = std::clamp(laserAimFixed_.z, arenaMin_.z, arenaMax_.z);

		laserBasePos_ = pos;

		ChangeState(State::LaserWindup);
		return;
	}
}

void BossController::UpdateRecover(float dt, Enemy& boss, Vector3& pos) {
	// 目標：軌道(Orbit)の高さとZへ戻す（Xは今のままでもOK）
	Vector3 target_{ pos.x, orbitY_, orbitZ_ };

	// ふわっと戻す（Orbitより少し強めでもいい）
	pos = SmoothDamp(pos, target_, 0.18f, dt);

	// 一定時間でOrbitへ
	if (timer_ >= recoverDuration_) {
		ChangeState(State::Orbit);
	}
}

void BossController::UpdateLaserRecover(float dt, Enemy& boss, Vector3& pos) {
	laserActive_ = false;
	laserTelegraph_ = false;

	pos = SmoothDamp(pos, Vector3{ pos.x, orbitY_, orbitZ_ }, 0.18f, dt);

	if (timer_ >= laserRecover_) {
		ChangeState(State::Recover);
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
	float t_ = (laserWindup_ > 0.0001f) ? (timer_ / laserWindup_) : 1.0f;
	t_ = std::clamp(t_, 0.0f, 1.0f);
	float ramp_ = t_ * t_;
	float amp_ = 0.18f * (0.2f + 0.8f * ramp_);
	pos.x += std::sin(timer_ * 60.0f) * amp_;
	pos.y += std::sin(timer_ * 87.0f + 1.7f) * (amp_ * 0.55f);

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

	Vector3 aim_ = laserAimFixed_;
	if (boss.GetPlayer()) {
		Vector3 p_ = boss.GetPlayer()();
		Vector3 v_ = playerVel_;
		Vector3 pred_ = p_ + v_ * predictLeadTime_;
		pred_.x = std::clamp(pred_.x, arenaMin_.x, arenaMax_.x);
		pred_.y = std::clamp(pred_.y, arenaMin_.y, arenaMax_.y);
		pred_.z = std::clamp(pred_.z, arenaMin_.z, arenaMax_.z);
		aim_ = MyMath::Vector3Lerp(aim_, pred_, laserTrackStrength_);
	}
	laserStartWS_ = laserBasePos_ + Vector3{ 0.0f, laserMuzzleYOffset_, 0.0f };
	laserEndWS_ = aim_;

	if (timer_ >= laserFire_) {
		// 100%出すためクールタイム無し
		laserCooldownT_ = 0.0f;
		ChangeState(State::LaserRecover);
	}
}

void BossController::ChangeState(State s) {
	state_ = s;
	timer_ = 0.0f;
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