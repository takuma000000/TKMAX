#pragma once
#define NOMINMAX
#include <cstdlib>
#include <algorithm>
#include "MyMath.h"
#include "camera/Camera.h"
#include "WindowsAPI.h"

namespace TKM {
	class FireworkController {
	public:
		void Reset() {
			timer_ = 0.0f;
			interval_ = RandRange_(minInterval_, maxInterval_);
		}

		void Update(float dt, const TKM::Camera* camera) {
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

		// 好きなら外から調整できるようにしておく（今はデフォ値でOK）
		void SetBurstCount(int count) { burstParticleCount_ = std::max(1, count); }

	private:
		float Rand01_() { return float(std::rand()) / float(RAND_MAX); }
		float RandRange_(float a, float b) { return a + (b - a) * Rand01_(); }

		void Spawn_(const Vector3& center);

	private:
		float timer_ = 0.0f;
		float interval_ = 1.0f;

		// タイミング
		float minInterval_ = 0.8f;
		float maxInterval_ = 1.6f;

		// 出現範囲（カメラ前方の箱）
		float minDepth_ = 80.0f;
		float maxDepth_ = 140.0f;
		float halfHeight_ = 25.0f;

		// 1回の更新で何発上げるか
		int burstPerTick_ = 1;

		// 1発の粒数（fw_burst）
		int burstParticleCount_ = 60;
	};
} // namespace TKM