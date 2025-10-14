#include "application/boss/BossEnemy.h"
#include <cmath>
#include "MyMath.h"
#include "application/scene/GameScene.h"

// 既存の安全正規化（そのまま利用）
static Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback = { 0,0,-1 }) {
	float len = MyMath::Length(v);
	if (len < 1e-5f) return fallback;
	return MyMath::Normalize(v);
}

float BossEnemy::Rand01() {
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	return dist(rng_);
}
void BossEnemy::TickCooldowns() {
	cdBeam_.t = std::max(0.0f, cdBeam_.t - 1.0f);
	cdFan_.t = std::max(0.0f, cdFan_.t - 1.0f);
	cdRapid_.t = std::max(0.0f, cdRapid_.t - 1.0f);
}
float BossEnemy::DotXZ(const Vector3& a, const Vector3& b) const {
	Vector3 aa{ a.x, 0, a.z }, bb{ b.x, 0, b.z };
	float la = MyMath::Length(aa), lb = MyMath::Length(bb);
	if (la < 1e-5f || lb < 1e-5f) return 0.0f;
	aa = aa * (1.0f / la); bb = bb * (1.0f / lb);
	return aa.x * bb.x + aa.z * bb.z;
}
Vector3 BossEnemy::PredictPlayer(const Vector3& playerPos) const {
	// 1フレーム=約1/60秒想定。弾速目安に合わせて先読み距離を控えめに。
	float lookAhead = 6.0f; // フレーム
	return playerPos + playerVelFiltered_ * lookAhead;
}
float BossEnemy::PhaseBiasFor(AttackType at) const {
	switch (phase_) {
	case Phase::P1:
		if (at == AttackType::Beam)  return phaseBiasBeam_;
		if (at == AttackType::Fan)   return 0.4f;
		return 0.2f;
	case Phase::P2:
		if (at == AttackType::Fan)   return phaseBiasFan_;
		if (at == AttackType::Beam)  return 0.6f;
		return 0.5f;
	case Phase::P3:
		if (at == AttackType::Rapid) return phaseBiasRapid_;
		if (at == AttackType::Fan)   return 0.7f;
		return 0.4f;
	}
	return 0.0f;
}

void BossEnemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	Enemy::Initialize(common, dxCommon);
	SetModel("enemy.obj");
	SetHP(80);
	SetScale({ 5.0f, 5.0f, 5.0f });
	SetColliderScale({ 7.5f, 7.5f, 7.5f });
}

void BossEnemy::Update() {
	if (IsDead()) return;

	// プレイヤー位置（既存の GetPlayer ラムダ）を利用
	Vector3 playerPos{ 0,0,0 };
	if (auto getter = GetPlayer()) playerPos = getter();

	// プレイヤー速度の簡易推定（指数平滑）
	Vector3 instV = playerPos - prevPlayerPos_;
	playerVelFiltered_ = playerVelFiltered_ * (1.0f - velFilter_) + instV * velFilter_;
	prevPlayerPos_ = playerPos;

	UpdatePhase();                                    // フェーズ判定（HP）  :contentReference[oaicite:2]{index=2}
	UpdateMovement(playerPos, playerVelFiltered_);    // 軌道に微ゆらぎを加えつつ追従
	UpdateAttack(1.0f, playerPos);                    // 攻撃FSMは存続（Telegraph/Fire/Cooldown）

	// ロック時の演出（既存）
	if (IsLocked()) {
		blinkT_ += 0.2f;
		float s = 1.0f + 0.2f * sinf(blinkT_);
		SetScale({ 5.0f * s, 5.0f * s, 5.0f * s });
		SetColliderScale({ 5.0f * s, 5.0f * s, 5.0f * s });
	} else {
		SetScale({ 5.0f, 5.0f, 5.0f });
		SetColliderScale({ 5.5f, 5.5f, 5.5f });
	}

	Enemy::Update();
}

