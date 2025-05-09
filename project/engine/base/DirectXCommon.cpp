#include "DirectXCommon.h"
#include "SrvManager.h"
#include <cassert>
#include <format>
#include "Logger.h"
#include "StringUtility.h"
#include "thread"
//#include "externals/imgui/imgui.h"
//#include "externals/imgui/imgui_impl_win32.h"
//#include "externals/imgui/imgui_impl_dx12.h"

#define ALIGN256(size) ((size + 255) & ~255)

using namespace StringUtility;
using namespace Logger;

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;

////最大テクスチャ枚数
//const uint32_t DirectXCommon::kMaxSRVCount = 512;

// コンストラクタの実装
//DirectXCommon::DirectXCommon()
//	: descriptorSizeSRV(0), descriptorSizeRTV(0), descriptorSizeDSV(0), fenceValue(0), fenceEvent(nullptr), windowsAPI(nullptr)
//{
//	// 初期化コードがあればここに記述
//}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height) {
	//生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;//Textureの幅
	resourceDesc.Height = height;//Textureの高さ
	resourceDesc.MipLevels = 1;//MipMapの数
	resourceDesc.DepthOrArraySize = 1;//奥行き or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//DepthStencilとして使う通知

	//利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;//VRAM上に作る

	//深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;//1.0f( 最大値 )でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//フォーマット。Resourceと合わせる。

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
		&resourceDesc,//Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度地を書き込む状態にしておく
		&depthClearValue,//Clear最適値
		IID_PPV_ARGS(&resource)//作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));

	return resource;
}

void DirectXCommon::CreateRootSignatureDX() {
	HRESULT hr;

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	

	// RootParameterの数を2に増やす（SRV + CBV）
	D3D12_ROOT_PARAMETER rootParameters[2]{};

	// SRV (t0)
	D3D12_DESCRIPTOR_RANGE srvRange{};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 1;
	srvRange.BaseShaderRegister = 0; // t0
	srvRange.RegisterSpace = 0;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &srvRange;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// CBV (b0)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].Descriptor.ShaderRegister = 0; // b0
	rootParameters[1].Descriptor.RegisterSpace = 0;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	// === Static Sampler (s0) ===
	D3D12_STATIC_SAMPLER_DESC staticSampler{};
	staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
	staticSampler.ShaderRegister = 0; // s0
	staticSampler.RegisterSpace = 0;
	staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = &staticSampler;
	descriptionRootSignature.NumStaticSamplers = 1;

	// シリアライズ
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	hr = D3D12SerializeRootSignature(
		&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob,
		&errorBlob
	);

	if (FAILED(hr)) {
		if (errorBlob) {
			Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		}
		assert(false);
	}

	// 生成
	hr = device->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)
	);
	assert(SUCCEEDED(hr));
}


void DirectXCommon::CreatePipelineStateDX()
{
	HRESULT hr;

	//呼び出し
	CreateRootSignatureDX();

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;;
	inputLayoutDesc.NumElements = 0;

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	D3D12_RASTERIZER_DESC resterizerDesc{};
	resterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	resterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = CompileShader(L"CopyImage.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = CompileShader(L"CopyImage.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効化する
	depthStencilDesc.DepthEnable = false;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	//比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
	graphicPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };
	graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };
	graphicPipelineStateDesc.BlendState = blendDesc;
	graphicPipelineStateDesc.RasterizerState = resterizerDesc;
	graphicPipelineStateDesc.NumRenderTargets = 1;
	graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicPipelineStateDesc.SampleDesc.Count = 1;
	graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//DepthStencilの設定
	graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = device->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
}

void DirectXCommon::CreateDepthSRV()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
	depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	depthSrvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += descriptorSizeSRV * 11; // 例: 深度SRVは11番目

	device->CreateShaderResourceView(depthStencilResource.Get(), &depthSrvDesc, handle);
}


Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	UINT numDescriptors,
	bool shaderVisible) {
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));

	return descriptorHeap;
}


