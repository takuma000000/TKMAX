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
		/// <summary>
		/// リセットします。
		/// </summary>
		void Reset() {
			timer_ = 0.0f;
			interval_ = RandRange_(minInterval_, maxInterval_);
		}
		/// <summary>
		/// 花火生成の更新処理を行います。
		/// カメラのワールド行列から前方空間を求め、一定間隔で花火を生成します。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="camera">生成範囲計算に使用するカメラ（nullptr の場合は何もしません）</param>
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

		// Setter========================================
		/// <summary>
		/// 1発あたりの粒数（パーティクル数）を設定します。
		/// </summary>
		/// <param name="count">粒数（1以上にクランプされます）</param>
		void SetBurstCount(int count) { burstParticleCount_ = std::max(1, count); }
		// ==============================================

	private:
		/// <summary>
		/// 0.0〜1.0 の乱数を生成します。
		/// </summary>
		/// <returns>0.0〜1.0 の乱数値</returns>
		float Rand01_() { return float(std::rand()) / float(RAND_MAX); }
		/// <summary>
		/// 指定した範囲内の乱数を生成します。
		/// </summary>
		/// <param name="a">最小値</param>
		/// <param name="b">最大値</param>
		/// <returns>a 以上 b 以下の乱数値</returns>
		float RandRange_(float a, float b) { return a + (b - a) * Rand01_(); }
		/// <summary>
		/// 指定位置を中心に花火（パーティクル）を生成します。
		/// </summary>
		/// <param name="center">生成中心位置（ワールド座標）</param>
		void Spawn_(const Vector3& center);

		//==============================
		// Update
		//==============================
		float timer_ = 0.0f; // 経過時間
		float interval_ = 1.0f; // 次回生成までの間隔
		//==============================
		// タイミング
		//==============================
		float minInterval_ = 0.8f; // 最小間隔
		float maxInterval_ = 1.6f; // 最大間隔
		//==============================
		// 出現範囲（カメラ前方の箱）
		//==============================
		float minDepth_ = 80.0f; // 最小奥行き
		float maxDepth_ = 140.0f; // 最大奥行き
		float halfHeight_ = 25.0f; // 垂直方向半分の長さ
		//==============================
		// 生成量
		//==============================
		int burstPerTick_ = 1; // 1回の更新で何発上げるか
		int burstParticleCount_ = 60; // 1発の粒数（fw_burst）
	};
} // namespace TKM