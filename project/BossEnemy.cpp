#include "BossEnemy.h"
#include <cmath> // sinf

void BossEnemy::Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
	Enemy::Initialize(common, dxCommon);

	// Boss 用に調整
	SetModel("sphere.obj");
	SetHP(50);                              // HPを大きく
	SetScale({ 5.0f, 5.0f, 5.0f });         // サイズを大きく
	SetColliderScale({ 5.0f, 5.0f, 5.0f }); // 念のため指定

}

void BossEnemy::Update() {
	Enemy::Update();

	// ロック中のスケール変化（点滅の代わりに大きさを変える）
	if (IsLocked()) {
		blinkT_ += 0.2f;
		float s = 1.0f + 0.2f * sinf(blinkT_);  // ±20% の変化
		SetScale({ 5.0f * s, 5.0f * s, 5.0f * s });
	} else {
		// 通常スケールに戻す
		SetScale({ 5.0f, 5.0f, 5.0f });
	}
}

