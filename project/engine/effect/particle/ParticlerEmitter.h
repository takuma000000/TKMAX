#pragma once
#include "ParticleManager.h"

class ParticleEmitter
{
private:

public:

	void Initialize(std::string name, Vector3 pos);

	void Emit();

	void Update();

private:

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