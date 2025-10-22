#pragma once
#include "DirectXCommon.h"

//=============================================================
// ModelCommonクラス
// モデル描画で共通して使用する設定を保持するクラス。
//=============================================================
class ModelCommon
{
private:
	DirectXCommon* dxCommon_;

public://メンバ関数
	void Initialize(DirectXCommon* dxCommon);

	//getter
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

};

