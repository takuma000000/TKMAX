#pragma once
#include "Enemy.h"

class BossEnemy : public Enemy {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();

private:
	float blinkT_ = 0.0f; // 点滅用タイマー
};
