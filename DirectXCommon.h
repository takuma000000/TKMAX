#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <dxcapi.h>		
#include "WindowsAPI.h"

class DirectXCommon
{
public:
	// コンストラク
	DirectXCommon();

public://メンバ関数...初期化
	//初期化
	void Initialize(WindowsAPI* windowsAPI);
	//デバイス初期化
	void InitializeDevice();
	//コマンド関連の初期化
	void InitializeCommand();
	//レンダーターゲットビューの初期化
	void InitializeRTV();
	//深度ステンシルビューの初期化
	void InitializeDSV();
	//フェンスの初期化
	void InitializeFence();
	//ビューポート矩形の初期化
	void InitializeViewport();
	//シザリング矩形の初期化
	void InitializeScissorRect();
	//ImGuiの初期化
	void InitializeImGui();

public://メンバ関数...生成
	//スワップチェーンの生成
	void GenerateSwapChain();
	//深度バッファの生成
	void GenerateZBuffer();
	//各種デスクリプタヒープの生成
	void GenerateDescpitorHeap();
	//DXCコンパイラの生成
	void GenerateDXC();

private://色々な関数
	//指定番号のCPUディスクリプタハンドルを取得する
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	//指定番号のGPUディスクリプタハンドルを取得する
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

public:
	//SRVの指定番号のCPUディスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	//SRVの指定番号のGPUディスクリプタハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

public://描画関数
	//描画前処理
	void PreDraw();
	//描画後処理
	void PostDraw();

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
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;
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
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	//swapChainResources
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;
	//DSV
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	//fence
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	uint64_t fenceValue;
	HANDLE fenceEvent;
	//viewport
	D3D12_VIEWPORT viewport{};
	//scissorRect
	D3D12_RECT scissorRect{};
	//DXC
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
	//RTV	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	//barrier
	D3D12_RESOURCE_BARRIER barrier{};

private:
	//WindowsAPI
	WindowsAPI* windowsAPI = nullptr;
	//fence値
	UINT fenceVal = 0;
};