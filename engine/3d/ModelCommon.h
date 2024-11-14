#pragma once
#include "DirectXCommon.h"

class ModelCommon
{
private:
	DirectXCommon* dxCommon_;

public://メンバ関数
	void Initialize(DirectXCommon* dxCommon);

	//getter
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

};

