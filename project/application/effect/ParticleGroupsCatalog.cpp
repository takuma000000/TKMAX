#include "ParticleGroupsCatalog.h"

namespace TKM {
	void ParticleGroupsCatalog::RegisterScene(ParticleManager* pm) {
		// パーティクルマネージャーが無ければ登録できない
		if (!pm) { return; }

		//=========================================================
		// 共通パーティクル
		//=========================================================

		// 汎用的に使う通常粒子
		pm->CreateParticleGroup("uv", "./resources/texture/circle.png", ParticleManager::ParticleType::NORMAL);

	}
}