// BossEnemy.cpp 内 ImGuiDebug() を拡張（可視化パネル）
void BossEnemy::ImGuiDebug() {
	Vector3 col = GetColliderScale();
	ImGui::Begin("Boss");

	ImGui::Text("HP: %d / %d", GetHP(), GetMaxHP());
	ImGui::Text("Phase: %s", (phase_ == Phase::P1) ? "P1" : (phase_ == Phase::P2) ? "P2" : "P3");
	ImGui::Text("Stage: %s (%.1f)", (stage_ == ActStage::Telegraph) ? "Telegraph" :
		(stage_ == ActStage::Fire) ? "Fire" : "Cooldown", stageT_);
	if (ImGui::DragFloat3("ColliderScale", &col.x, 0.05f, 0.1f, 50.0f)) SetColliderScale(col);

	ImGui::Separator();
	ImGui::Checkbox("Show AI Inspector", &dbg_.show);

	// 既存のチューニング項目（省略可。ここは元のまま）
	if (dbg_.show) {
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.8f, 0.9f, 1, 1), "AI Inputs");
		ImGui::Text("dist: %.2f", dbg_.dist);
		ImGui::ProgressBar(std::min(dbg_.align, 1.0f), ImVec2(180, 0), "align");

		ImGui::Text("jitter: %+0.3f", dbg_.jitter);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.8f, 1, 0.8f, 1), "Distance Fitness");
		ImGui::ProgressBar(std::clamp(dbg_.distBeam, 0.f, 1.f), ImVec2(180, 0), "Beam distPref");
		ImGui::ProgressBar(std::clamp(dbg_.distFan, 0.f, 1.f), ImVec2(180, 0), "Fan  distPref");
		ImGui::ProgressBar(std::clamp(dbg_.distRapid, 0.f, 1.f), ImVec2(180, 0), "Rapid distPref");

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1, 0.9f, 0.7f, 1), "Phase Bias (+)");
		ImGui::Text("Beam:+%.2f  Fan:+%.2f  Rapid:+%.2f", dbg_.biasBeam, dbg_.biasFan, dbg_.biasRapid);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1, 0.7f, 0.7f, 1), "Penalty (-)");
		ImGui::Text("CD:   B:%.1f  F:%.1f  R:%.1f", dbg_.cdBeam, dbg_.cdFan, dbg_.cdRapid);
		ImGui::Text("Chain:B:%.1f  F:%.1f  R:%.1f", dbg_.chainBeam, dbg_.chainFan, dbg_.chainRapid);

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.9f, 0.9f, 1, 1), "Final Scores");
		auto bar = [](const char* lbl, float v) {
			float view = std::clamp((v + 3.0f) / 6.0f, 0.0f, 1.0f); // 見栄え用に -3..+3 を 0..1 に
			ImGui::ProgressBar(view, ImVec2(220, 0), lbl);
			};
		bar((std::string("Beam  s=") + std::to_string(dbg_.sBeam)).c_str(), dbg_.sBeam);
		bar((std::string("Fan   s=") + std::to_string(dbg_.sFan)).c_str(), dbg_.sFan);
		bar((std::string("Rapid s=") + std::to_string(dbg_.sRapid)).c_str(), dbg_.sRapid);

		ImGui::Separator();
		const char* chosen =
			(dbg_.chosen == 0) ? "Beam" : (dbg_.chosen == 1) ? "Fan" : "Rapid";
		ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "Chosen: %s", chosen);

		// 履歴帯（直近16手）
		ImGui::Text("History (latest ->)");
		ImGui::BeginChild("hist", ImVec2(240, 22), true);
		for (int i = 0; i < kHist; ++i) {
			int idx = (histIndex_ - 1 - i + kHist) % kHist;
			int v = history_[idx];
			ImVec4 c = (v == 0) ? ImVec4(0.6f, 0.8f, 1, 1) : (v == 1) ? ImVec4(0.6f, 1, 0.6f, 1) : ImVec4(1, 0.6f, 0.6f, 1);
			ImGui::SameLine();
			ImGui::TextColored(c, "%s", (v == 0) ? "B" : (v == 1) ? "F" : "R");
		}
		ImGui::EndChild();
	}

	// 元のチューニング類（重み/バイアス/CDなど）はすでにこの関数にあるので省略
	ImGui::End();
}

void BossEnemy::UpdatePhase() {
	int hp = GetHP();
	if (hp <= 30) phase_ = Phase::P3;
	else if (hp <= 60) phase_ = Phase::P2;
	else phase_ = Phase::P1;
}

