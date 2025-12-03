#pragma once
#include "DirectXCommon.h"

//=============================================================
// ModelCommonクラス
// モデル描画で共通して使用する設定を保持するクラス。
//=============================================================
class ModelCommon{
private:
	DirectXCommon* dxCommon_;

public://メンバ関数
	/// <summary>
	/// ModelCommonの初期化を行う関数
	/// </summary>
	/// <param name="dxCommon"></param>
	void Initialize(DirectXCommon* dxCommon);

	// Getter===================================
	/// <summary>
	/// DirectXCommonのゲッター
	/// </summary>
	/// <returns></returns>
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	// =========================================

};

