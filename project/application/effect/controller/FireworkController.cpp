#include "FireworkController.h"
#include "ParticleManager.h"

namespace TKM {
	void FireworkController::Reset() {
		timer_ = 0.0f;
		interval_ = RandRange_(minInterval_, maxInterval_);
	}

	void FireworkController::Update(float dt, const TKM::Camera* camera) {
		if (!camera) { return; }

		timer_ += dt;
		if (timer_ < interval_) { return; }

		// 次回へ
		timer_ = 0.0f;
		interval_ = RandRange_(minInterval_, maxInterval_);

		// カメラ行列から基底を取る（あなたの行列ルール：translationが m[3]）
		const Matrix4x4 camW = camera->GetWorldMatrix();
		Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };
		Vector3 camFwd = MyMath::Normalize({ camW.m[2][0], camW.m[2][1], camW.m[2][2] });
		Vector3 camRight = MyMath::Normalize({ camW.m[0][0], camW.m[0][1], camW.m[0][2] });
		Vector3 camUp = MyMath::Normalize({ camW.m[1][0], camW.m[1][1], camW.m[1][2] });

		float aspect = static_cast<float>(WindowsAPI::kClientWidth_) /
			static_cast<float>(WindowsAPI::kClientHeight_);

		const float halfHeight = halfHeight_;
		const float halfWidth = halfHeight_ * aspect;

		for (int i = 0; i < burstPerTick_; ++i) {
			float sx = RandRange_(-1.0f, 1.0f);
			float sy = RandRange_(-0.8f, 0.8f);
			float depth = RandRange_(minDepth_, maxDepth_);

			Vector3 center =
				camPos
				+ camFwd * depth
				+ camRight * (sx * halfWidth)
				+ camUp * (sy * halfHeight);

			Spawn_(center);
		}
	}

	void FireworkController::SetBurstCount(int count) {
		burstParticleCount_ = std::max(1, count); // 1以上にクランプ
	}

	float FireworkController::Rand01_() {
		return float(std::rand()) / float(RAND_MAX); // 0.0〜1.0の乱数を生成
	}

	float FireworkController::RandRange_(float a, float b) {
		return a + (b - a) * Rand01_(); // a 以上 b 以下の乱数を生成
	}

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