void DirectXCommon::Initialize(WindowsAPI* windowsAPI)
{
	//FPS初期化固定
	InitializeFixFPS();

	//NULL検出
	assert(windowsAPI);

	//借りてきたWinAppのインスタンスを記録
	this->windowsAPI = windowsAPI;

	InitializeDevice();
	InitializeCommand();
	GenerateSwapChain();
	GenerateZBuffer();
	GenerateDescpitorHeap();
	GenerateDXC();
	InitializeRTV();
	InitializeDSV();
	InitializeFence();
	InitializeViewport();
	InitializeScissorRect();
	CreateRenderTextureReaourceRTV();
	CreateRenderTextureReaourceSRV();
	CreatePipelineStateDX();
	CreateDepthSRV();
}

void DirectXCommon::InitializeDevice()
{
	HRESULT hr;


#ifdef _DEBUG

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		//デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		//さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif



	//DXGIファクトリーの生成
	dxgiFactory = nullptr;

	//"HRESULTはWindows系のエラーコード"であり、
	//関数が成功したかどうかを SUCCEEDEDマクロ で判定できる	
	hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	//初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	//どうにも出来ない場合が多いので assert にしておく
	assert(SUCCEEDED(hr));


	//良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		//アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));//取得できないのは一大事

		//ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			//採用したアダプタの情報をログに出力。wstring の方なので注意
			Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;//ソフトウェアアダプタの場合は見なかったことにする
	}
	//適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);


	device = nullptr;
	//機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
	};

	const char* featureLevelStrings[] = { "12.2","12.1","12.0" };
	//高い順に生成できるか試していく
	for (size_t i = 0; _countof(featureLevels); ++i) {
		//採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));
		//指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr)) {
			//生成できたのでログ出力を行って
			Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	//デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	//初期化完了のログを出す
	Log("Complet create D3D12Device!!!\n");


#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		D3D12_MESSAGE_ID denyIds[] = { D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE };
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue->PushStorageFilter(&filter);
	}

#endif
}

void DirectXCommon::InitializeCommand()
{
	HRESULT hr;


#pragma region commandAllocator
	//コマンドアロケーターを生成する
	hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//コマンドアロケーターの生成がうまくいかなかったので起動出来ない
	assert(SUCCEEDED(hr));

#pragma endregion

#pragma region commandList

	//コマンドリストを生成する
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	//コマンドリストの生成がうまくいかなかったので起動出来ない
	assert(SUCCEEDED(hr));

#pragma endregion

#pragma region CommandQueue
	//コマンドキューを生成する
	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc{};
	hr = device->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//コマンドキューの生成がうまくいかなかったので起動出来ない
	assert(SUCCEEDED(hr));

#pragma endregion
}

void DirectXCommon::GenerateSwapChain()
{
	HRESULT hr;


#pragma region スワップチェーンの生成
	//スワップチェーンを生成する
	swapChainDesc.Width = WindowsAPI::kClientWidth;	//画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = WindowsAPI::kClientHeight;//画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	//色の形式
	swapChainDesc.SampleDesc.Count = 1;	//マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	//描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;	//ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	//モニタに写したら、中身を破棄
	//コマンドキュー、ウィンドウハンドル、設定を渡して生成する	
	hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue.Get(), windowsAPI->GetHwnd(), &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf()));
	assert(SUCCEEDED(hr));

#pragma endregion

}

void DirectXCommon::GenerateZBuffer() {
	// device と width, height を正しく設定
	int32_t width = WindowsAPI::kClientWidth;  // クライアント領域の幅
	int32_t height = WindowsAPI::kClientHeight; // クライアント領域の高さ

	// Zバッファ（深度ステンシルテクスチャ）を作成
	depthStencilResource = CreateDepthStencilTextureResource(device, width, height);
}

void DirectXCommon::GenerateDescpitorHeap()
{
	// 各ディスクリプタサイズを取得
	descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // SRVのサイズを追加

#pragma region ディスクリプタヒープの生成

	// RTV用ディスクリプタヒープ（最大4つ使用する可能性があるので4に）
	rtvDescriptorHeap = this->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4, false);
	// SRV用ディスクリプタヒープ（Shaderで使うためshaderVisibleはtrue）
	srvDescriptorHeap = this->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true); // 必要に応じて数は変更可能
	// DSV用ディスクリプタヒープ
	dsvDescriptorHeap = this->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

