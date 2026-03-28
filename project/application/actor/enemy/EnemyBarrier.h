#pragma once
#include <memory>
#include <vector>
#include "Object3d.h"
#include "Camera.h"
#include "DirectXCommon.h"
#include "Object3dCommon.h"
#include "MyMath.h"
#include "BarrierCommon.h"

class Player;

// =============================================================
// EnemyBarrierクラス
// Wave1本隊を包むバリアの状態・描画器・Player同期を管理する。
// =============================================================
class EnemyBarrier {
public:
	EnemyBarrier() = default;
	~EnemyBarrier() = default;

	/// <summary>
	/// バリアを初期化します。
	/// </summary>
	/// <param name="common">Object3d の共通管理クラス</param>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Initialize(TKM::Object3dCommon* common, TKM::DirectXCommon* dxCommon);
	/// <summary>
	/// バリアを更新します。
	/// </summary>
	void Update(float dt);
	/// <summary>
	/// バリアを描画します。
	/// </summary>
	/// <param name="dxCommon">DirectX 共通管理クラス</param>
	void Draw(TKM::DirectXCommon* dxCommon);

	// Setter============================================
	/// <summary>
	/// カメラの設定。
	/// </summary>
	/// <param name="camera"></param>
	void SetCamera(TKM::Camera* camera);
	/// <summary>
	/// Playerの設定。バリアはPlayerの位置に追従するため、Playerへの参照を保持します。
	/// </summary>
	/// <param name="player"></param>
	void SetPlayer(Player* player);
	/// <summary>
	/// バリアの表示設定。表示状態は内部で管理され、描画処理で反映されます。
	/// </summary>
	/// <param name="visible"></param>
	void SetVisible(bool visible) { visible_ = visible; }
	/// <summary>
	/// バリアの有効設定。無効にすると描画も判定も行われなくなります。
	/// </summary>
	/// <param name="active"></param>
	void SetActive(bool active);
	/// <summary>
	/// バリアの中心位置の設定。通常はPlayerの位置に追従させるため、Playerの座標を渡すことが想定されます。
	/// </summary>
	/// <param name="center"></param>
	void SetCenter(const Vector3& center);
	/// <summary>
	/// バリアの半径の設定。バリアは球体として扱われ、半径を設定することで大きさを調整できます。
	/// </summary>
	/// <param name="radius"></param>
	void SetRadius(float radius);
	/// <summary>
	/// バリアの色の設定。描画処理で使用される色を設定します。通常は半透明な色が使用され、バリアの存在を視覚的に示すために利用されます。
	/// </summary>
	/// <param name="color"></param>
	void SetColor(const Vector4& color);
	/// <summary>
	/// バリアの形状スケールの設定。バリアは球体として描画されますが、形状スケールを設定することで、球体の見た目を変形させることができます。例えば、特定の軸方向に伸ばすことで楕円体のような見た目にすることができます。
	/// </summary>
	/// <param name="shapeScale"></param>
	void SetShapeScale(const Vector3& shapeScale);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="value"></param>
	void SetCollisionScaleZ(float value) { collisionScaleZ_ = value; }
	// ==================================================
	// Getter============================================
	/// <summary>
	/// バリアの中心位置の取得。通常はPlayerの位置に追従させるため、Playerの座標を返すことが想定されます。
	/// </summary>
	/// <returns></returns>
	float GetRadius() const { return radius_; }
	/// <summary>
	/// バリアの半径の取得。バリアは球体として扱われ、半径を取得することで大きさを知ることができます。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetCenter() const { return center_; }
	/// <summary>
	/// バリアのAABBサイズの取得。バリアは球体として扱われるため、AABBサイズは半径の2倍になります。
	/// </summary>
	/// <returns></returns>
	Vector3 GetAABBSize() const;
	/// <summary>
	/// バリアの色の取得。描画処理で使用される色を取得します。
	/// </summary>
	/// <returns></returns>
	Player* GetPlayer() const { return player_; }
	/// <summary>
	/// バリアの形状スケールの取得。バリアは球体として描画されますが、形状スケールを取得することで、球体の見た目の変形具合を知ることができます。
	/// </summary>
	/// <returns></returns>
	const Vector4& GetColor() const { return color_; }
	/// <summary>
	/// バリアの形状スケールの取得。バリアは球体として描画されますが、形状スケールを取得することで、球体の見た目の変形具合を知ることができます。
	/// </summary>
	/// <returns></returns>
	const Vector3& GetShapeScale() const { return shapeScale_; }

