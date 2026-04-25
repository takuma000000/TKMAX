#include "FireworkController.h"
#include "ParticleManager.h"

namespace TKM {
	void FireworkController::Reset() {
		// 経過時間をリセットする
		timer_ = 0.0f;

		// 次に花火を出すまでの間隔をランダムに決める
		interval_ = RandRange_(minInterval_, maxInterval_);
	}

	void FireworkController::Update(float dt, const TKM::Camera* camera) {
		// カメラが無ければ花火の発生位置を決められないので更新しない
		if (!camera) { return; }

		// 経過時間を進める
		timer_ += dt;

		// まだ発生間隔に達していなければ何もしない
		if (timer_ < interval_) { return; }

		//=========================================================
		// 次回発生タイミングの更新
		//=========================================================

		// タイマーをリセットする
		timer_ = 0.0f;

		// 次の発生間隔をランダムに決め直す
		interval_ = RandRange_(minInterval_, maxInterval_);

		//=========================================================
		// カメラ基準の発生範囲計算
		//=========================================================

		// カメラのワールド行列を取得する
		const Matrix4x4 camW = camera->GetWorldMatrix();

		// カメラ位置を取得する
		Vector3 camPos = { camW.m[3][0], camW.m[3][1], camW.m[3][2] };

		// カメラ前方向を取得する
		Vector3 camFwd = MyMath::Normalize({ camW.m[2][0], camW.m[2][1], camW.m[2][2] });

		// カメラ右方向を取得する
		Vector3 camRight = MyMath::Normalize({ camW.m[0][0], camW.m[0][1], camW.m[0][2] });

		// カメラ上方向を取得する
		Vector3 camUp = MyMath::Normalize({ camW.m[1][0], camW.m[1][1], camW.m[1][2] });

		// 画面アスペクト比を計算する
		float aspect = static_cast<float>(WindowsAPI::GetClientWidth()) /
			static_cast<float>(WindowsAPI::GetClientHeight());

		// 発生範囲の縦半分サイズ
		const float halfHeight = halfHeight_;

		// アスペクト比をかけて横半分サイズを決める
		const float halfWidth = halfHeight_ * aspect;

		//=========================================================
		// 花火発生
		//=========================================================
		for (int i = 0; i < burstPerTick_; ++i) {
			// 画面内の横方向ランダム位置
			float sx = RandRange_(-1.0f, 1.0f);

			// 画面内の縦方向ランダム位置
			float sy = RandRange_(-0.8f, 0.8f);

			// カメラからの奥行き距離をランダムに決める
			float depth = RandRange_(minDepth_, maxDepth_);

			// カメラ前方空間内の発生位置を計算する
			Vector3 center =
				camPos
				+ camFwd * depth
				+ camRight * (sx * halfWidth)
				+ camUp * (sy * halfHeight);

			// 計算した位置に花火を出す
			Spawn_(center);
		}
	}

	void FireworkController::SetBurstCount(int count) {
		// 花火本体の粒数を1以上にして設定する
		burstParticleCount_ = std::max(1, count);
	}

	float FireworkController::Rand01_() {
		// 0.0〜1.0の乱数を返す
		return float(std::rand()) / float(RAND_MAX);
	}

	float FireworkController::RandRange_(float a, float b) {
		// a〜bの範囲に乱数を変換して返す
		return a + (b - a) * Rand01_();
	}

	void FireworkController::Spawn_(const Vector3& center) {
		// パーティクルマネージャーを取得する
		auto pm = ParticleManager::GetInstance();

		//=========================================================
		// 打ち上げ光
		//=========================================================
		{
			// 爆発位置より下から打ち上がる光を出す
			Vector3 launchPos = center;
			launchPos.y -= 40.0f;

			// 打ち上げ用パーティクルを発生させる
			pm->Emit("fw_launch", launchPos, 1);
		}

		//=========================================================
		// 爆発フラッシュ
		//=========================================================
		{
			// 爆発中心にフラッシュを出す
			Vector3 flashPos = center;

			// フラッシュ用パーティクルを発生させる
			pm->Emit("fw_flash", flashPos, 1);
		}

		//=========================================================
		// 花火本体
		//=========================================================
		{
			// 爆発中心から放射する粒を出す
			Vector3 burstPos = center;

			// 花火本体パーティクルを指定数発生させる
			pm->Emit("fw_burst", burstPos, burstParticleCount_);
		}
	}
} // namespace TKM