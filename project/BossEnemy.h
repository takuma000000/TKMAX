#pragma once
#include "Enemy.h"

class BossEnemy : public Enemy {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon) {
		Enemy::Initialize(common, dxCommon);

		// Boss 用に調整
		SetModel("sphere.obj");
		SetHP(50);                     // HPを大きく
		SetScale({ 5.0f, 5.0f, 5.0f });  // サイズを大きく
	}

	void Update() {
		// ひとまずは通常Enemyと同じ動き
		Enemy::Update();
	}
};
