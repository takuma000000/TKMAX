#include "BossEnemy.h"
#include <cmath>
#include "MyMath.h"

void BossEnemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	Enemy::Initialize(common, dxCommon);

	SetModel("enemy.obj");
	SetHP(80);                              // HPを大きく
	SetScale({ 5.0f, 5.0f, 5.0f });
	SetColliderScale({ 5.0f, 5.0f, 5.0f });
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
	} else {
		SetScale({ 5.0f, 5.0f, 5.0f });
	}
}

void BossEnemy::UpdatePhase() {
	int hp = GetHP();
	if (hp <= 30) phase_ = Phase::P3;
	else if (hp <= 60) phase_ = Phase::P2;
	else phase_ = Phase::P1;
}

void BossEnemy::UpdateMovement(const Vector3& playerPos, const Vector3& playerVel) {
	theta_ += orbitOmega_ * (phase_ == Phase::P3 ? 1.5f : 1.0f);

	Vector3 ring = { orbitR_ * cosf(theta_), 0.0f, orbitR_ * sinf(theta_) };
	Vector3 target = playerPos + ring;

	if (target.z < playerPos.z + dzMin_) target.z = playerPos.z + dzMin_;

	Vector3 pos = GetWorldPosition();
	Vector3 toT = target - pos;
	float dist = MyMath::Length(toT);

	Vector3 desired = (dist > 0.001f) ? MyMath::Normalize(toT) * maxSpeed_ : Vector3{ 0,0,0 };
	if (dist < arriveRadius_) {
		desired = desired * (dist / arriveRadius_);  // ★ *=をやめてこれに
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
	case Phase::P2: currentAttack_ = AttackType::Fan; break;
	case Phase::P3: currentAttack_ = AttackType::Rapid; break;
	}
}

void BossEnemy::FireBegin() {
	// TODO: テレグラフ演出（光るエフェクトなど）
}

void BossEnemy::FireTick(float dt, const Vector3& playerPos) {
	switch (currentAttack_) {
	case AttackType::Beam:
		// TODO: playerPosに向かってビームを生成
		break;
	case AttackType::Fan:
		// TODO: 扇状に弾を生成
		break;
	case AttackType::Rapid:
		// TODO: ランダム方向に連射
		break;
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
