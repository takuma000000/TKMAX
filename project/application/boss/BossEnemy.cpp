#include "application/boss/BossEnemy.h"
#include <cmath>
#include "MyMath.h"
#include "application/scene/GameScene.h" // 追加：弾スポーンのため

// 安全正規化
static Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback = { 0,0,-1 }) {
	float len = MyMath::Length(v);
	if (len < 1e-5f) return fallback;
	return MyMath::Normalize(v);
}

void BossEnemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	Enemy::Initialize(common, dxCommon);

	SetModel("enemy.obj");
	SetHP(80);                              // HPを大きく
	SetScale({ 5.0f, 5.0f, 5.0f });
	SetColliderScale({ 7.5f, 7.5f, 7.5f });
}

void BossEnemy::Update() {
	if (IsDead()) return;

	// プレイヤー位置取得
	Vector3 playerPos{ 0,0,0 };
	Vector3 playerVel{ 0,0,0 };
	if (auto getter = GetPlayer()) {
		playerPos = getter();
	}

	// フェーズ切替
	UpdatePhase();

	// 移動
	UpdateMovement(playerPos, playerVel);

	// 攻撃
	UpdateAttack(1.0f, playerPos);

	// ロック時の演出（点滅代わりにスケール変化）
	if (IsLocked()) {
		blinkT_ += 0.2f;
		float s = 1.0f + 0.2f * sinf(blinkT_);
		SetScale({ 5.0f * s, 5.0f * s, 5.0f * s });
		SetColliderScale({ 5.0f * s, 5.0f * s, 5.0f * s }); // 見た目と同じだけ当たりも膨らます
	} else {
		SetScale({ 5.0f, 5.0f, 5.0f });
		SetColliderScale({ 5.5f, 5.5f, 5.5f }); // 平常時は少しだけ大きめ
	}

	Enemy::Update();
}

void BossEnemy::ImGuiDebug()
{
	// 当たり判定スケールを編集できるようにする
	Vector3 col = GetColliderScale();

	ImGui::Begin("Boss");
	ImGui::Text("HP: %d / %d", GetHP(), GetMaxHP()); // HP表示
	ImGui::Text("Phase: %s", (phase_ == Phase::P1) ? "P1" : (phase_ == Phase::P2) ? "P2" : "P3"); // フェーズ表示
	ImGui::Text("Attack: %s", (currentAttack_ == AttackType::Beam) ? "Beam" :
		(currentAttack_ == AttackType::Fan) ? "Fan" : "Rapid"); // 攻撃表示
	ImGui::Text("Stage: %s (%.1f)", (stage_ == ActStage::Telegraph) ? "Telegraph" :
		(stage_ == ActStage::Fire) ? "Fire" : "Cooldown", stageT_); // ステージ表示
	if (ImGui::DragFloat3("ColliderScale", &col.x, 0.05f, 0.1f, 50.0f)) { // スケール編集
		SetColliderScale(col); // 当たり判定スケール更新
	}
	ImGui::End();
}

void BossEnemy::UpdatePhase() {
	int hp = GetHP();
	if (hp <= 30) phase_ = Phase::P3;
	else if (hp <= 60) phase_ = Phase::P2;
	else phase_ = Phase::P1;
}

void BossEnemy::UpdateMovement(const Vector3& playerPos, const Vector3& /*playerVel*/) {
	theta_ += orbitOmega_ * (phase_ == Phase::P3 ? 1.5f : 1.0f);

	Vector3 ring = { orbitR_ * cosf(theta_), 0.0f, orbitR_ * sinf(theta_) };
	Vector3 target = playerPos + ring;

	if (target.z < playerPos.z + dzMin_) target.z = playerPos.z + dzMin_;

	Vector3 pos = GetWorldPosition();
	Vector3 toT = target - pos;
	float dist = MyMath::Length(toT);

	Vector3 desired = (dist > 0.001f) ? MyMath::Normalize(toT) * maxSpeed_ : Vector3{ 0,0,0 };
	if (dist < arriveRadius_) {
		desired = desired * (dist / arriveRadius_);
	}
	pos += desired;
	SetPosition(pos);
}

