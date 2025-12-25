#include "ParticlerEmitter.h"

void ParticleEmitter::Initialize(std::string name, Vector3 pos){
	this->name = name;

	emitter.count = 1;           // 毎フレーム1個出す
	emitter.frequency = 0.0f;    // 0なら常時Emit
	emitter.frequencyTime = 0.0f;  // 経過時間初期化
	emitter.transform.translate = pos; // エミッター位置設定
	emitter.transform.rotate = { 0.0f,0.0f,0.0f }; // 回転は0固定
	emitter.transform.scale = { 1.0f,1.0f,1.0f }; // スケールは1固定
}

void ParticleEmitter::Emit(){
	TKM::ParticleManager::GetInstance()->Emit(name, emitter.transform.translate, emitter.count); // パーティクル発生
}

void ParticleEmitter::Update() {
	// frequency==0なら毎フレーム Emit（常時噴射）
	if (emitter.frequency <= 0.0f) {
		Emit(); // パーティクル発生
		return;
	}

	emitter.frequencyTime += kDeltaTime; // 経過時間を加算
	if (emitter.frequencyTime >= emitter.frequency) { // 発生間隔を超えたら
		emitter.frequencyTime -= emitter.frequency; // 経過時間をリセット
		Emit(); // パーティクル発生
	}
}