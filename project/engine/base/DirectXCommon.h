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

// PostEffect
class RadialBlurEffect;
class VignettingEffect;
class WaterRippleEffect;
class FogEffect;

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

	// VignettingCB構造体
	struct VignettingCB {
		Vector4 color;     // 枠色
		float   intensity; // 強度
		float   radius;    // どこから暗くするか
		float   softness;  // ふちのボケ具合
		float   padding;   // 16byte アライメント
	};
	// WaterRippleCB構造体
	struct WaterRippleCB {
		Vector2 center;    // 波紋中心 (UV)
		float   radius;    // 現在の半径
		float   amplitude; // ズレの強さ
		float   frequency; // 波の細かさ
		float   width;     // 帯の幅
		float  padding;   // アライメント
		Vector3 color;   // 波紋色
		float   colorIntensity; // 波紋色の強さ
	};
	// FogCB構造体
	struct FogCB {
		Vector3 FogColor;     // 霧の色
		float   FogDensity;   // 全体の濃さ
		float   FogStart;     // 霧開始の高さ (0〜1)
		float   FogEnd;       // 霧最大の高さ (0〜1)
		float   NoiseScale;   // ノイズの細かさ
		float   NoiseStrength;// 濃さのムラの強さ
		float   Time;         // 経過時間
		float   worldScale;   // 霧パターンの「世界空間スケール」
		Vector3 worldPos;     // カメラ or プレイヤーのワールド座標
		float   padding2;     // 16byte整列用
	};

	// -------------------- 初期化 --------------------
	/// <summary>
	/// DirectXCommonの初期化を行う関数
	/// </summary>
	/// <param name="windowsAPI"></param>
	void Initialize(WindowsAPI* windowsAPI);
	/// <summary>
	/// DirectXCommonの終了処理を行う関数
	/// </summary>
	void InitializeDevice();
	/// <summary>
	/// DirectXCommonの終了処理を行う関数
	/// </summary>
	void InitializeCommand();
	/// <summary>
	/// DirectXCommonの終了処理を行う関数
	/// </summary>
	void InitializeRTV();
	/// <summary>
	/// DirectXCommonの終了処理を行う関数
	/// </summary>
	void InitializeDSV();
	/// <summary>
	/// DirectXCommonの終了処理を行う関数
	/// </summary>
	void InitializeFence();
	/// <summary>
	/// DirectXCommonの終了処理を行う関数
	/// </summary>
	void InitializeViewport();
	/// <summary>
	/// DirectXCommonの終了処理を行う関数
	/// </summary>
	void InitializeScissorRect();
	// -------------------- 描画 --------------------
	/// <summary>
	/// DirectXCommonの描画前処理を行う関数
	/// </summary>
	void PreDraw();
	/// <summary>
	/// DirectXCommonの描画後処理を行う関数
	/// </summary>
	void PostDraw();
	// -------------------- デスクリプタヒープ生成 --------------------
	/// <summary>
	/// スワップチェーンの生成を行う関数
	/// </summary>
	void GenerateSwapChain();
	/// <summary>
	/// レンダーターゲットビューの生成を行う関数
	/// </summary>
	void GenerateZBuffer();
	/// <summary>
	/// デスクリプタヒープの生成を行う関数
	/// </summary>
	void GenerateDescpitorHeap();
	/// <summary>
	/// DXCの初期化を行う関数
	/// </summary>
	void GenerateDXC();
	// -------------------- リソース生成 --------------------
	/// <summary>
	/// バッファリソースの生成を行う関数
	/// </summary>
	/// <param name="sizeInBytes"></param>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);
	/// <summary>
	/// テクスチャリソースの生成を行う関数
	/// </summary>
	/// <param name="metadata"></param>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);
	/// <summary>
	/// テクスチャデータのアップロードを行う関数
	/// </summary>
	/// <param name="texture"></param>
	/// <param name="mipImages"></param>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
	/// <summary>
	/// デスクリプタヒープの生成を行う関数
	/// </summary>
	/// <param name="heapType"></param>
	/// <param name="numDescriptors"></param>
	/// <param name="shaderVisible"></param>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	/// <summary>
	/// レンダーターゲット用テクスチャリソースの生成を行う関数
	/// </summary>
	/// <param name="device"></param>
	/// <param name="width"></param>
	/// <param name="height"></param>
	/// <param name="format"></param>
	/// <param name="clearColor"></param>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device,
		int width, int height, DXGI_FORMAT format, const Vector4& clearColor);
	/// <summary>
	/// RenderTexture 用 RTV 作成
	/// </summary>
	void CreateRenderTextureRTV();
	/// <summary>
	/// ポストエフェクトチェーン用：RadialBlur適用
	/// </summary>
	/// <param name="inputTex"></param>
	/// <param name="inputSrvIndex"></param>
	/// <param name="outputTex"></param>
	/// <param name="outputRtv"></param>
	void ApplyRadialBlur(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
	/// <summary>
	/// ポストエフェクトチェーン用：WaterRipple適用
	/// </summary>
	/// <param name="inputTex"></param>
	/// <param name="inputSrvIndex"></param>
	/// <param name="outputTex"></param>
	/// <param name="outputRtv"></param>
	void ApplyWaterRipple(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
	/// <summary>
	/// ポストエフェクトチェーン用：Vignetting適用
	/// </summary>
	/// <param name="inputTex"></param>
	/// <param name="inputSrvIndex"></param>
	/// <param name="outputTex"></param>
	/// <param name="outputRtv"></param>
	void ApplyVignetting(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);
	/// <summary>
	/// ポストエフェクトチェーン用：Fog適用
	/// </summary>
	/// <param name="inputTex"></param>
	/// <param name="inputSrvIndex"></param>
	/// <param name="outputTex"></param>
	/// <param name="outputRtv"></param>
	void ApplyFog(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv);

	/// <summary>
	/// テクスチャ → Swapchain へコピー描画
	/// </summary>
	/// <param name="inputTex"></param>
	/// <param name="inputSrvIndex"></param>
	void DrawTextureToSwapchain(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex);
	/// <summary>
	/// RenderTexture 用 SRV 作成
	/// </summary>
	void BeginDrawToSwapchain();
	/// <summary>
	/// RenderTexture 用 SRV 作成
	/// </summary>
	void InitializeCopyImagePipeline();
	/// <summary>
	/// RenderTexture → Swapchain へコピー描画
	/// </summary>
	void DrawRenderTextureToSwapchain();
	/// <summary>
	/// RadialBlur パイプラインの初期化
	/// </summary>
	void InitializeRadialBlurPipeline();
	/// <summary>
	/// Vignetting パイプラインの初期化
	/// </summary>
	void InitializeVignettingPipeline();
	/// <summary>
	/// WaterRipple パイプラインの初期化
	/// </summary>
	void InitializeWaterRipplePipeline();
	/// <summary>
	/// Fog パイプラインの初期化
	/// </summary>
	void InitializeFogPipeline();
	/// <summary>
	/// ポストエフェクトなしで RenderTexture → Swapchain へ描画
	/// </summary>
	void DrawPostEffectToSwapchain();
	/// <summary>
	/// シェーダのコンパイルを行う関数
	/// </summary>
	/// <param name="filePath"></param>
	/// <param name="profile"></param>
	/// <returns></returns>
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

	// Getter==================================================================
	/// <summary>
	/// D3D12デバイスのゲッター
	/// </summary>
	/// <returns></returns>
	ID3D12Device* GetDevice() const { return device.Get(); }
	/// <summary>
	/// コマンドキューのゲッター
	/// </summary>
	/// <returns></returns>
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }
	/// <summary>
	/// ビューポートのゲッター
	/// </summary>
	/// <returns></returns>
	D3D12_VIEWPORT GetViewport() const { return viewport; }
	/// <summary>
	/// シザー矩形のゲッター
	/// </summary>
	/// <returns></returns>
	D3D12_RECT GetRect() const { return scissorRect; }
	/// <summary>
	/// デスクリプタサイズのゲッター
	/// </summary>
	/// <returns></returns>
	uint32_t GetDescriptorSizeRTV() const { return descriptorSizeRTV; }
	/// <summary>
	/// デスクリプタサイズのゲッター
	/// </summary>
	/// <returns></returns>
	uint32_t GetDescriptorSizeDSV() const { return descriptorSizeDSV; }
	/// <summary>
	/// デスクリプタサイズのゲッター
	/// </summary>
	/// <returns></returns>
	size_t GetBackBufferCount() const { return backBufferChange; }
	/// <summary>
	/// DSVハンドルのゲッター
	/// </summary>
	/// <returns></returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const {
		return dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	}
	/// <summary>
	/// 現在のRTVハンドルのゲッター
	/// </summary>
	/// <returns></returns>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTVHandle() const {
		UINT index = swapChain->GetCurrentBackBufferIndex();
		return rtvHandles[index];
	}
	/// <summary>
	/// RenderTexture 用 SRV インデックスのゲッター
	/// </summary>
	/// <returns></returns>
	uint32_t GetRenderTextureSrvIndex() const { return renderTextureSrvIndex_; }
	/// <summary>
	/// RadialBlurEffect を取得
	/// </summary>
	/// <returns></returns>
	RadialBlurEffect* GetRadialBlurEffect() const { return radialBlurEffect_; }
	// ========================================================================
	// Setter==================================================================
	/// <summary>
	/// SrvManager をセット
	/// </summary>
	/// <param name="srvManager"></param>
	void SetSrvManager(SrvManager* srvManager) { srvManager_ = srvManager; }
	/// <summary>
	/// RadialBlurEffect をセット（必要なら）
	/// </summary>
	/// <param name="effect"></param>
	void SetRadialBlurEffect(RadialBlurEffect* effect) { radialBlurEffect_ = effect; }
	/// <summary>
	/// VignettingEffect をセット（必要なら）
	/// </summary>
	/// <param name="effect"></param>
	void SetVignettingEffect(VignettingEffect* effect) { vignettingEffect_ = effect; }
	/// <summary>
	/// Vignetting 用 パラメータセット
	/// </summary>
	/// <param name="color"></param>
	/// <param name="intensity"></param>
	/// <param name="radius"></param>
	/// <param name="softness"></param>
	void SetVignettingParam(const Vector4& color, float intensity, float radius, float softness);
	/// <summary>
	/// WaterRippleEffect をセット（必要なら）
	/// </summary>
	/// <param name="effect"></param>
	void SetWaterRippleEffect(WaterRippleEffect* effect) { rippleEffect_ = effect; }
	/// <summary>
	/// WaterRipple 用 パラメータセット
	/// </summary>
	/// <param name="centerUV"></param>
	/// <param name="radius"></param>
	/// <param name="amplitude"></param>
	/// <param name="frequency"></param>
	/// <param name="width"></param>
	void SetWaterRippleParam(const Vector2& centerUV, float radius, float amplitude, float frequency, float width, const Vector3& color, float colorIntensity);
	/// <summary>
	/// FogEffect をセット（必要なら）
	/// </summary>
	/// <param name="effect"></param>
	void SetFogEffect(FogEffect* effect) { fogEffect_ = effect; }
	/// <summary>
	/// Fog 用 パラメータセット
	/// </summary>
	/// <param name="color"></param>
	/// <param name="density"></param>
	/// <param name="start"></param>
	/// <param name="end"></param>
	/// <param name="noiseScale"></param>
	/// <param name="noiseStrength"></param>
	/// <param name="time"></param>
	/// <param name="worldPos"></param>
	/// <param name="worldScale"></param>
	void SetFogParam(const Vector3& color, float density,
		float start, float end,
		float noiseScale, float noiseStrength,
		float time,
		const Vector3& worldPos, float worldScale);
	// ========================================================================
