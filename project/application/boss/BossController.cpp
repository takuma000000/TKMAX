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

	// 予測系もリセットしておくと安全
	hasPrevPlayerPos_ = false;
	prevPlayerPos_ = { 0.0f, 0.0f, 0.0f };
	playerVel = { 0.0f, 0.0f, 0.0f };
}

void BossController::Update(float dt, Enemy& boss) {
	timer_ += dt;

	// ============================================================
// Rage Gauge Update（毎フレーム）
// ・ダメージを受けたら増える（noDamageTime_ をリセット）
// ・一定秒ノーダメの時だけ減衰
// ・減衰中にダメージを受けたら：減衰即中断＆猶予リセット＆増加
// ============================================================
	if (lastHpForRage_ < 0) {
		lastHpForRage_ = boss.GetHP();
		noDamageTime_ = 0.0f;
	}
	// 今回のHP取得＆ダメ計算
	int hpNow = boss.GetHP();
	int dmg = std::max(0, lastHpForRage_ - hpNow);
	lastHpForRage_ = hpNow;

	if (dmg > 0) {
		// ダメージ受けた：増える＆減衰猶予リセット（減ってたとしてもここで止まる）
		rageGauge_ += static_cast<float>(dmg) * rageGainPerHp_;
		noDamageTime_ = 0.0f;
	} else {
		// ノーダメ：時間だけ進める
		noDamageTime_ += dt;

		// 一定時間ノーダメの時だけ減る
		if (noDamageTime_ >= rageDecayDelay_) {
			rageGauge_ -= rageDecayPerSec_ * dt;
		}
	}
	// クランプ
	rageGauge_ = std::clamp(rageGauge_, 0.0f, 1.5f);

	// ON/OFF（ヒステリシス）
	if (!rageActive_) {
		if (rageGauge_ >= rageOnThreshold_) { rageActive_ = true; } // 満タンでON
	} else {
		if (rageGauge_ <= rageOffThreshold_) { rageActive_ = false; } // 20%以下でOFF
	}

	// ① 先に最新 playerPos を取る
	if (boss.GetPlayer()) {
		playerPos = boss.GetPlayer()();
	}

	// ② その最新 playerPos を使って playerVel を計算
	if (hasPrevPlayerPos_) {
		playerVel = (playerPos - prevPlayerPos_) * (1.0f / std::max(0.0001f, dt));
	}
	playerVel.y = 0.0f; // ズレ防止のためY成分は無視
	prevPlayerPos_ = playerPos;
	hasPrevPlayerPos_ = true;

	Vector3 pos = boss.GetWorldPosition();

	switch (state_) {
	case State::Enter:      UpdateEnter(dt, boss, pos); break;
	case State::Orbit:      UpdateOrbit(dt, boss, pos, playerPos); break;
	case State::DashWindup: UpdateDashWindup(dt, boss, pos, playerPos); break;
	case State::DashRun:    UpdateDashRun(dt, boss, pos); break;
	case State::Recover:    UpdateRecover(dt, boss, pos); break;
	}

	ClampToArena(pos);
	boss.SetPosition(pos);
	boss.SyncTransform();
}

