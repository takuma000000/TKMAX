#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
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
	//レンダーターゲットビューの初期化
	void InitializeRTV();
	//深度ステンシルビューの初期化
	void InitializeDSV();

public://色々な関数
	//指定番号のCPUディスクリプタハンドルを取得する
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	//指定番号のGPUディスクリプタハンドルを取得する
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	//SRVの指定番号のCPUディスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	//SRVの指定番号のGPUディスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

private:
	//DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	//DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	//swapChain
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResource[2] = { nullptr };
	//device
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	//command
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	//DescriptorHeap
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	const uint32_t descriptorSizeSRV;
	const uint32_t descriptorSizeRTV;
	const uint32_t descriptorSizeDSV;
	//swapChainResources
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

private:
	//WindowsAPI
	WindowsAPI* windowsAPI = nullptr;

};

