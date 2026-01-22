#pragma once
#include "ParticleManager.h"

//=============================================================
// ParticleEmitterクラス
// パーティクルの発生位置・頻度を管理するクラス。
//=============================================================
class ParticleEmitter{
public:
	/// <summary>
	/// パーティクルエミッターを初期化します。
	/// </summary>
	/// <param name="name"></param>
	/// <param name="pos"></param>
	void Initialize(std::string name, Vector3 pos);
	/// <summary>
	/// <summary>パーティクルを放出します。</summary>
	/// </summary>
	void Emit();
	/// <summary>
	/// <summary>パーティクルエミッターを更新します。</summary>
	/// </summary>
	void Update();
	/// <summary>
	/// <summary>パーティクルエミッターの位置を設定します。</summary>
	/// </summary>
	/// <param name="pos"></param>
	void SetPosition(const Vector3& pos) {
		emitter_.transform_.translate_ = pos;
	};
private:
	//エミッター構造体
	struct Emitter {
		TKM::ParticleManager::Transform transform_;
		uint32_t count_;
		float frequency_;
		float frequencyTime_;
	};

	Emitter emitter_{};

	std::string name_;
	std::unordered_map<std::string, TKM::ParticleManager::ParticleGroup> particleGroups_;

	//Δtを定義
	const float kDeltaTime_ = 1.0f / 60.0f;
};