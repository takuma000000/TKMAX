#pragma once
#include <string>
#include <random>
#include "ParticleManager.h"

namespace TKM {

	class ParticleSpawner {
	public:
		/// <summary>
		/// パーティクルを生成する関数。groupNameで指定されたグループのパーティクルを、centerを中心にしてランダムに生成する。
		/// </summary>
		/// <param name="rng">乱数生成器</param>
		/// <param name="groupName">生成するパーティクルのグループ名</param>
		/// <param name="center">生成するパーティクルの中心位置</param>
		/// <returns>生成されたパーティクル</returns>
		static ParticleManager::Particle MakeNewParticle(std::mt19937& rng, const std::string& groupName, const Vector3& center);
	};

}