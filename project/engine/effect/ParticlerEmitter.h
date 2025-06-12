#pragma once
#include "ParticleManager.h"

class ParticleEmitter
{
private:

public:

	void Initialize(std::string name, Vector3 pos);

	void Emit();

	void Update();

	inline const std::string& GetName() const { return name; }

	inline Vector3 GetPosition() const { return emitter.transform.translate; }

	inline float GetFrequency() const { return emitter.frequency; }

	inline uint32_t GetCount() const { return emitter.count; }

	inline void SetFrequency(float freq) { emitter.frequency = freq; }

	inline void SetCount(uint32_t count) { emitter.count = count; }

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