void BossEnemy::UpdateMovement(const Vector3& playerPos, const Vector3& /*playerVel*/) {
	// フェーズが上がるほど軌道にゆらぎを入れて読みづらく
	float rJit = (phase_ == Phase::P3 ? orbitR_Jitter_ * 1.0f : (phase_ == Phase::P2 ? orbitR_Jitter_ * 0.6f : orbitR_Jitter_ * 0.3f));
	float omgJit = (phase_ == Phase::P3 ? orbitOmega_Jitter_ : (phase_ == Phase::P2 ? orbitOmega_Jitter_ * 0.6f : orbitOmega_Jitter_ * 0.3f));

	float r = orbitR_ + (Rand01() * 2.0f - 1.0f) * rJit;
	float omg = orbitOmega_ + (Rand01() * 2.0f - 1.0f) * omgJit;

	theta_ += omg;

	Vector3 ring = { r * cosf(theta_), 0.0f, r * sinf(theta_) };
	Vector3 target = playerPos + ring;

	if (target.z < playerPos.z + dzMin_) target.z = playerPos.z + dzMin_;

	Vector3 pos = GetWorldPosition();
	Vector3 toT = target - pos;
	float dist = MyMath::Length(toT);

	Vector3 desired = (dist > 0.001f) ? MyMath::Normalize(toT) * maxSpeed_ : Vector3{ 0,0,0 };
	if (dist < arriveRadius_) desired = desired * (dist / arriveRadius_);
	pos += desired;
	SetPosition(pos);
}

void BossEnemy::UpdateAttack(float dt, const Vector3& playerPos) {
	stageT_ += dt;
	TickCooldowns();

	switch (stage_) {
	case ActStage::Telegraph:
		if (stageT_ >= TelegraphTime()) { stage_ = ActStage::Fire; stageT_ = 0; FireBegin(); }
		break;
	case ActStage::Fire:
		FireTick(dt, playerPos);
		if (stageT_ >= FireTime()) { stage_ = ActStage::Cooldown; stageT_ = 0; FireEnd(); }
		break;
	case ActStage::Cooldown:
		if (stageT_ >= CooldownTime()) {
			stage_ = ActStage::Telegraph; stageT_ = 0;
			SelectNextAttackUtility(playerPos); // ← ここでAI選択
		}
		break;
	}
}

void BossEnemy::SelectNextAttackUtility(const Vector3& playerPos) {
	// ===== 1) 入力の前処理 =====
	const Vector3 me = GetWorldPosition();
	const Vector3 toPlayer = playerPos - me;
	const float   dist = MyMath::Length(toPlayer);

	// 正面ベクトル（必要なら将来ここを実向きに合わせて更新）
	const Vector3 forward = SafeNormalize(Vector3{ 0,0,1 });
	// XZ平面での正対度 [-1..1] → [0..1] に射影
	float align01 = DotXZ(forward, toPlayer); // -1..1
	align01 = align01 * 0.5f + 0.5f;          // 0..1

	// ===== 2) 距離適性（各技が得意な距離帯） =====
	auto distPref = [&](float d, float center, float width) {
		// 中心(center)から離れるほど線形で減点：peak=1, ±widthで0
		const float t = std::abs(d - center) / width;
		return std::max(0.0f, 1.0f - t); // 0..1
		};
	const float fitBeam = distPref(dist, 45.0f, 30.0f); // 遠〜中
	const float fitFan = distPref(dist, 32.0f, 18.0f); // 中
	const float fitRapid = distPref(dist, 18.0f, 16.0f); // 近

	// ===== 3) フェーズ・バイアス（“今はこの技を出したい”） =====
	const float biasBeam = PhaseBiasFor(AttackType::Beam);
	const float biasFan = PhaseBiasFor(AttackType::Fan);
	const float biasRapid = PhaseBiasFor(AttackType::Rapid);

	// ===== 4) ペナルティ（CD中/同技連発） =====
	auto cdPenalty = [&](const CD& cd) -> float { return (cd.t > 0.0f) ? 0.6f : 0.0f; };
	auto chainPenalty = [&](AttackType at) -> float {
		return (lastAttack_ == at && sameAttackChain_ >= maxSameChain_) ? 0.7f : 0.0f;
		};
	const float penBeamCD = cdPenalty(cdBeam_);
	const float penFanCD = cdPenalty(cdFan_);
	const float penRapidCD = cdPenalty(cdRapid_);
	const float penBeamCh = chainPenalty(AttackType::Beam);
	const float penFanCh = chainPenalty(AttackType::Fan);
	const float penRapidCh = chainPenalty(AttackType::Rapid);

	// ===== 5) ゆらぎ（毎回ちょっと違う選択にするためのノイズ） =====
	const float jitter = (Rand01() * 2.0f - 1.0f) * jitterAmplitude_; // ±jitterAmplitude_

	// ===== 6) 最終スコア（見やすい形に分解して合算） =====
	const float sBeam =
		wDistBeam_ * fitBeam +     // 距離適性
		wAlignBeam_ * align01 +     // 正対
		biasBeam +     // フェーズバイアス
		jitter -     // ゆらぎ
		penBeamCD - penBeamCh;        // ペナルティ

	const float sFan =
		wDistFan_ * fitFan +
		wAlignFan_ * align01 +
		biasFan +
		jitter -
		penFanCD - penFanCh;

	const float sRapid =
		wDistRapid_ * fitRapid +
		wAlignRapid_ * align01 +
		biasRapid +
		jitter -
		penRapidCD - penRapidCh;

	// ===== 7) 選択（僅差の時は微小ノイズでタイブレーク） =====
	AttackType next = AttackType::Beam;
	float best = sBeam;

	auto consider = [&](AttackType at, float s) {
		// 同点付近での固定化防止：極小ノイズを足して比較
		const float epsBreak = 1e-4f * (Rand01() - 0.5f);
		if (s + epsBreak > best) { best = s; next = at; }
		};
	consider(AttackType::Fan, sFan);
	consider(AttackType::Rapid, sRapid);

	// ===== 8) 選択結果の反映（CD/連続回数/現行技） =====
	switch (next) {
	case AttackType::Beam:  cdBeam_.t = cdBeam_.cool;  break;
	case AttackType::Fan:   cdFan_.t = cdFan_.cool;   break;
	case AttackType::Rapid: cdRapid_.t = cdRapid_.cool; break;
	}
	sameAttackChain_ = (lastAttack_ == next) ? (sameAttackChain_ + 1) : 0;
	lastAttack_ = next;
	currentAttack_ = next;
}


