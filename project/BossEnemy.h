#pragma once
#include "Enemy.h"
#include "Camera.h"

class BossEnemy : public Enemy {
public:
	void Initialize(Object3dCommon* common, DirectXCommon* dxCommon);
	void Update();

private:


	Camera* camera_ = nullptr;

	// フェーズ管理
	enum class Phase { P1, P2, P3 };
	Phase phase_ = Phase::P1;

	// 攻撃ステージ管理
	enum class ActStage { Telegraph, Fire, Cooldown };
	ActStage stage_ = ActStage::Cooldown;
	float stageT_ = 0.0f;

	// 移動用
	float theta_ = 0.0f;     // 周回角度
	float orbitR_ = 35.0f;   // 周回半径
	float orbitOmega_ = 0.7f;// 周回角速度
	float dzMin_ = 45.0f;    // プレイヤーより常に奥にいる差

	// パラメータ
	float maxSpeed_ = 0.6f;
	float arriveRadius_ = 3.0f;

	// 内部処理
	void UpdatePhase(); // HPでフェーズ切替
	void UpdateMovement(const Vector3& playerPos, const Vector3& playerVel);
	void UpdateAttack(float dt, const Vector3& playerPos);

	// 攻撃アクション
	void FireBegin();
	void FireTick(float dt, const Vector3& playerPos);
	void FireEnd();
	void SelectNextAttack();

	// 攻撃種類
	enum class AttackType { Beam, Fan, Rapid };
	AttackType currentAttack_ = AttackType::Beam;

	// 時間制御
	float TelegraphTime() const;
	float FireTime() const;
	float CooldownTime() const;

	// 点滅演出
	float blinkT_ = 0.0f;
};