private:
	//======================================================================
	// 固定FPS制御
	//======================================================================
	/// <summary>
	/// 固定FPS制御の初期化を行う関数
	/// </summary>
	void InitializeFixFPS();
	/// <summary>
	/// 固定FPS制御の更新を行う関数
	/// </summary>
	void UpdateFixFPS();
	// -------------------- 時間計測 --------------------
	std::chrono::steady_clock::time_point reference_;
	//======================================================================
	// DirectX関連コア（デバイス / ファクトリ / スワップチェーン）
	//======================================================================
	// -------------------- DirectX関連 --------------------
	Microsoft::WRL::ComPtr<ID3D12Device>       device;       // D3D12デバイス
	Microsoft::WRL::ComPtr<IDXGIFactory7>      dxgiFactory;  // DXGIファクトリ
	Microsoft::WRL::ComPtr<IDXGISwapChain4>    swapChain;    // スワップチェーン
	Microsoft::WRL::ComPtr<ID3D12Debug1>       debugController; // デバッグコントローラ
	Microsoft::WRL::ComPtr<IDXGIAdapter4>      useAdapter;   // 使用アダプタ

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>          commandQueue;
	//======================================================================
	// ヒープ / フェンス / レンダーターゲット / 深度
	//======================================================================
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Fence>          fence;

	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;

	DXGI_SWAP_CHAIN_DESC1          swapChainDesc{};
	D3D12_RENDER_TARGET_VIEW_DESC  rtvDesc{};
	D3D12_DEPTH_STENCIL_VIEW_DESC  dsvDesc{};
	D3D12_VIEWPORT                 viewport{};
	D3D12_RECT                     scissorRect{};
	D3D12_RESOURCE_BARRIER         barrier{};
	// -------------------- 描画状態 --------------------
	D3D12_RESOURCE_STATES renderTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// -------------------- デスクリプタサイズ --------------------
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;
	uint32_t descriptorSizeSRV = 0;
	// -------------------- ImGui/RTV --------------------
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE                  rtvStartHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE                  rtvHandles[4]{};
	//======================================================================
	// DXC（シェーダコンパイラ）
	//======================================================================
	// -------------------- DXC --------------------
	Microsoft::WRL::ComPtr<IDxcUtils>          dxcUtils;
	Microsoft::WRL::ComPtr<IDxcCompiler3>      dxcCompiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	//======================================================================
	// RootSignature / PipelineState
	//======================================================================
	// -------------------- Root & PSO --------------------
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>  graphicsPipelineState;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  copyImageRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>  copyImagePipelineState_;
	bool                                          copyImageInitialized_ = false;

	// RadialBlur 用 PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> radialBlurPipelineState_;
	bool                                         radialBlurInitialized_ = false;
	RadialBlurEffect* radialBlurEffect_ = nullptr; // 現在シーンの RadialBlurEffect（なければ nullptr）

	// Vignetting 用 PSO
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  vignettingRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>  vignettingPipelineState_;
	bool                                         vignettingInitialized_ = false;
	VignettingEffect* vignettingEffect_ = nullptr; // 現在シーンの VignettingEffect（なければ nullptr）
	Microsoft::WRL::ComPtr<ID3D12Resource> vignettingConstantBuffer_; // Vignetting 用 定数バッファ
	void* vignettingMappedData_ = nullptr; // Vignetting 用 定数バッファマッピングデータポインタ

	// WaterRipple 用 PSO
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  rippleRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>  ripplePipelineState_;
	bool                                         rippleInitialized_ = false;
	WaterRippleEffect* rippleEffect_ = nullptr; // 現在シーンの WaterRippleEffect
	Microsoft::WRL::ComPtr<ID3D12Resource> rippleConstantBuffer_; // Ripple 用 定数バッファ
	void* rippleMappedData_ = nullptr; // Ripple 用 定数バッファマッピングデータポインタ

	// Fog 用 PSO
	FogEffect* fogEffect_ = nullptr;
	bool fogInitialized_ = false;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>  fogRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>  fogPipelineState_;
	Microsoft::WRL::ComPtr<ID3D12Resource>       fogConstantBuffer_;
	void* fogMappedData_ = nullptr;
	//======================================================================
	// 定数バッファ / ポストエフェクト関連リソース
	//======================================================================
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

	// ポストエフェクト用 ping-pong テクスチャ
	Microsoft::WRL::ComPtr<ID3D12Resource> postEffectTextureResource;
	uint32_t postEffectSrvIndex_ = 0;
	//======================================================================
	// フェンス / シンクロ
	//======================================================================
	uint64_t fenceValue = 0;
	HANDLE   fenceEvent = nullptr;
	UINT     fenceVal = 0;
	//======================================================================
	// 外部参照 / 定数
	//======================================================================
	// -------------------- 外部参照 --------------------
	WindowsAPI* windowsAPI = nullptr;
	SrvManager* srvManager_ = nullptr;
	// -------------------- 定数 --------------------
	uint32_t               backBufferChange = 2;
	static constexpr uint32_t kRenderTextureRTVIndex = 2;
	static constexpr uint32_t kDepthSRVIndex = 11;
};
