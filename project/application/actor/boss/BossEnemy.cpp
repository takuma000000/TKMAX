#include "BossEnemy.h"
#include <cmath>

void BossEnemy::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	Enemy::Initialize(common, dxCommon);

	// 傘（本体）
	SetModel("jerryfish_boss.obj");

	// 触手（子）
	SetTentacleModel("tentacle_boss.obj");
	SetTentacleLocal({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	tentacleBasePos_ = { 0.0f, 0.0f, 0.0f };
	tentacleBaseRot_ = { 0.0f, 0.0f, 0.0f };
	tentacleBaseScale_ = { 1.0f, 1.0f, 1.0f };

	SetHP(BossParam::InitHP_);

	// baseScale_ を正しい値にするために SetScale は最初に1回だけ
	SetScale({ BossParam::InitScale_, BossParam::InitScale_, BossParam::InitScale_ });
	// 当たり判定サイズも最初に1回だけ
	SetColliderScale(BossParam::InitColliderScale_);
	// タイプは最初に1回だけ
	SetType(EnemyType::Boss);
}

void BossEnemy::Update(float dt) {
	if (IsDead()) { return; }

	// ============================
	// イントロ用パニック触手
	// ============================
	if (introPanicActive_) {
		tentacleWiggleT_ += dt;

		float s = introPanic01_;
		if (s < 0.0f) s = 0.0f;
		if (s > 1.0f) s = 1.0f;

		float freq = 18.0f + s * 10.0f;
		float ampX = 0.22f * s;
		float ampY = 0.16f * s;
		float ampZ = 0.28f * s;
		float lift = 0.20f * s;

		Vector3 pos = tentacleBasePos_;
		Vector3 rot = tentacleBaseRot_;
		Vector3 scl = tentacleBaseScale_;

		rot.x += sinf(tentacleWiggleT_ * freq) * ampX;
		rot.y += cosf(tentacleWiggleT_ * (freq * 1.35f)) * ampY;
		rot.z += sinf(tentacleWiggleT_ * (freq * 1.8f)) * ampZ;

		pos.x += sinf(tentacleWiggleT_ * (freq * 0.7f)) * 0.18f * s;
		pos.y += fabsf(sinf(tentacleWiggleT_ * (freq * 0.9f))) * lift;

		float pulse = 1.0f + fabsf(sinf(tentacleWiggleT_ * (freq * 0.65f))) * (0.06f * s);
		scl.x *= pulse;
		scl.y *= 1.0f + 0.03f * s;
		scl.z *= pulse;

		SetTentacleLocal(pos, rot, scl);
	}
	// ============================
	// 通常のチャージ触手
	// ============================
	else if (tentacleChargeActive_) {
		tentacleWiggleT_ += dt;

		float s = tentacleCharge01_;
		if (s < 0.0f) s = 0.0f;
		if (s > 1.0f) s = 1.0f;

		float freq = 9.0f + s * 4.0f;
		float ampX = 0.18f * s;
		float ampY = 0.08f * s;
		float ampZ = 0.15f * s;
		float lift = 0.12f * s;

		Vector3 pos = tentacleBasePos_;
		Vector3 rot = tentacleBaseRot_;
		Vector3 scl = tentacleBaseScale_;

		rot.x += sinf(tentacleWiggleT_ * freq) * ampX;
		rot.y += cosf(tentacleWiggleT_ * (freq * 0.7f)) * ampY;
		rot.z += sinf(tentacleWiggleT_ * (freq * 1.2f)) * ampZ;

		pos.y += sinf(tentacleWiggleT_ * (freq * 0.5f)) * lift;

		float pulse = 1.0f + sinf(tentacleWiggleT_ * (freq * 0.8f)) * (0.03f * s);
		scl.x *= pulse;
		scl.y *= pulse;
		scl.z *= pulse;

		SetTentacleLocal(pos, rot, scl);
	} else {
		SetTentacleLocal(tentacleBasePos_, tentacleBaseRot_, tentacleBaseScale_);
	}

	// 本体は従来更新
	Enemy::Update(dt);
}

void BossEnemy::ImGuiDebug() {
#ifdef USE_IMGUI
	ImGui::Begin("ボス");

	Vector3 col_ = GetColliderScale();
	if (ImGui::DragFloat3("当たり判定サイズ", &col_.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col_);
	}

	int hp_ = GetHP();
	int maxHP_ = GetMaxHP();
	ImGui::Text("HP : %d / %d", hp_, maxHP_);

	ImGui::Text("isDead : %s", IsDead() ? "true" : "false");
	ImGui::Text("isDying : %s", IsDying() ? "true" : "false");

	ImGui::End();
#endif
}

void BossEnemy::SetTentacleCharge(bool active, float charge01) {
	tentacleChargeActive_ = active; // チャージのON/OFF
	tentacleCharge01_ = charge01; // 0..1の範囲でチャージ量を指定
}

void BossEnemy::SetIntroPanic(bool active, float panic01) {
	introPanicActive_ = active; // イントロ用パニック触手のON/OFF
	introPanic01_ = panic01; // 0..1の範囲で慌て強度を指定
}