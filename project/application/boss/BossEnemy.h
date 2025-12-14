#pragma once
#include "Enemy.h"

namespace BossParam {
	// ここはとりあえず “動くこと優先” の初期値
	constexpr int   InitHP = 80;

	constexpr float InitScale = 5.0f;
	constexpr Vector3 InitColliderScale = { 12.180f, 18.210f, 11.560f };

	// ロック中の脈動（見た目だけ）
	constexpr float NormalScale = 5.0f;
	constexpr float LockBlinkSpeed = 0.2f;
	constexpr float LockBlinkAmount = 0.2f;
}

//=============================================================
// BossEnemy
// ・座標（移動）は BossController が決める
// ・BossEnemy は Enemy としての共通更新だけ（死亡/描画/当たり判定など）
//=============================================================
class BossEnemy : public Enemy {
public:
	BossEnemy() = default;
	~BossEnemy() = default;

	// Enemy側が virtual じゃないので override は付けない
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();
	void ImGuiDebug();

private:
	float blinkT_ = 0.0f;
};