	float GetShaderFresnelPower() const { return shaderFresnelPower_; }
	float GetShaderBaseStrength() const { return shaderBaseStrength_; }
	float GetShaderRimStrength() const { return shaderRimStrength_; }
	float GetShaderAlphaBase() const { return shaderAlphaBase_; }
	float GetShaderAlphaRim() const { return shaderAlphaRim_; }
	const Vector3& GetShaderTint() const { return shaderTint_; }
	// ==================================================

	/// <summary>
	/// バリアが有効かどうかを取得します。有効な場合、描画と判定が行われます。無効な場合、描画も判定も行われません。
	/// </summary>
	/// <returns></returns>
	bool IsActive() const { return active_; }
	/// <summary>
	/// バリアが表示されているかどうかを取得します。表示されている場合、描画処理でバリアが描かれます。非表示の場合、描画処理でバリアは描かれませんが、判定は行われる状態になります。
	/// </summary>
	/// <returns></returns>
	bool IsVisible() const { return visible_; }

	/// <summary>
	/// バリアの状態をPlayerの位置に同期させます。通常はPlayerの座標をバリアの中心位置に設定することで、バリアがPlayerを包むようにします。
	/// </summary>
	void SyncToPlayer();

	/// <summary>
	/// バリアの破壊を開始します。破壊開始後、バリアは徐々に消えていくなどのエフェクトが発生し、最終的には無効になります。
	/// </summary>
	void StartBreak();
	/// <summary>
	/// バリアが破壊中かどうかを取得します。破壊中の場合、バリアは徐々に消えていくなどのエフェクトが発生し、最終的には無効になります。
	/// </summary>
	/// <returns></returns>
	bool IsBreaking() const { return isBreaking_; }

private:
	/// <summary>
	/// バリアの見た目を更新します。通常はバリアの中心位置や半径、色などを反映させるために、Object3d のワールド行列やマテリアルパラメータを更新する処理が含まれます。
	/// </summary>
	void UpdateVisual_();

	std::unique_ptr<TKM::Object3d> object_ = nullptr;

	TKM::DirectXCommon* dxCommon_ = nullptr;
	TKM::Camera* camera_ = nullptr;
	Player* player_ = nullptr;

	bool active_ = false;
	bool visible_ = false;

	Vector3 center_ = { 0.0f, 0.0f, 0.0f };
	float radius_ = 18.0f;
	Vector3 shapeScale_ = { 1.0f, 1.0f, 1.0f };

	float time_ = 0.0f;

	float dt_ = 0.016f; // 仮のフレーム時間（秒）。実際のゲームループでは、前フレームからの経過時間を計算して使用することが想定されます。

	TKM::BarrierCommon* barrierCommon_ = nullptr; // バリア

	Vector4 color_ = { 0.0f, 0.0f, 0.0f, 1.0f };

	float shaderFresnelPower_ = 2.2f;
	float shaderBaseStrength_ = 0.02f;
	float shaderRimStrength_ = 1.0f;
	float shaderAlphaBase_ = 0.08f;
	float shaderAlphaRim_ = 0.35f;
	Vector3 shaderTint_ = { 1.0f, 0.72f, 0.95f };

	float shaderHexScale_ = 8.0f;
	float shaderHexLineWidth_ = 0.030f;
	float shaderHexGlowStrength_ = 2.4f;
	float shaderHexAlpha_ = 0.85f;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Vector4* materialData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
	Matrix4x4* wvpData_ = nullptr;

	struct BarrierShaderParam {
		float fresnelPower;
		float baseStrength;
		float rimStrength;
		float alphaBase;

		float alphaRim;
		float hexScale;
		float hexLineWidth;
		float hexGlowStrength;

		float hexAlpha;
		float breakProgress;
		float breakEdgeWidth;
		float breakGlowStrength;

		Vector3 tint;
		float breakNoiseScale;

		Vector3 breakOrigin;
		float padding1;
	};

	Microsoft::WRL::ComPtr<ID3D12Resource> barrierShaderParamResource_;
	BarrierShaderParam* barrierShaderParamData_ = nullptr;

	bool isBreaking_ = false;
	float breakTimer_ = 0.0f;
	float breakDuration_ = 2.0f;

	float shaderBreakProgress_ = 0.0f;
	float shaderBreakEdgeWidth_ = 0.08f;
	float shaderBreakGlowStrength_ = 2.8f;
	float shaderBreakNoiseScale_ = 14.0f;
	Vector3 shaderBreakOrigin_ = { 0.0f, 0.0f, 0.0f };
	float collisionScaleZ_ = 1.0f;
};