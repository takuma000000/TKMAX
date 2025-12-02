#pragma once

#include "Object3d.h"
#include "Matrix4x4.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

//=============================================================
// Cameraクラス
// ビュー行列と射影行列を扱うカメラクラス。
//=============================================================

class Camera
{

	struct Transform {
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};

private:
	Transform transform;
	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;

	Matrix4x4 projectionMatrix;
	float fovY;	//水平方向視野角
	float aspectRatio;	//アスペクト比
	float nearClip;	//ニアクリップ距離
	float farClip;	//ファークリップ距離

	Matrix4x4 viewProjectionMatrix;

public://メンバ関数

	/// <summary>カメラを生成します。</summary>
	Camera();

	/// <summary>カメラを更新します。</summary>
	void Update();

	/// <summary>デバッグ用ImGui表示。</summary>
	void ImGuiDebug();

	//setter
	/// <summary>カメラの各種パラメータを設定します。</summary>
	void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
	/// <summary>カメラの各種パラメータを設定します。</summary>
	void SetScale(const Vector3& scale) { this->transform.scale = scale; }
	/// <summary>カメラの各種パラメータを設定します。</summary>
	void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
	/// <summary>カメラの各種パラメータを設定します。</summary>
	void SetFovY(float horizontal) { this->fovY = horizontal; }
	/// <summary>カメラの各種パラメータを設定します。</summary>
	void SetAspectRatio(float aspect) { this->aspectRatio = aspect; }
	/// <summary>カメラの各種パラメータを設定します。</summary>
	void SetNearClip(float nearClip) { this->nearClip = nearClip; }
	/// <summary>カメラの各種パラメータを設定します。</summary>
	void SetFarClip(float farClip) { this->farClip = farClip; }
	//getter
	/// <summary>各種行列やパラメータを取得します。</summary>
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	/// <summary>各種行列やパラメータを取得します。</summary>
	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	/// <summary>各種行列やパラメータを取得します。</summary>
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	/// <summary>各種行列やパラメータを取得します。</summary>
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	/// <summary>各種行列やパラメータを取得します。</summary>
	const Vector3& GetRotate() const { return transform.rotate; }
	/// <summary>各種行列やパラメータを取得します。</summary>
	const Vector3& GetTranslate() const { return transform.translate; }
	/// <summary>各種行列やパラメータを取得します。</summary>
	const Vector3& GetScale() const { return transform.scale; }

};

