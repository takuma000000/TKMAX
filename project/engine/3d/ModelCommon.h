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
	/// <summary>モデル共通機能を初期化します。</summary>
	/// <param name="dxCommon">DirectX共通。</param>
	void Initialize(DirectXCommon* dxCommon);

	//getter
	/// <summary>DirectXCommonのゲッター。</summary>
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

};

