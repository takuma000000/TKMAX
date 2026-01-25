#include "FireworkController.h"
#include "ParticleManager.h"

namespace TKM {
	void FireworkController::Spawn_(const Vector3& center) {
		auto pm = ParticleManager::GetInstance();

		// 1. 打ち上がる光の筋
		{
			Vector3 launchPos = center;
			launchPos.y -= 40.0f;
			pm->Emit("fw_launch", launchPos, 1);
		}
		// 2. 爆発フラッシュ
		{
			Vector3 flashPos = center;
			pm->Emit("fw_flash", flashPos, 1);
		}
		// 3. 花火本体（放射）
		{
			Vector3 burstPos = center;
			pm->Emit("fw_burst", burstPos, burstParticleCount_);
		}
	}
} // namespace TKM