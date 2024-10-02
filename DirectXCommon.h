#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "WindowsAPI.h"


class DirectXCommon
{
public://メンバ関数
	//初期化
	void Initialize(WindowsAPI* windowsAPI);
	//デバイス初期化
	void InitializeDevice();
	//コマンド関連の初期化
	void InitializeCommand();
	//スワップチェーンの生成
	void GenerateSwapChain();
	//深度バッファの生成
	void GenerateZBuffer();
	//各種デスクリプタヒープの生成
	void GenerateDescpitorHeap();

private:
	//DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	//DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
private:
	//WindowsAPI
	WindowsAPI* windowsAPI = nullptr;
};

