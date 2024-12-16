#pragma once

#include "Object3d.h"
#include "Matrix4x4.h"

class Camera
{

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

	Camera();

	void Update();

	//setter
	void SetRotate(const Vector3& rotate) { this->transform.rotate = rotate; }
	void SetScale(const Vector3& scale) { this->transform.scale = scale; }
	void SetTranslate(const Vector3& translate) { this->transform.translate = translate; }
	void SetFovY(float horizontal) { this->fovY = horizontal; }
	void SetAspectRatio(float aspect) { this->aspectRatio = aspect; }
	void SetNearClip(float nearClip) { this->nearClip = nearClip; }
	void SetFarClip(float farClip) { this->farClip = farClip; }
	//getter
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const Vector3& GetRotate() const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }
	const Vector3& GetScale() const { return transform.scale; }

};

