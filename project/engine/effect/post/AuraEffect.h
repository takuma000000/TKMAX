#pragma once
#include "BaseEffect.h"
#include "MyMath.h"
#include <AuraVolumeRenderer.h>
#include "EnemyBullet.h"

namespace TKM {
	class DirectXCommon;
}

namespace TKM {

	//=============================================================
	// AuraEffectクラス
	// オーラエフェクトの管理を行うクラス
	//=============================================================
	class AuraEffect : public BaseEffect {
	public:
		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// AuraEffect を初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理</param>
		void Initialize(TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// 毎フレーム更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		void Update(float dt) override;

		/// <summary>
		/// 描画処理を行います。
		/// </summary>
		void Draw() override {}

		/// <summary>
		/// ImGuiデバッグ表示を行います。
		/// </summary>
		void ImGuiDebug();

		/// <summary>
		/// GPUにパラメータを送ります。
		/// </summary>
		void PushToGpu();

		//=============================================================
		// 状態取得
		//=============================================================

		/// <summary>
		/// エフェクトが有効かを取得します。
		/// </summary>
		/// <returns>有効ならtrue</returns>
		bool IsActive() const { return active_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// エフェクト有効フラグを設定します。
		/// </summary>
		/// <param name="v">有効フラグ</param>
		void SetActive(bool v) { active_ = v; }

		/// <summary>
		/// 中心UVを設定します。
		/// </summary>
		/// <param name="uv">中心UV</param>
		void SetCenterUV(const Vector2& uv) { centerUV_ = uv; }

		/// <summary>
		/// スケールを設定します。
		/// </summary>
		/// <param name="s">スケール値</param>
		void SetScale(float s) { scale_ = s; }

		/// <summary>
		/// 明るさを設定します。
		/// </summary>
		/// <param name="s">強度</param>
		void SetIntensity(float s) { intensity_ = s; }

		/// <summary>
		/// リング使用フラグを設定します。
		/// </summary>
		/// <param name="v">使用フラグ</param>
		void SetUseRing(bool v) { useRing_ = v; }

		/// <summary>
		/// リング半径を設定します。
		/// </summary>
		/// <param name="r">半径</param>
		void SetRingRadius(float r) { ringRadius_ = r; }

		/// <summary>
		/// リング幅を設定します。
		/// </summary>
		/// <param name="w">幅</param>
		void SetRingWidth(float w) { ringWidth_ = w; }

		/// <summary>
		/// 色Aを設定します。
		/// </summary>
		/// <param name="c">色</param>
		void SetColorA(const Vector3& c) { colorA_ = c; }

		/// <summary>
		/// 色Bを設定します。
		/// </summary>
		/// <param name="c">色</param>
		void SetColorB(const Vector3& c) { colorB_ = c; }

		/// <summary>
		/// 混色率を設定します。
		/// </summary>
		/// <param name="m">0=A, 1=B</param>
		void SetMix(float m) { mix_ = m; }

		/// <summary>
		/// ワールド座標を設定します。
		/// </summary>
		/// <param name="p">座標</param>
		void SetWorldPos(const Vector3& p) { worldPos_ = p; }

		/// <summary>
		/// 高さを設定します。
		/// </summary>
		/// <param name="h">高さ</param>
		void SetHeight(float h) { height_ = h; }

		/// <summary>
		/// 上部UVを設定します。
		/// </summary>
		/// <param name="v">UV</param>
		void SetTopUV(const Vector2& v) { topUV_ = v; }

		/// <summary>
		/// 下部UVを設定します。
		/// </summary>
		/// <param name="v">UV</param>
		void SetBottomUV(const Vector2& v) { bottomUV_ = v; }

		/// <summary>
		/// アスペクト比を設定します。
		/// </summary>
		/// <param name="a">アスペクト比</param>
		void SetAspect(float a) { aspect_ = a; }

		/// <summary>
		/// テーパーを設定します。
		/// </summary>
		/// <param name="v">テーパー値</param>
		void SetTaper(float v) { taper_ = v; }

		/// <summary>
		/// ノイズスケールを設定します。
		/// </summary>
		/// <param name="v">スケール</param>
		void SetNoiseScale(float v) { noiseScale_ = v; }

		/// <summary>
		/// ノイズスピードを設定します。
		/// </summary>
		/// <param name="v">速度</param>
		void SetNoiseSpeed(float v) { noiseSpeed_ = v; }

		/// <summary>
		/// 炎の強さを設定します。
		/// </summary>
		/// <param name="v">強度</param>
		void SetFlameStrength(float v) { flameStrength_ = v; }

		/// <summary>
		/// エッジのキレを設定します。
		/// </summary>
		/// <param name="v">値</param>
		void SetEdgePower(float v) { edgePower_ = v; }

		/// <summary>
		/// 垂直フェードを設定します。
		/// </summary>
		/// <param name="v">フェード量</param>
		void SetVerticalFade(float v) { verticalFade_ = v; }

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// ワールド座標を取得します。
		/// </summary>
		/// <returns>ワールド座標</returns>
		const Vector3& GetWorldPos() const { return worldPos_; }

		/// <summary>
		/// 高さを取得します。
		/// </summary>
		/// <returns>高さ</returns>
		float GetHeight() const { return height_; }

	private:
		//=============================================================
		// 基本パラメータ
		//=============================================================

		float time_ = 0.0f;

		Vector2 centerUV_{ 0.5f, 0.5f };
		float   scale_ = 0.22f;
		float   intensity_ = 0.35f;

		bool    useRing_ = true;
		float   ringRadius_ = 0.12f;
		float   ringWidth_ = 22.0f;

		Vector3 colorA_{ 0.2f, 0.6f, 1.0f };
		Vector3 colorB_{ 1.0f, 0.85f, 0.2f };
		float   mix_ = 0.25f;

		bool active_ = false;

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

		//=============================================================
		// レンダラ
		//=============================================================

		AuraVolumeRenderer auraVolumeRenderer_;
	};
}