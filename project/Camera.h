#pragma once

#include "Object3d.h"
#include "Matrix4x4.h"

class Camera
{

private:
	Transform transform;
	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;

public://メンバ関数
	void Update();

};