void BossEnemy::FireBegin() {
	// テレグラフ演出など任意
}

void BossEnemy::FireTick(float /*dt*/, const Vector3& playerPos) {
	auto* gs = dynamic_cast<GameScene*>(GetParentScene());
	if (!gs) return;

	const Vector3 myPos = GetWorldPosition();
	const Vector3 aimPos = PredictPlayer(playerPos); // 簡易先読み

	switch (currentAttack_) {
	case AttackType::Beam: {
		// 3fに1発、やや遅い・長寿命の直射（先読み方向）
		if (static_cast<int>(stageT_) % 3 == 0) {
			Vector3 dir = SafeNormalize(aimPos - myPos, { 0,0,-1 });
			gs->SpawnEnemyBullet(myPos, dir, 0.7f, 2, 240);
		}
		break;
	}
	case AttackType::Fan: {
		// 10fに1回、扇状にN発。散開角に毎回微ゆらぎ
		if (static_cast<int>(stageT_) % 10 == 0) {
			Vector3 forward = SafeNormalize(aimPos - myPos, { 0,0,-1 });
			Vector3 right = SafeNormalize(Vector3{ forward.z, 0.0f, -forward.x }, { 1,0,0 });

			int   N = std::max(3, fanCount_);
			float spread = fanSpread_ * (0.9f + 0.2f * Rand01()); // ±10%ゆらぎ

			for (int i = 0; i < N; ++i) {
				float t = (i - (N - 1) * 0.5f); // -k..+k
				Vector3 dir = SafeNormalize(forward + right * (t * spread));
				gs->SpawnEnemyBullet(myPos, dir, 0.9f, 1, 180);
			}
		}
		break;
	}
	case AttackType::Rapid: {
		// 毎フレーム、短命・高速の連射（ジッターを動的変化）
		float jx = std::sinf(stageT_ * 0.7f) * rapidJitterX_;
		float jz = std::cosf(stageT_ * 0.5f) * rapidJitterZ_;
		Vector3 base = SafeNormalize(aimPos - myPos, { 0,0,-1 });
		Vector3 dir = SafeNormalize(Vector3{ base.x + jx, base.y, base.z + jz });
		gs->SpawnEnemyBullet(myPos, dir, 1.4f, 1, 120);
		break;
	}
	}
}

void BossEnemy::FireEnd() {
	// 終了演出など任意
}

float BossEnemy::TelegraphTime() const { return (phase_ == Phase::P3) ? 30.0f : 45.0f; }
float BossEnemy::FireTime()      const { return (phase_ == Phase::P3) ? 90.0f : 60.0f; }
float BossEnemy::CooldownTime()  const { return (phase_ == Phase::P1) ? 90.0f : (phase_ == Phase::P2 ? 60.0f : 45.0f); }

// BossEnemy.cpp（追記：ヘルパー）
const char* BossEnemy::AttackName(AttackType at) const {
	switch (at) {
	case AttackType::Beam:  return "Beam";
	case AttackType::Fan:   return "Fan";
	case AttackType::Rapid: return "Rapid";
	}
	return "?";
}
void BossEnemy::PushHistory(AttackType at) {
	history_[histIndex_] = (at == AttackType::Beam ? 0 : (at == AttackType::Fan ? 1 : 2));
	histIndex_ = (histIndex_ + 1) % kHist;
}
