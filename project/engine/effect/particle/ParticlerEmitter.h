#pragma once
#include "ParticleManager.h"

//=============================================================
// ParticleEmitterクラス
// パーティクルの発生位置・頻度を管理するクラス。
//=============================================================
class ParticleEmitter{
private:

public:

	/// <summary>
	/// <summary>パーティクルエミッターを初期化します。</summary>
	/// </summary>
	/// <param name="name"></param>
	/// <param name="pos"></param>
	void Initialize(std::string name, Vector3 pos);

	///<summary>パーティクルを発生させます。</summary>
	void Emit();
	///<summary>パーティクルエミッターを更新します。</summary>
	void Update();

	///<summary>パーティクルエミッターの位置を設定します。</summary>
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