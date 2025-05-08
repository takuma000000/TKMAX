#include "ParticlerEmitter.h"

void ParticleEmitter::Initialize(std::string name, Vector3 pos)
{
	this->name = name;

	// 結晶生成は一発で出して終わりなので、Emit一回分だけ
	emitter.count = 5;       // 1回の発生数
	emitter.frequency = 0.1f; // 0.5秒ごとに出現
	emitter.frequencyTime = 0.0f;

	emitter.transform.translate = pos;
	emitter.transform.rotate = { 0.0f, 0.0f, 0.0f };
	emitter.transform.scale = { 1.0f, 1.0f, 1.0f };
}



void ParticleEmitter::Emit()
{
	ParticleManager::GetInstance()->Emit(name, emitter.transform.translate, emitter.count);

}

void ParticleEmitter::Update()
{
	particleGroups = ParticleManager::GetInstance()->GetParticleGroups();

	emitter.frequencyTime += kDeltaTime;
	if (emitter.frequencyTime >= emitter.frequency) {
		emitter.frequencyTime = 0.0f; // リセット

		// ここでEmitする
		ParticleManager::GetInstance()->Emit(name, emitter.transform.translate, emitter.count);
	}
}



