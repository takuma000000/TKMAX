#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	class FogVolume3D {
	public:
		struct Desc {
			Vector3 centerWS{ 0.0f, 0.0f, 0.0f };
			Vector3 halfSizeWS{ 900.0f, 220.0f, 900.0f };

			Vector3 color{ 0.55f, 0.75f, 0.95f };
			float density = 0.01f; // 全体の濃さ

			uint32_t sliceCount = 80; // スライス数

			float noiseScale = 0.035f;
			float noiseSpeed = 0.35f;

			float softness = 1.8f; // 端の落ち方（大きいほど中心寄りに残る）
		};

		void Initialize(DirectXCommon* dx);
		void Update(float dt);

		// Drawに必要：ViewProj と カメラ基底（ワールド）
		void Draw(const Matrix4x4& viewProj, const Vector3& camRightWS, const Vector3& camUpWS, const Vector3& camFwdWS);
#ifdef USE_IMGUI
		void ImGuiDebug();
#endif

		void SetActive(bool a) { active_ = a; }
		bool IsActive() const { return active_; }

		Desc& GetDesc() { return desc_; }
		const Desc& GetDesc() const { return desc_; }

	private:
		DirectXCommon* dxCommon_ = nullptr;
		Desc desc_{};

		bool active_ = true;
		float time_ = 0.0f;
	};
}