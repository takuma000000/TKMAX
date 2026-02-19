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
		void Reset();
		/// <summary>
		/// 花火生成の更新処理を行います。
		/// カメラのワールド行列から前方空間を求め、一定間隔で花火を生成します。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		/// <param name="camera">生成範囲計算に使用するカメラ（nullptr の場合は何もしません）</param>
		void Update(float dt, const TKM::Camera* camera);

		// Setter========================================
		/// <summary>
		/// 1発あたりの粒数（パーティクル数）を設定します。
		/// </summary>
		/// <param name="count">粒数（1以上にクランプされます）</param>
		void SetBurstCount(int count);
		// ==============================================

	private:
		/// <summary>
		/// 0.0〜1.0 の乱数を生成します。
		/// </summary>
		/// <returns>0.0〜1.0 の乱数値</returns>
		float Rand01_();
		/// <summary>
		/// 指定した範囲内の乱数を生成します。
		/// </summary>
		/// <param name="a">最小値</param>
		/// <param name="b">最大値</param>
		/// <returns>a 以上 b 以下の乱数値</returns>
		float RandRange_(float a, float b);
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