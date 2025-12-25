#pragma once
#include "BaseEffect.h"
#include "MyMath.h"
#include <AuraVolumeRenderer.h>

namespace TKM {
	class DirectXCommon;
}

	class AuraEffect : public BaseEffect {
	public:
		void Initialize(TKM::DirectXCommon* dxCommon);

		void Update(float dt) override;
		void Draw() override {}
#ifdef USE_IMGUI
		void ImGuiDebug();
#endif
		bool IsActive() const { return active_; }

		// 外部から更新（BossManager が呼ぶ想定）
		void SetActive(bool v) { active_ = v; }
		void SetCenterUV(const Vector2& uv) { centerUV_ = uv; }
		void SetScale(float s) { scale_ = s; }
		void SetIntensity(float s) { intensity_ = s; }
		void SetUseRing(bool v) { useRing_ = v; }
		void SetRingRadius(float r) { ringRadius_ = r; }
		void SetRingWidth(float w) { ringWidth_ = w; }
		void SetColorA(const Vector3& c) { colorA_ = c; }
		void SetColorB(const Vector3& c) { colorB_ = c; }
		void SetMix(float m) { mix_ = m; } // 0=A, 1=B
		void SetWorldPos(const Vector3& p) { worldPos_ = p; }
		const Vector3& GetWorldPos() const { return worldPos_; }
		float GetHeight() const { return height_; }
		void SetHeight(float h) { height_ = h; }
		void SetTopUV(const Vector2& v) { topUV_ = v; }
		void SetBottomUV(const Vector2& v) { bottomUV_ = v; }
		void SetAspect(float a) { aspect_ = a; }
		void SetTaper(float v) { taper_ = v; }
		void SetNoiseScale(float v) { noiseScale_ = v; }
		void SetNoiseSpeed(float v) { noiseSpeed_ = v; }
		void SetFlameStrength(float v) { flameStrength_ = v; }
		void SetEdgePower(float v) { edgePower_ = v; }
		void SetVerticalFade(float v) { verticalFade_ = v; }

		// 毎フレーム dxCommon に送る
		void PushToGpu();

	private:
		TKM::DirectXCommon* dxCommon_ = nullptr;

		float time_ = 0.0f;

		Vector2 centerUV_{ 0.5f, 0.5f };
		float   scale_ = 0.22f;      // 画面上の広がり
		float   intensity_ = 0.35f;  // 明るさ

		bool    useRing_ = true;
		float   ringRadius_ = 0.12f;
		float   ringWidth_ = 22.0f;

		Vector3 colorA_{ 0.2f, 0.6f, 1.0f }; // 青
		Vector3 colorB_{ 1.0f, 0.85f, 0.2f }; // 黄
		float   mix_ = 0.25f; // 混色

		bool active_ = false; // エフェクト有効フラグ

		Vector3 worldPos_{ 0.0f,0.0f,0.0f };
		float height_ = 10.0f;

		Vector2 topUV_ = { 0.5f, 0.3f };
		Vector2 bottomUV_ = { 0.5f, 0.7f };
		float aspect_ = 1.0f;

		float taper_ = 0.65f;
		float noiseScale_ = 6.0f;
		float noiseSpeed_ = 1.2f;
		float flameStrength_ = 1.2f;
		float edgePower_ = 2.0f;
		float verticalFade_ = 0.12f;

		AuraVolumeRenderer auraVolumeRenderer_;
	};