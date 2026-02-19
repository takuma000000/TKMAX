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

	SetColliderScale(BossParam::InitColliderScale_);
	SetType(EnemyType::Boss);
}

void BossEnemy::Update(float dt) {
	if (IsDead()) { return; }

	// ============================
	// チャージ中だけ触手を動かす
	// ============================
	if (tentacleChargeActive_) {
		tentacleWiggleT_ += dt;

		// 強度（0..1）
		float s = tentacleCharge01_;
		if (s < 0.0f) s = 0.0f;
		if (s > 1.0f) s = 1.0f;

		// 抑えめ設定（生物感重視）
		float freq = 9.0f + s * 4.0f;   // やや速くなる程度
		float ampX = 0.18f * s;
		float ampY = 0.08f * s;
		float ampZ = 0.15f * s;
		float lift = 0.12f * s;

		Vector3 pos = tentacleBasePos_;
		Vector3 rot = tentacleBaseRot_;
		Vector3 scl = tentacleBaseScale_;

		// 回転のうねり
		rot.x += sinf(tentacleWiggleT_ * freq) * ampX;
		rot.y += cosf(tentacleWiggleT_ * (freq * 0.7f)) * ampY;
		rot.z += sinf(tentacleWiggleT_ * (freq * 1.2f)) * ampZ;

		// 上下の脈動
		pos.y += sinf(tentacleWiggleT_ * (freq * 0.5f)) * lift;

		// ごく軽いスケール変化
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