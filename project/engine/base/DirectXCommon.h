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

class RadialBlurEffect;

//=============================================================
// DirectXCommonクラス
// DirectX12の初期化・描画・リソース管理を行うクラス。
//=============================================================
class DirectXCommon {
public:

	// OutlineParameter構造体
	struct OutlineParameter {
		Matrix4x4 projectionInverse;
	};

	// -------------------- 初期化 --------------------
	///<param name="windowsAPI">WindowsAPIクラスのポインタ</param>
	///<summary>DirectXCommonの初期化を行う関数</summary>
	void Initialize(WindowsAPI* windowsAPI);
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void InitializeDevice();
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void InitializeCommand();
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void InitializeRTV();
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void InitializeDSV();
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void InitializeFence();
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void InitializeViewport();
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void InitializeScissorRect();

	// -------------------- 描画 --------------------
	///<summary>描画前処理を行う関数</summary>
	void PreDraw();
	///<summary>描画後処理を行う関数</summary>
	void PostDraw();
	///<summary>DirectXCommonの終了処理を行う関数</summary>
	void ImGuiDebug();

	// -------------------- デスクリプタヒープ生成 --------------------
	///<summary>スワップチェーンの生成を行う関数</summary>
	void GenerateSwapChain();
	///<summary>レンダーターゲットビューの生成を行う関数</summary>
	void GenerateZBuffer();
	///<summary>デスクリプタヒープの生成を行う関数</summary>
	void GenerateDescpitorHeap();
	///<summary>DXCの生成を行う関数</summary>
	void GenerateDXC();

	// -------------------- リソース生成 --------------------
	///<summary>バッファリソースの生成を行う関数</summary>
	///<param name="sizeInBytes">バッファサイズ（バイト単位）</param>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
	///<summary>テクスチャリソースの生成を行う関数</summary>
	///<param name="metadata">テクスチャメタデータ</param>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
	///<summary>テクスチャデータのアップロードを行う関数</summary>
	///<param name="texture">テクスチャリソース</param>
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
	///<summary>デスクリプタヒープの生成を行う関数</summary>
	///<param name="heapType">ヒープタイプ</param>
	///<param name="numDescriptors">デスクリプタ数</param>
	///<param name="shaderVisible">シェーダから見えるかどうか</param>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device,
		int width, int height, DXGI_FORMAT format, const Vector4& clearColor);

	// CreateRenderTextureRTV関数
	void CreateRenderTextureRTV();

	///<summary>Swapchain に描き始める（ImGui 用）</summary>
	void BeginDrawToSwapchain();

	///<summary>CopyImage 用パイプライン初期化</summary>
	void InitializeCopyImagePipeline();

	///<summary>RenderTexture → Swapchain へコピー描画</summary>
	void DrawRenderTextureToSwapchain();

	///<summary>RadialBlur 用パイプライン初期化</summary>
	void InitializeRadialBlurPipeline();

	///<summary>RadialBlur 付きで RenderTexture → Swapchain へコピー描画</summary>
	void DrawRadialBlurToSwapchain();

	/// RadialBlurEffect を登録（シーンから渡す）
	void SetRadialBlurEffect(RadialBlurEffect* effect) { radialBlurEffect_ = effect; }

	/// 現在の RadialBlurEffect を取得（必要なら）
	RadialBlurEffect* GetRadialBlurEffect() const { return radialBlurEffect_; }

	/// 「今の状態に応じて」RenderTexture → Swapchain をコピー
	///   - RadialBlur が有効なら RadialBlur で
	///   - そうでなければ通常コピー
	void DrawPostEffectToSwapchain();

	// -------------------- シェーダ関連 --------------------
	///<summary>シェーダのコンパイルを行う関数</summary>
	///<param name="filePath">シェーダファイルのパス</param>
	///<param name="profile">シェーダプロファイル</param>
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);
	// -------------------- Getter --------------------
	///<summary>デバイスのゲッター</summary>
	ID3D12Device* GetDevice() const { return device.Get(); }
	///<summary>コマンドリストのゲッター</summary>
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
	///<summary>コマンドキューのゲッター</summary>
	D3D12_VIEWPORT GetViewport() const { return viewport; }
	///<summary>シザー矩形のゲッター</summary>
	D3D12_RECT GetRect() const { return scissorRect; }
	///<summary>デスクリプタサイズのゲッター</summary>
	uint32_t GetDescriptorSizeRTV() const { return descriptorSizeRTV; }
	///<summary>デスクリプタサイズのゲッター</summary>
	uint32_t GetDescriptorSizeDSV() const { return descriptorSizeDSV; }
	///<summary>デスクリプタサイズのゲッター</summary>
	size_t GetBackBufferCount() const { return backBufferChange; }
	///<summary>DSVハンドルのゲッター</summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const {
		return dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}
	///<summary>現在のRTVハンドルのゲッター</summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const {
		UINT index = swapChain->GetCurrentBackBufferIndex();
		return rtvHandles[index];
	}

	/// <summary>SrvManager を登録する</summary>
	void SetSrvManager(SrvManager* srvManager) { srvManager_ = srvManager; }

	/// <summary>RenderTexture 用 SRV インデックスを取得する</summary>
	uint32_t GetRenderTextureSrvIndex() const { return renderTextureSrvIndex_; }

private:
	///<summary>固定FPS制御の初期化を行う関数</summary>
	void InitializeFixFPS();
	///<summary>固定FPS制御の更新を行う関数</summary>
	void UpdateFixFPS();

	// -------------------- DirectX関連 --------------------
	Microsoft::WRL::ComPtr<ID3D12Device> device; // D3D12デバイス
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory; // DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain; // スワップチェーン
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController; // デバッグコントローラ
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter; // 使用アダプタ

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
	Microsoft::WRL::ComPtr<ID3D12RootSignature> copyImageRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> copyImagePipelineState_;
	bool copyImageInitialized_ = false;
	// RadialBlur 用 PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> radialBlurPipelineState_;
	bool radialBlurInitialized_ = false;
	// 現在シーンの RadialBlurEffect（なければ nullptr）
	RadialBlurEffect* radialBlurEffect_ = nullptr;
	// -------------------- ConstantBuffer --------------------
	Microsoft::WRL::ComPtr<ID3D12Resource> outlineConstantBuffer_;
	OutlineParameter* outlineMappedData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> thresholdBuffer_;
	// ThresholdParam構造体
	struct ThresholdParam { float threshold; float padding[3]; }; // 16バイトアライメントのためにパディングを追加

	ThresholdParam* thresholdMappedData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource;
	// RenderTexture 用 SRV のインデックス
	uint32_t renderTextureSrvIndex_ = 0;

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
	static constexpr uint32_t kRenderTextureRTVIndex = 2;
	static constexpr uint32_t kDepthSRVIndex = 11;
};