#pragma endregion
}

void DirectXCommon::GenerateDXC()
{
	HRESULT hr;

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

}

Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(const std::wstring& filePath, const wchar_t* profile)
{
	Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));

	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi", L"-Qembed_debug",
		L"-Od",
		L"-Zpr"
	};

	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	// includeHandlerから生のポインタを取得
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler.Get(), // ここを修正
		IID_PPV_ARGS(&shaderResult)
	);

	assert(SUCCEEDED(hr));

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		Log(shaderError->GetStringPointer());
		assert(false);
	}

	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

	return shaderBlob;
}


//Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes)
//{
//	HRESULT hr;
//
//	// 頂点リソース用のヒープの設定
//	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
//	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
//	// 頂点リソースの設定
//	D3D12_RESOURCE_DESC vertexResourceDesc{};
//	// バッファリソース
//	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	vertexResourceDesc.Width = sizeInBytes; // リソースのサイズ。今回はVector4を3頂点分
//	// バッファの場合はこれらは1にする決まり
//	vertexResourceDesc.Height = 1;
//	vertexResourceDesc.DepthOrArraySize = 1;
//	vertexResourceDesc.MipLevels = 1;
//	vertexResourceDesc.SampleDesc.Count = 1;
//	// バッファの場合はこれにする決まり
//	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//	// 実際に頂点リソースを作る
//	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
//	hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
//		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
//		IID_PPV_ARGS(&vertexResource));
//	assert(SUCCEEDED(hr));
//
//	return vertexResource;
//}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes)
{
	HRESULT hr;

	// **256 バイト単位に揃える**
	sizeInBytes = ALIGN256(sizeInBytes);

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes; // **256 バイトアライメント済み**
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際に頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
	hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexResource)
	);
	assert(SUCCEEDED(hr));

	return vertexResource;
}


Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata)
{
	//metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);//Textureの幅
	resourceDesc.Height = UINT(metadata.height);//Textureの高さ
	resourceDesc.MipLevels = UINT(metadata.mipLevels);//mipmapの数
	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);//奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format;//TextureのFormat
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);//Textureの次元数。

	//利用するHeapの作成。非常に特殊な運用。
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;//細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//プロセッサの近くに配置

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
		&resourceDesc,//Resouceの設定
		D3D12_RESOURCE_STATE_GENERIC_READ,//初回のResourceState。	Textureは基本読むだけ
		nullptr,//Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)//作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

void DirectXCommon::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages)
{
	//Meta情報を取得
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	//全MipMapについて
	for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
		//
		const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
		//
		HRESULT hr = texture->WriteToSubresource(
			UINT(mipLevel),
			nullptr,//全領域へコピー
			img->pixels,//元データアドレス
			UINT(img->rowPitch),//1ラインサイズ
			UINT(img->slicePitch)//1枚サイズ
		);
		assert(SUCCEEDED(hr));
	}
}

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateRenderTextureResource(
	Microsoft::WRL::ComPtr<ID3D12Device> device,
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	const Vector4& clearColor)
{
	// Resourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = width;
	resourceDesc.Height = height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	// Heapの設定（VRAM）
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// クリアカラー
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor.x;
	clearValue.Color[1] = clearColor.y;
	clearValue.Color[2] = clearColor.z;
	clearValue.Color[3] = clearColor.w;

	// リソース作成
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTexture;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&renderTexture)
	);
	assert(SUCCEEDED(hr));

	return renderTexture;
}

void DirectXCommon::CreateRenderTextureReaourceRTV()
{
	const Vector4 kRenderTargetClearValue{ 1.0f, 0.0f, 0.0f, 1.0f };

	renderTextureResource = CreateRenderTextureResource(
		device,
		WindowsAPI::kClientWidth,
		WindowsAPI::kClientHeight,
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		kRenderTargetClearValue
	);

	renderTextureResource->SetName(L"RenderTexture");

	// RTVヒープの3番目にRenderTextureのハンドルを割り当てる（0,1はswapchain用）
	rtvHandles[2] = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	rtvHandles[2].ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 2;

	device->CreateRenderTargetView(renderTextureResource.Get(), &rtvDesc, rtvHandles[2]);
}


