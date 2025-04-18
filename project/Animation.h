#pragma once
#include "Vector3.h"
#include "MyMath.h"
#include "Quaternion.h"

class Animation
{
public:
	struct KeyFrameVector3
	{
		Vector3 value;//キーフレームの値
		float time; //キーフレームの時間(秒)
	};
	struct KeyFrameQuaternion
	{
		Quaternion value;//キーフレームの値
		float time; //キーフレームの時間(秒)
	};
};