void BossEnemy::UpdateAttack(float dt, const Vector3& playerPos) {
	stageT_ += dt;
	switch (stage_) {
	case ActStage::Telegraph:
		if (stageT_ >= TelegraphTime()) { stage_ = ActStage::Fire; stageT_ = 0; FireBegin(); }
		break;
	case ActStage::Fire:
		FireTick(dt, playerPos);
		if (stageT_ >= FireTime()) { stage_ = ActStage::Cooldown; stageT_ = 0; FireEnd(); }
		break;
	case ActStage::Cooldown:
		if (stageT_ >= CooldownTime()) { stage_ = ActStage::Telegraph; stageT_ = 0; SelectNextAttack(); }
		break;
	}
}

void BossEnemy::SelectNextAttack() {
	switch (phase_) {
	case Phase::P1: currentAttack_ = AttackType::Beam; break;
	case Phase::P2: currentAttack_ = AttackType::Fan;  break;
	case Phase::P3: currentAttack_ = AttackType::Rapid; break;
	}
}

void BossEnemy::FireBegin() {
	// TODO: テレグラフ演出（光るエフェクトなど）
}

void BossEnemy::FireTick(float /*dt*/, const Vector3& playerPos) {
	// 親シーンから弾を生やす
	auto* gs = dynamic_cast<GameScene*>(GetParentScene());
	if (!gs) return;

	const Vector3 myPos = GetWorldPosition();

	switch (currentAttack_) {
	case AttackType::Beam: {
		// 3フレームに1発、プレイヤーに向けて直射（長寿命＆遅い）
		if (static_cast<int>(stageT_) % 3 == 0) {
			Vector3 dir = SafeNormalize(playerPos - myPos, { 0,0,-1 });
			gs->SpawnEnemyBullet(myPos, dir, /*speed*/0.7f, /*dmg*/2, /*life*/240);
		}
		break;
	}
	case AttackType::Fan: {
		// 10フレームに1回、扇状に5発
		if (static_cast<int>(stageT_) % 10 == 0) {
			Vector3 forward = SafeNormalize(playerPos - myPos, { 0,0,-1 });
			// XZ 平面の右ベクトル
			Vector3 right = SafeNormalize(Vector3{ forward.z, 0.0f, -forward.x }, { 1,0,0 });
			const int   N = 5;
			const float spread = 0.35f;
			for (int i = 0; i < N; ++i) {
				float t = (i - (N - 1) * 0.5f); // -2..+2
				Vector3 dir = SafeNormalize(forward + right * (t * spread));
				gs->SpawnEnemyBullet(myPos, dir, /*speed*/0.9f, /*dmg*/1, /*life*/180);
			}
		}
		break;
	}
	case AttackType::Rapid: {
		// 毎フレーム、高速・短命の連射（プレイヤー方向＋微ランダム）
		float jx = std::sinf(stageT_ * 0.7f) * 0.2f;
		float jz = std::cosf(stageT_ * 0.5f) * 0.2f;
		Vector3 base = SafeNormalize(playerPos - myPos, { 0,0,-1 });
		Vector3 dir = SafeNormalize(Vector3{ base.x + jx, base.y, base.z + jz });
		gs->SpawnEnemyBullet(myPos, dir, /*speed*/1.4f, /*dmg*/1, /*life*/120);
		break;
	}
	}
}

void BossEnemy::FireEnd() {
	// TODO: 攻撃終了演出
}

float BossEnemy::TelegraphTime() const {
	return (phase_ == Phase::P3) ? 30.0f : 45.0f; // frame想定
}
float BossEnemy::FireTime() const {
	return (phase_ == Phase::P3) ? 90.0f : 60.0f;
}
float BossEnemy::CooldownTime() const {
	return (phase_ == Phase::P1) ? 90.0f : (phase_ == Phase::P2 ? 60.0f : 45.0f);
}
