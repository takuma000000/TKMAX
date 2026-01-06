#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	class SmokeVolume3D {
	public:
		struct Desc {
			Vector3 centerWS{ 0.0f, 0.0f, 0.0f };
			Vector3 halfSizeWS{ 600.0f, 220.0f, 600.0f };

			// 見た目
			Vector3 color{ 0.92f, 0.92f, 0.92f };
			float density = 0.12f;

			uint32_t sliceCount = 96;

			// ノイズ（モクモク）
			float baseScale = 0.14f;        // 大きい塊
			float detailScale = 0.65f;      // 細かいディテール
			float detailStrength = 0.65f;   // 0..1

			// 雲化（threshold/softness）
			float threshold = 0.52f;  // 0..1（高いほど薄くなる）
			float softness = 0.12f;   // 0..1（大きいほど境界が柔らかい）

			// 流れ（画面手前方向：-CamFwd へ流す）
			float flowSpeed = 0.85f;  // 速さ
			float riseSpeed = 0.15f;  // 上昇（煙っぽさ）

			float alphaMax = 0.85f;   // 上限

			// ノイズ座標の基準
			float worldScale = 1.0f;
			Vector3 worldPos{ 0.0f, 0.0f, 0.0f }; // 基本 centerWS に同期
		};

		void Initialize(DirectXCommon* dx);
		void Update(float dt);

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