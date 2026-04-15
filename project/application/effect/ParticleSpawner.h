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

	private:
		/// <summary>
		/// 開幕用：中心から“放出”する粒を生成する関数。groupNameが"irisFire"のときに呼び出される。
		/// </summary>
		/// <param name="rng">乱数生成器</param>
		/// <param name="groupName">生成するパーティクルのグループ名</param>
		/// <param name="center">生成するパーティクルの中心位置</param>
		/// <param name="p">生成されたパーティクルを格納する参照</param>
		/// <returns>生成に成功したかどうか</returns>
		static bool MakeClearStageFireParticle(
			std::mt19937& rng,
			const std::string& groupName,
			const Vector3& center,
			ParticleManager::Particle& p
		);
	};

}