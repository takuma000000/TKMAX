#pragma once
#include <cstdint>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;

	class FogVolume3D {
	public:
		struct Desc {
			Vector3 centerWS{ 0.0f, 0.0f, 0.0f }; // 中心座標（ワールド）
			Vector3 halfSizeWS{ 900.0f, 220.0f, 900.0f }; // 範囲サイズ（半径）

			// --- 見た目（煙っぽく） ---
			Vector3 color{ 0.92f, 0.92f, 0.92f };
			float density = 0.06f; // 全体の濃さ（煙は少し濃い目が映える）

			uint32_t sliceCount = 80; // スライス数

			// --- もくもく感（大きい塊 + ゆっくり） ---
			float noiseScale = 0.18f;  // PS側で低周波メインに組むので、ここは少し大きめでOK
			float noiseSpeed = 0.00f;  // 煙はゆっくり

			// --- 高さ方向（下が濃い / 上が薄い）---
			// 0..1（0=体積の下端, 1=上端）
			float fogStart = 0.10f; // この高さまでは濃い
			float fogEnd = 0.90f;   // この高さでほぼ消える

			// --- ノイズ強さ / 基準座標 ---
			float noiseStrength = 0.85f; // もくもくのムラ（強め）
			float worldScale = 1.0f;     // ノイズ座標のスケール（基本1）
			Vector3 worldPos{ 0.0f, 0.0f, 0.0f }; // ノイズの基準（基本=centerWSに同期）

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