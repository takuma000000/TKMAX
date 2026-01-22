#include "BossEnemy.h"
#include <cmath>

void BossEnemy::Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon) {
	Enemy::Initialize(common, dxCommon);

	SetModel("enemy.obj");
	SetHP(BossParam::InitHP_);

	// baseScale_ を正しい値にするために SetScale は最初に1回だけ
	SetScale({ BossParam::InitScale_, BossParam::InitScale_, BossParam::InitScale_ });

	SetColliderScale(BossParam::InitColliderScale_);
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

	Vector3 col_ = GetColliderScale();
	if (ImGui::DragFloat3("当たり判定サイズ", &col_.x, 0.01f, 0.01f, 999.0f)) {
		SetColliderScale(col_);
	}

	int hp_ = GetHP();
	int maxHP_ = GetMaxHP();
	ImGui::Text("HP : %d / %d", hp_, maxHP_);

	// isDead/isDying/isLocked は Enemy 側の Getter を使う
	ImGui::Text("isDead : %s", IsDead() ? "true" : "false");
	ImGui::Text("isDying : %s", IsDying() ? "true" : "false");

	ImGui::End();
#endif
}