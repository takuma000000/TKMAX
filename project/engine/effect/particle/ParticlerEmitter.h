#pragma once
#include "ParticleManager.h"

//=============================================================
// ParticleEmitterクラス
// パーティクルの発生位置・頻度を管理するクラス。
//=============================================================
class ParticleEmitter
{
private:

public:

	void Initialize(std::string name, Vector3 pos);

	void Emit();

	void Update();

	void SetPosition(const Vector3& pos) {
		emitter.transform.translate = pos;
	};

private:
	//エミッター構造体
	struct Emitter {
		ParticleManager::Transform transform;
		uint32_t count;
		float frequency;
		float frequencyTime;
	};

	Emitter emitter{};

	std::string name;
	std::unordered_map<std::string, ParticleManager::ParticleGroup> particleGroups;

	//Δtを定義
	const float kDeltaTime = 1.0f / 60.0f;

};