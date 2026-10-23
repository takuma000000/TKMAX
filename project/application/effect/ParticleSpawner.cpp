#include "ParticleSpawner.h"

namespace TKM {

	ParticleManager::Particle ParticleSpawner::MakeNewParticle(
		std::mt19937& rng,
		const std::string& /*groupName*/,
		const Vector3& center
	) {
		ParticleManager::Particle p{};

		//======================================================================
		// 発生位置
		//======================================================================
		std::uniform_real_distribution<float> offset(-0.3f, 0.3f);

		p.transform_.translate_ = {
			center.x + offset(rng),
			center.y + offset(rng) * 0.6f,
			center.z + offset(rng)
		};

		//======================================================================
		// 回転
		//======================================================================
		p.transform_.rotate_ = { 0.0f, 0.0f, 0.0f };

		//======================================================================
		// 移動速度
		//======================================================================
		std::uniform_real_distribution<float> velocityXZ(-0.15f, 0.15f);
		std::uniform_real_distribution<float> velocityY(0.10f, 0.30f);

		p.velocity_ = {
			velocityXZ(rng),
			velocityY(rng),
			velocityXZ(rng)
		};

		//======================================================================
		// サイズ
		//======================================================================
		std::uniform_real_distribution<float> scale(1.0f, 2.0f);
		const float size = scale(rng);

		p.transform_.scale_ = { size, size, size };

		//======================================================================
		// 寿命
		//======================================================================
		std::uniform_real_distribution<float> lifeTime(0.6f, 1.2f);

		p.lifeTime_ = lifeTime(rng);
		p.currentTime_ = 0.0f;

		//======================================================================
		// 色
		//======================================================================
		std::uniform_real_distribution<float> brightness(0.8f, 1.0f);
		const float base = brightness(rng);

		p.color_ = {
			base,
			base * 0.5f,
			base * 0.2f,
			1.0f
		};

		return p;
	}
}