void DirectXCommon::CreateRenderTextureReaourceSRV()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC renderTextureSrvDesc{};
	renderTextureSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	renderTextureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	renderTextureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	renderTextureSrvDesc.Texture2D.MipLevels = 1;

	// 例：SRVの10番にRenderTextureを割り当てる（0〜9は他テクスチャに使う前提）
	uint32_t renderTextureSRVIndex = 10;
	D3D12_CPU_DESCRIPTOR_HANDLE handle = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += descriptorSizeSRV * renderTextureSRVIndex;

	device->CreateShaderResourceView(renderTextureResource.Get(), &renderTextureSrvDesc, handle);
}

//DirectX::ScratchImage DirectXCommon::LoadTexture(const std::string& filePath)
//{
//	//テクスチャファイルを読んでプログラムを扱えるようにする
//	DirectX::ScratchImage image{};
//	std::wstring filePathW = ConvertString(filePath);
//	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
//	assert(SUCCEEDED(hr));
//
//	//MipMapの作成
//	DirectX::ScratchImage mipImages{};
//	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
//	assert(SUCCEEDED(hr));
//
//	//MipMap付きのデータを返す
//	return mipImages;
//}

//D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
//{
//	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	handleCPU.ptr += (descriptorSize * index);
//	return handleCPU;
//}
//
//D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
//{
//	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
//	handleGPU.ptr += (descriptorSize * index);
//	return handleGPU;
//}

void DirectXCommon::InitializeRTV()
{
	HRESULT hr;

	rtvHeap_ = this->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4, false);

#pragma region SwapChainからResourceを引っ張てくる

	//SwapChainからResourceを引っ張てくる
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResource[0]));
	swapChainResource[0]->SetName(L"SwapChainResource0");
	//うまく取得できなければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResource[1]));
	swapChainResource[1]->SetName(L"SwapChainResource1");
	assert(SUCCEEDED(hr));

#pragma endregion

	//RTVの設定
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;	//2dテスクチャとして書き込む
	//ディスクリプタの先頭を取得する
	//rtvStartHandle = GetCPUDescriptorHandle(rtvDescriptorHeap.Get(), descriptorSizeRTV, 0);
	rtvStartHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	//裏表の2つ分

		//RTVを2つ作るのでディスクリプタを2つ用意
		// rtvHandles[0] に最初のデスクリプタハンドルを設定
	rtvHandles[0] = rtvStartHandle;

	// デスクリプタのサイズを取得
	UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// rtvHandles[1] に、最初のハンドルからのオフセットを設定
	rtvHandles[1].ptr = rtvHandles[0].ptr + descriptorSize;

	//2つ目を作る
	device->CreateRenderTargetView(swapChainResource[0].Get(), &rtvDesc, rtvHandles[0]);
	device->CreateRenderTargetView(swapChainResource[1].Get(), &rtvDesc, rtvHandles[1]);

}

