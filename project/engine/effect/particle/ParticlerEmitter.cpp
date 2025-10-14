#include "ParticlerEmitter.h"

void ParticleEmitter::Initialize(std::string name, Vector3 pos)
{
	this->name = name;

	emitter.count = 3;
	emitter.frequency = 0.5f;
	emitter.frequencyTime = 0.0f;
	emitter.transform.translate = pos;
	emitter.transform.rotate = { 0.0f,0.0f,0.0f };
	emitter.transform.scale = { 1.0f,1.0f,1.0f };
}

void ParticleEmitter::Emit()
{
	ParticleManager::GetInstance()->Emit(name, emitter.transform.translate, emitter.count);

}

void ParticleEmitter::Update() {
	emitter.frequencyTime += kDeltaTime;
	if (emitter.frequency <= emitter.frequencyTime) {
		emitter.frequencyTime -= emitter.frequency;
		Emit();
	}
}