void BossController::ImGuiDebug(Enemy& boss) {
#ifdef USE_IMGUI
	ImGui::Begin("ボスコントローラ");
	// ----------------------------
	// 状態表示
	// ----------------------------
	const char* stateName = "不明";
	switch (state_) {
	case State::Enter:      stateName = "登場"; break;
	case State::Orbit:      stateName = "旋回"; break;
	case State::DashWindup: stateName = "突進予備動作"; break;
	case State::DashRun:    stateName = "突進中"; break;
	case State::Recover:    stateName = "復帰"; break;
	}
	ImGui::Text("状態: %s", stateName);
	ImGui::Text("怒り: %s", rageActive_ ? "怒りモード" : "怒ってない");
	// ----------------------------
	// Rage Gauge
	// ----------------------------
	ImGui::Text("怒りゲージ: %.2f", rageGauge_);
	ImGui::ProgressBar(std::clamp(rageGauge_, 0.0f, 1.0f), ImVec2(0.0f, 0.0f));
	ImGui::Separator();
	ImGui::Text("状態タイマー: %.2f", timer_);
	ImGui::Separator();

	// ----------------------------
	// ボス情報
	// ----------------------------
	ImGui::Text("ボスHP: %d", boss.GetHP());
	ImGui::Separator();

	if (ImGui::Button("コントローラ初期化")) {
		Reset();
	}
	ImGui::SameLine();
	if (ImGui::Button("強制：旋回")) {
		ChangeState(State::Orbit);
	}
	ImGui::SameLine();
	if (ImGui::Button("強制：突進予備")) {
		ChangeState(State::DashWindup);
	}
	ImGui::Separator();

	// ----------------------------
	// 旋回（Orbit）
	// ----------------------------
	if (ImGui::CollapsingHeader("旋回挙動", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("旋回Z位置", &orbitZ_, 0.1f);
		ImGui::DragFloat("旋回Y高さ", &orbitY_, 0.1f);
		ImGui::DragFloat("旋回半径X", &orbitRadiusX_, 0.1f);
		ImGui::DragFloat("旋回半径Y", &orbitRadiusY_, 0.1f);
		ImGui::DragFloat("旋回速度", &orbitAngularSpeed_, 0.01f);
		ImGui::DragFloat("プレイヤー影響度", &orbitPlayerInfluence_, 0.01f);
		ImGui::DragFloat("追従の強さ", &orbitFollow_, 0.01f);
		ImGui::DragFloat("旋回時間", &orbitDuration_, 0.05f);
	}

	// ----------------------------
	// 予測・フェイント
	// ----------------------------
	if (ImGui::CollapsingHeader("予測・フェイント", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("予測時間", &predictLeadTime_, 0.01f);

		ImGui::DragFloat("狙いズレX", &aimJitterX_, 0.05f);
		ImGui::DragFloat("狙いズレY", &aimJitterY_, 0.05f);
		ImGui::DragFloat("狙いズレZ", &aimJitterZ_, 0.05f);
	}

	// ----------------------------
	// 突進
	// ----------------------------
	if (ImGui::CollapsingHeader("突進", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("予備動作時間", &dashWindup_, 0.01f);
		ImGui::DragFloat("基本突進速度", &dashSpeed_, 0.1f);
		ImGui::DragFloat("現在突進速度", &dashSpeedNow_, 0.1f);
		ImGui::DragInt("連続突進回数", &dashRepeat_, 1, 1, 10);

		ImGui::Separator();
		ImGui::DragFloat("突進開始X", &dashStartX_, 0.1f);
		ImGui::DragFloat("突進終了X", &dashEndX_, 0.1f);
		ImGui::DragFloat("突進開始Z", &dashStartZ_, 0.1f);
		ImGui::DragFloat("突進終了Z", &dashEndZ_, 0.1f);

		int dt = (dashType_ == DashType::Cross) ? 0 : 1;
		if (ImGui::RadioButton("突進タイプ：横切り", dt == 0)) { dashType_ = DashType::Cross; }
		ImGui::SameLine();
		if (ImGui::RadioButton("突進タイプ：フック", dt == 1)) { dashType_ = DashType::Hook; }

		ImGui::Text("現在の突進回数: %d", dashCount_);
		ImGui::Text("突進方向反転値: %d", lastDashDir_);
	}

	// ----------------------------
	// Rage params（調整用）
	// ----------------------------
	if (ImGui::CollapsingHeader("怒りゲージ設定", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("GainPerHP", &rageGainPerHp_, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("DecayPerSec", &rageDecayPerSec_, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat("OnThreshold", &rageOnThreshold_, 0.01f, 0.0f, 2.0f);
		ImGui::DragFloat("OffThreshold", &rageOffThreshold_, 0.01f, 0.0f, 2.0f);

		if (ImGui::Button("ゲージ満タン")) {
			rageGauge_ = rageOnThreshold_;
		}
		ImGui::SameLine();
		if (ImGui::Button("ゲージ0")) {
			rageGauge_ = 0.0f;
			rageActive_ = false;
		}
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
		Vector3 aimPos = playerPos + playerVel * predictLeadTime_; // 予測位置を計算

		auto frand = [&](float a, float b) {
			return std::uniform_real_distribution<float>(a, b)(rng_);
			};

		// 外し（フェイント）
		aimPos.x += frand(-aimJitterX_, aimJitterX_);
		aimPos.y += frand(-aimJitterY_, aimJitterY_);
		aimPos.z += frand(-aimJitterZ_, aimJitterZ_);

		// 画面外を狙わないように軽くクランプ（重要）
		aimPos.x = std::clamp(aimPos.x, arenaMin_.x, arenaMax_.x);
		aimPos.y = std::clamp(aimPos.y, arenaMin_.y, arenaMax_.y);
		aimPos.z = std::clamp(aimPos.z, arenaMin_.z, arenaMax_.z);

		// ============================================================
		// ダッシュタイプ決定（優先：怒り > 固定 > ランダム）
		// ============================================================
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

		// ============================================================
		// 速度確定（怒り中は速度UP）
		// ============================================================
		dashSpeedNow_ = dashSpeed_ * (rageActive_ ? 1.35f : 1.0f);

		PrepareDash(pos, aimPos);
		ChangeState(State::DashWindup);
	}
}

void BossController::UpdateDashWindup(float dt, Enemy& boss, Vector3& pos, const Vector3& playerPos) {

	// 予備動作に入った瞬間：ベース位置を固定
	if (timer_ <= dt) {
		windupBasePos_ = pos;
		windupFxTimer_ = 0.0f;

		// 開始フレーム：外殻リング（1回）
		ParticleManager::GetInstance()->Emit("boss_windup_shell", windupBasePos_, 8);
	}

	// 継続：稲妻＆吸い込み（間引き）
	windupFxTimer_ += dt;
	if (windupFxTimer_ >= 0.06f) { // 約16フレームに1回
		windupFxTimer_ = 0.0f;
		ParticleManager::GetInstance()->Emit("boss_windup_crackle", windupBasePos_, 2);
		ParticleManager::GetInstance()->Emit("boss_windup_inward", windupBasePos_, 2);
	}

	// -------------------------------------------------
	// その場停止 ＋ ブルブル震え（3秒くらい）
	// ・pos をベースに戻してから揺れを足す（ドリフト防止）
	// -------------------------------------------------
	pos = windupBasePos_;

	float t = (dashWindup_ > 0.0001f) ? (timer_ / dashWindup_) : 1.0f;
	t = std::clamp(t, 0.0f, 1.0f);

	// 後半ほど強く（溜め感）
	float ramp = t * t;
	float amp = windupShakeAmp_ * (0.25f + 0.75f * ramp);
	if (rageActive_) { amp *= 1.25f; } // 任意：怒り中は増幅

	float sx = std::sin(timer_ * windupShakeFreq1_) * amp;
	float sy = std::sin(timer_ * windupShakeFreq2_ + 1.7f) * (amp * 0.55f);

	pos.x += sx;
	pos.y += sy;

	// 終了：突進へ
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

	// ★ Recover終了：ゲージ式ではここで怒り判定しない
	if (timer_ >= recoverDuration_) {
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

	if (dashType_ == DashType::Cross) {
		dashStartPos_ = {
			side * dashStartX_,
			orbitY_,
			dashStartZ_
		};

		dashEndPos_ = {
			-side * dashEndX_,
			orbitY_,
			dashEndZ_
		};
	} else { // Hook
		dashStartPos_ = {
			side * dashStartX_,
			orbitY_,
			dashStartZ_ + 10.0f
		};

		dashEndPos_ = {
			-side * dashEndX_,
			orbitY_,
			dashEndZ_ - 5.0f
		};
	}
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