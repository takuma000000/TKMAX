#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	class LaserBeam3D {
	public:
		struct Desc {
			Vector3 startWS{ 0.0f, 0.0f, 0.0f };
			Vector3 endWS{ 0.0f, 0.0f, 0.0f };

			// ビームの太さ（ワールド半径）
			float radius = 2.2f;

			// 見た目
			Vector3 color{ 0.2f, 0.85f, 1.0f };
			float intensity = 3.0f;     // 発光強さ（加算）
			float coreSharpness = 7.0f; // 中心コアの締まり（大きいほど細く強い）
			float edgeSoftness = 1.2f;  // 外側の落ち方

			// 分割（ビーム方向に何枚置くか）
			uint32_t sliceCount = 64;

			// ゆらぎ（ちらつき/波）
			float noiseScale = 1.0f; // 1D的なノイズのスケール
			float noiseSpeed = 1.0f; // 時間変化

			// 状態
			bool active = false;
			bool telegraph = false; // 予告（点滅弱めなど）
		};

		void Initialize(DirectXCommon* dx);
		void Update(float dt);

		void Draw(const Matrix4x4& viewProj,
			const Vector3& camRightWS,
			const Vector3& camUpWS,
			const Vector3& camFwdWS);

#ifdef USE_IMGUI
		void ImGuiDebug();
#endif

		void SetActive(bool a) { desc_.active = a; }
		bool IsActive() const { return desc_.active; }

		Desc& GetDesc() { return desc_; }
		const Desc& GetDesc() const { return desc_; }

	private:
		DirectXCommon* dxCommon_ = nullptr;
		Desc desc_{};

		float time_ = 0.0f;
	};
}