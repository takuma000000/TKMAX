#pragma once
#include "BaseEffect.h"
#include "MyMath.h"
#include <AuraVolumeRenderer.h>

namespace TKM {
	class DirectXCommon;
}

namespace TKM {

	//=============================================================
	// AuraEffectクラス
	// オーラエフェクトの管理を行うクラス。
	//=============================================================
	class AuraEffect : public BaseEffect {
	public:
		/// <summary>
		/// AuraEffect を初期化します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void Initialize(TKM::DirectXCommon* dxCommon);
		/// <summary>
		/// 毎フレーム更新
		/// </summary>
		/// <param name="dt"></param>
		void Update(float dt) override;
		/// <summary>
		/// 描画処理（RenderTexture → Swapchain）
		/// </summary>
		void Draw() override {}
		/// <summary>
		/// ImGui デバッグ表示
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// エフェクトが有効かどうか
		/// </summary>
		/// <returns></returns>
		bool IsActive() const { return active_; }
		/// <summary>
		/// GPU にパラメータを送る
		/// </summary>
		void PushToGpu();

		// Setter===================================
		/// <summary>
		/// エフェクト有効フラグの設定
		/// </summary>
		/// <param name="v"></param>
		void SetActive(bool v) { active_ = v; }
		/// <summary>
		/// 中心UVの設定
		/// </summary>
		/// <param name="uv"></param>
		void SetCenterUV(const Vector2& uv) { centerUV_ = uv; }
		/// <summary>
		/// スケールの設定
		/// </summary>
		/// <param name="s"></param>
		void SetScale(float s) { scale_ = s; }
		/// <summary>
		/// 明るさの設定
		/// </summary>
		/// <param name="s"></param>
		void SetIntensity(float s) { intensity_ = s; }
		/// <summary>
		/// リングを使うかどうかの設定
		/// </summary>
		/// <param name="v"></param>
		void SetUseRing(bool v) { useRing_ = v; }
		/// <summary>
		/// リングの半径の設定
		/// </summary>
		/// <param name="r"></param>
		void SetRingRadius(float r) { ringRadius_ = r; }
		/// <summary>
		/// リングの幅の設定
		/// </summary>
		/// <param name="w"></param>
		void SetRingWidth(float w) { ringWidth_ = w; }
		/// <summary>
		/// 色Aの設定
		/// </summary>
		/// <param name="c"></param>
		void SetColorA(const Vector3& c) { colorA_ = c; }
		/// <summary>
		/// 色Bの設定
		/// </summary>
		/// <param name="c"></param>
		void SetColorB(const Vector3& c) { colorB_ = c; }
		/// <summary>
		/// 混色の設定
		/// </summary>
		/// <param name="m"></param>
		void SetMix(float m) { mix_ = m; } // 0=A, 1=B
		/// <summary>
		/// ワールド座標の設定
		/// </summary>
		/// <param name="p"></param>
		void SetWorldPos(const Vector3& p) { worldPos_ = p; }
		/// <summary>
		/// 高さの設定
		/// </summary>
		/// <param name="h"></param>
		void SetHeight(float h) { height_ = h; }
		/// <summary>
		/// 上部UVの設定
		/// </summary>
		/// <param name="v"></param>
		void SetTopUV(const Vector2& v) { topUV_ = v; }
		/// <summary>
		/// 下部UVの設定
		/// </summary>
		/// <param name="v"></param>
		void SetBottomUV(const Vector2& v) { bottomUV_ = v; }
		/// <summary>
		/// アスペクト比の設定
		/// </summary>
		/// <param name="a"></param>
		void SetAspect(float a) { aspect_ = a; }
		/// <summary>
		/// テーパーの設定
		/// </summary>
		/// <param name="v"></param>
		void SetTaper(float v) { taper_ = v; }
		/// <summary>
		/// ノイズスケールの設定
		/// </summary>
		/// <param name="v"></param>
		void SetNoiseScale(float v) { noiseScale_ = v; }
		/// <summary>
		/// ノイズスピードの設定
		/// </summary>
		/// <param name="v"></param>
		void SetNoiseSpeed(float v) { noiseSpeed_ = v; }
		/// <summary>
		/// 炎の強さの設定
		/// </summary>
		/// <param name="v"></param>
		void SetFlameStrength(float v) { flameStrength_ = v; }
		/// <summary>
		/// エッジのキレの設定
		/// </summary>
		/// <param name="v"></param>
		void SetEdgePower(float v) { edgePower_ = v; }
		/// <summary>
		/// 垂直フェードの設定
		/// </summary>
		/// <param name="v"></param>
		void SetVerticalFade(float v) { verticalFade_ = v; }
		// =========================================
		// Getter===================================
		/// <summary>
		/// ワールド座標の取得
		/// </summary>
		/// <returns></returns>
		const Vector3& GetWorldPos() const { return worldPos_; }
		/// <summary>
		/// 高さの取得
		/// </summary>
		/// <returns></returns>
		float GetHeight() const { return height_; }
		// =========================================
	private:
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
}