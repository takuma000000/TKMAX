#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <dxcapi.h>
#include <string>
#include <chrono>
#include "WindowsAPI.h"
#include "SrvManager.h"
#include "externals/DirectXTex/DirectXTex.h"//DirectX
#include "MyMath.h"
#include "SystemIncludes.h"

class DirectXCommon
{
public:
	struct OutlineParameter {
		Matrix4x4 projectionInverse;
	};

	// -------------------- 初期化 --------------------
	void Initialize(WindowsAPI* windowsAPI);
	void InitializeDevice();
	void InitializeCommand();
	void InitializeRTV();
	void InitializeDSV();
	void InitializeFence();
	void InitializeViewport();
	void InitializeScissorRect();

	// -------------------- 描画 --------------------
	void PreDraw();
	void PostDraw();
	void ImGuiDebug();

	// -------------------- デスクリプタヒープ生成 --------------------
	void GenerateSwapChain();
	void GenerateZBuffer();
	void GenerateDescpitorHeap();
	void GenerateDXC();

	// -------------------- リソース生成 --------------------
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device>, uint32_t width, uint32_t height, DXGI_FORMAT format, const Vector4& clearColor);
	void CreateRenderTextureResourceRTV();
	void CreateDepthSRV();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	// -------------------- シェーダ関連 --------------------
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);
	void CreateRootSignatureDX();
	void CreatePipelineStateDX();

	// -------------------- Getter --------------------
	ID3D12Device* GetDevice() const { return device.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
	D3D12_VIEWPORT GetViewport() const { return viewport; }
	D3D12_RECT GetRect() const { return scissorRect; }
	uint32_t GetDescriptorSizeRTV() const { return descriptorSizeRTV; }
	uint32_t GetDescriptorSizeDSV() const { return descriptorSizeDSV; }
	size_t GetBackBufferCount() const { return backBufferChange; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const {
		return dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const {
		UINT index = swapChain->GetCurrentBackBufferIndex();
		return rtvHandles[index];
	}

private:
	void InitializeFixFPS();
	void UpdateFixFPS();

	// -------------------- DirectX関連 --------------------
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	D3D12_RESOURCE_BARRIER barrier{};

	// -------------------- 描画状態 --------------------
	D3D12_RESOURCE_STATES renderTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// -------------------- デスクリプタサイズ --------------------
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;
	uint32_t descriptorSizeSRV = 0;

	// -------------------- ImGui/RTV --------------------
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[4]{};

	// -------------------- DXC --------------------
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;

	// -------------------- Root & PSO --------------------
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

	// -------------------- ConstantBuffer --------------------
	Microsoft::WRL::ComPtr<ID3D12Resource> outlineConstantBuffer_;
	OutlineParameter* outlineMappedData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> thresholdBuffer_;
	struct ThresholdParam { float threshold; float padding[3]; };
	ThresholdParam* thresholdMappedData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource;

	// -------------------- 時間計測 --------------------
	std::chrono::steady_clock::time_point reference_;
	uint64_t fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	// -------------------- 外部参照 --------------------
	WindowsAPI* windowsAPI = nullptr;
	SrvManager* srvManager_ = nullptr;

	// -------------------- 定数 --------------------
	uint32_t backBufferChange = 2;
	UINT fenceVal = 0;
	static constexpr uint32_t kRenderTextureSRVIndex = 10;
	static constexpr uint32_t kRenderTextureRTVIndex = 2;
	static constexpr uint32_t kDepthSRVIndex = 11;
};
