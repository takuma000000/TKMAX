#include "ParticlerEmitter.h"

void ParticleEmitter::Initialize(std::string name, Vector3 pos)
{
	this->name = name;

	emitter.count = 1;           // 毎フレーム1個出す
	emitter.frequency = 0.0f;    // 0なら常時Emit
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
	// frequency==0なら毎フレーム Emit（常時噴射）
	if (emitter.frequency <= 0.0f) {
		Emit();
		return;
	}

	emitter.frequencyTime += kDeltaTime;
	if (emitter.frequencyTime >= emitter.frequency) {
		emitter.frequencyTime -= emitter.frequency;
		Emit();
	}
}