void DirectXCommon::InitializeDSV()
{
	//DepthStencilTextureをウィンドウのサイズで作成
	depthStencilResource = CreateDepthStencilTextureResource(device.Get(), WindowsAPI::kClientWidth, WindowsAPI::kClientHeight);

	//DSV用のHeapでDiscriptorの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	dsvDescriptorHeap = this->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	//DSVの設定
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;//Format。基本的にはResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;//2dTexture
	//DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::InitializeFence()
{
	HRESULT hr;

	//フェンスの生成
	fence = nullptr;
	fenceValue = 0;
	hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXCommon::InitializeViewport()
{
	//ビューポート矩形の設定
	viewport.Width = WindowsAPI::kClientWidth;
	viewport.Height = WindowsAPI::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
}

void DirectXCommon::InitializeScissorRect()
{
	//シザリング矩形の設定
	scissorRect.left = 0;
	scissorRect.right = WindowsAPI::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WindowsAPI::kClientHeight;
}

//void DirectXCommon::InitializeImGui()
//{
	////Imguiの初期化
	//IMGUI_CHECKVERSION();
	//ImGui::CreateContext();
	//ImGui::StyleColorsDark();
	//ImGui_ImplWin32_Init(windowsAPI->GetHwnd());
	//ImGui_ImplDX12_Init(device.Get(),
	//	swapChainDesc.BufferCount,
	//	rtvDesc.Format,
	//	srvDescriptorHeap.Get(),
	//	srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
	//	srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	//);
//}

void DirectXCommon::InitializeFixFPS()
{

	//現在時間を記録する
	reference_ = std::chrono::steady_clock::now();

}

void DirectXCommon::UpdateFixFPS()
{

	// 1/60秒ぴったりの時間
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));
	// 1/60秒よりわずかに短い時間
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	//現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	//前回記録からの経過時間を取得
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);
	// 1/60秒( よりわずかに短い時間 )経っていない場合
	if (elapsed < kMinCheckTime) {
		// 1/60秒経過するまで微小なスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			//1マイクロ秒スリープ
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}
	//現在の時間を記録する
	reference_ = std::chrono::steady_clock::now();

}

//D3D12_CPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVCPUDescriptorHandle(uint32_t index)
//{
//	return GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, index);
//}
//
//D3D12_GPU_DESCRIPTOR_HANDLE DirectXCommon::GetSRVGPUDescriptorHandle(uint32_t index)
//{
//	return GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, index);
//}

void DirectXCommon::PreDraw()
{
	
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	commandList->OMSetRenderTargets(1, &rtvHandles[2], false, &dsvHandle);

	float clearRenderTextureColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandles[2], clearRenderTextureColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	// === Object描画ここで行う ===
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	barrier.Transition.pResource = swapChainResource[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList->ResourceBarrier(1, &barrier);

	//commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);

	//float clearSwapChainColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };//背景色
	//commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearSwapChainColor, 0, nullptr);

	/*commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);*/
}


void DirectXCommon::PostDraw()
{
	HRESULT hr;
	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);


	assert(rootSignature != nullptr);
	assert(graphicsPipelineState != nullptr);
	assert(srvDescriptorHeap != nullptr);

	// ==========================
	// ① RenderTextureをSRVとして使用（ポストエフェクト描画）
	// ==========================
	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetPipelineState(graphicsPipelineState.Get());

	// SRVヒープのバインド
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// SRV（t0）にRenderTextureの10番目をバインド
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += descriptorSizeSRV * 10;
	commandList->SetGraphicsRootDescriptorTable(0, gpuHandle);

	// ✅ RenderTexture を PIXEL_SHADER_RESOURCE 状態に遷移
	//if (renderTextureState != D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) {
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTextureResource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	//barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
	renderTextureState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	//}
	// ポストエフェクト用のフルスクリーン描画
	commandList->DrawInstanced(3, 1, 0, 0);


	// ==========================
	// ② SwapchainをPresentに遷移
	// ==========================

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResource[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	//barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);

	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = renderTextureResource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
	renderTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;

	//// ✅ 次フレームの PreDraw で再び RenderTarget として使えるように戻しておく
	////if (renderTextureState != D3D12_RESOURCE_STATE_RENDER_TARGET) {
	//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	//	barrier.Transition.pResource = renderTextureResource.Get();
	//	barrier.Transition.StateBefore = renderTextureState;
	//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//	//barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//	commandList->ResourceBarrier(1, &barrier);
	//	renderTextureState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	////}

	// ==========================
	// ③ コマンドリスト終了・実行・スワップ
	// ==========================

	hr = commandList->Close();
	assert(SUCCEEDED(hr));

	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(1, commandLists);

	swapChain->Present(1, 0);

	// フェンスで待機
	fenceValue++;
	hr = commandQueue->Signal(fence.Get(), fenceValue);
	assert(SUCCEEDED(hr));

	UpdateFixFPS();

	if (fence->GetCompletedValue() < fenceValue) {
		hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
		assert(SUCCEEDED(hr));
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	// コマンドの再利用準備
	hr = commandAllocator->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	assert(SUCCEEDED(hr));
}






