#pragma once
#include "Vector3.h"
#include "Vector4.h"

//=============================================================
// DirectionalLightクラス
// 平行光源の色・方向・強度を管理するクラス。
//=============================================================
class DirectionalLight {

public:
	DirectionalLight() = default;
	~DirectionalLight() = default;

	/// <summary>
	/// <summary>平行光源を初期化します。</summary>
	/// </summary>
	/// <param name="color"></param>
	/// <param name="direction"></param>
	/// <param name="intensity"></param>
	void Initialize(const Vector4& color, const Vector3& direction, float intensity);
	/// <summary>平行光源を更新します。</summary>
	void Update();

	/// <summary>平行光源の各種パラメータを取得します。</summary>
	Vector4 GetColor() const { return color_; }
	/// <summary>平行光源の各種パラメータを取得します。</summary>
	Vector3 GetDirection() const { return direction_; }
	/// <summary>平行光源の各種パラメータを取得します。</summary>
	float GetIntensity() const { return intensity_; }

	/// <summary>平行光源の各種パラメータを設定します。</summary>
	void SetDirection(const Vector3& direction) { direction_ = direction; }

private:
	Vector4 color_;
	Vector3 direction_;
	float intensity_;
};