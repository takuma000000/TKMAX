#pragma once
#include "Camera.h"
#include "MyMath.h"
#include <Windows.h>

namespace TKM {
	class DebugCamera : public TKM::Camera {
	public:
		DebugCamera() = default;
		~DebugCamera() = default;

		/// <summary>
		/// デバッグカメラを初期化します。
		/// </summary>
		/// <param name="pos"></param>
		/// <param name="target"></param>
		void Initialize(const Vector3& pos, const Vector3& target);
		/// <summary>
		/// デバッグカメラを更新します。
		/// </summary>
		void Update();

	private:
		Vector3 pos_{}; // カメラ位置
		float yaw_ = 0.0f; // Y軸回りの回転角
		float pitch_ = 0.0f; // X軸回りの回転角

		POINT prevMouse_{}; // 前回のマウス座標
		bool firstMouse_ = true; // 最初のフレームかどうか
	};
}