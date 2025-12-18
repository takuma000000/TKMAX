#include "BossEnemy.h"
#include <cmath>

void BossEnemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	Enemy::Initialize(common, dxCommon);

	SetModel("enemy.obj");
	SetHP(BossParam::InitHP);

	// baseScale_ を正しい値にするために SetScale は最初に1回だけ
	SetScale({ BossParam::InitScale, BossParam::InitScale, BossParam::InitScale });

	SetColliderScale(BossParam::InitColliderScale);
	SetType(EnemyType::Boss);
}

void BossEnemy::Update(float dt) {
	if (IsDead()) { return; }

	// 位置ロック中は脈動エフェクト
	Enemy::Update(dt);
}

void BossEnemy::ImGuiDebug() {
#ifdef USE_IMGUI
	ImGui::Begin("ボス");

	Vector3 col = GetColliderScale();
	if (ImGui::DragFloat3("当たり判定サイズ", &col.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col);
	}

	int hp = GetHP();
	int maxHP = GetMaxHP();
	ImGui::Text("HP : %d / %d", hp, maxHP);

	// isDead/isDying/isLocked は Enemy 側の Getter を使う
	ImGui::Text("isDead : %s", IsDead() ? "true" : "false");
	ImGui::Text("isDying : %s", IsDying() ? "true" : "false");

	ImGui::End();
#endif
}
