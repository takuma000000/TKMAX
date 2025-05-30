#include "ParticlerEmitter.h"

void ParticleEmitter::Initialize(std::string name, Vector3 pos)
{
	this->name = name;

	emitter.count = 6;//パーティクルの数
	emitter.frequency = 1.0f;//パーティクルの発生間隔
	emitter.frequencyTime = 0.0f;//発生間隔のカウント
	emitter.transform.translate = pos;//パーティクルの発生位置
	emitter.transform.rotate = { 0.0f,0.0f,0.0f };//パーティクルの回転
	emitter.transform.scale = { 1.0f,1.0f,1.0f };//パーティクルのスケール
}

void ParticleEmitter::Emit()
{
	ParticleManager::GetInstance()->Emit(name, emitter.transform.translate, emitter.count);

}

void ParticleEmitter::Update()
{
	particleGroups = ParticleManager::GetInstance()->GetParticleGroups();


	emitter.frequencyTime += kDeltaTime;
	if (emitter.frequency <= emitter.frequencyTime) {

		emitter.frequencyTime -= emitter.frequency;

		for (std::unordered_map<std::string, ParticleManager::ParticleGroup>::iterator particleGroupIterator = particleGroups.begin(); particleGroupIterator != particleGroups.end();) {

			ParticleManager::ParticleGroup* particleGroup = &(particleGroupIterator->second);

			Emit();

			++particleGroupIterator;
		}
	}
}
