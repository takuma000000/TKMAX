#include "DirectXCommon.h"
#include "SrvManager.h"
#include <cassert>
#include <format>
#include "Logger.h"
#include "StringUtility.h"
#include "thread"
#include "d3dx12.h"
#include <vector>

#include "RadialBlurEffect.h" 
#include "VignettingEffect.h"
#include "WaterRippleEffect.h"
#include "FogEffect.h"
#include "AuraEffect.h"

#define ALIGN256(size) ((size + 255) & ~255)

using namespace StringUtility;
using namespace Logger;

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;

namespace TKM {
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

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DirectXCommon::CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		UINT numDescriptors,
		bool shaderVisible) {
		//デスクリプタヒープ設定
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
		descriptorHeapDesc.Type = heapType;
		descriptorHeapDesc.NumDescriptors = numDescriptors;
		descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		//デスクリプタヒープの生成
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
		assert(SUCCEEDED(hr));

		return descriptorHeap;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateRenderTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int width, int height, DXGI_FORMAT format, const Vector4& clearColor) {
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;

		//生成するResourceの設定
		D3D12_RESOURCE_DESC resourceDesc{};
		resourceDesc.Width = width;//Textureの幅
		resourceDesc.Height = height;//Textureの高さ
		resourceDesc.MipLevels = 1;//MipMapの数
		resourceDesc.DepthOrArraySize = 1;//奥行き or 配列Textureの配列数
		resourceDesc.Format = format;//DepthStencilとして利用可能なフォーマット
		resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2次元
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;//DepthStencilとして使う通知

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM

		D3D12_CLEAR_VALUE clearValue; // クリア値設定
		clearValue.Format = format;
		clearValue.Color[0] = clearColor.x;
		clearValue.Color[1] = clearColor.y;
		clearValue.Color[2] = clearColor.z;
		clearValue.Color[3] = clearColor.w;

		HRESULT hr = device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS(&resource)
		);
		assert(SUCCEEDED(hr));

		return resource;
	}

	void DirectXCommon::CreateRenderTextureRTV() {
		const Vector4 kRenderTargetClearValue{ 1.0f, 0.0f, 0.0f, 1.0f }; // 赤色でクリア

		// ========= 1枚目：シーン用 RenderTexture =========
		renderTextureResource =
			CreateRenderTextureResource(
				device,
				WindowsAPI::kClientWidth,
				WindowsAPI::kClientHeight,
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				kRenderTargetClearValue);

		renderTextureResource->SetName(L"RenderTexture");

		UINT descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// RTV: インデックス2に RenderTexture
		rtvHandles[2] = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
		rtvHandles[2].ptr += descriptorSize * 2;

		device->CreateRenderTargetView(
			renderTextureResource.Get(),
			&rtvDesc,
			rtvHandles[2]);

		// ========= 2枚目：ポストエフェクト用 PostEffectTexture =========
		postEffectTextureResource =
			CreateRenderTextureResource(
				device,
				WindowsAPI::kClientWidth,
				WindowsAPI::kClientHeight,
				DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
				kRenderTargetClearValue);

		postEffectTextureResource->SetName(L"PostEffectTexture");

		// RTV: インデックス3に PostEffectTexture
		rtvHandles[3] = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
		rtvHandles[3].ptr += descriptorSize * 3;

		device->CreateRenderTargetView(
			postEffectTextureResource.Get(),
			&rtvDesc,
			rtvHandles[3]);

		// ============================
		// SRV を 2つ作成
		// ============================
		assert(srvManager_ && "SrvManager がセットされていません");

		// 1枚目：RenderTexture 用 SRV
		renderTextureSrvIndex_ = srvManager_->Allocate();
		srvManager_->CreateSRVforTexture2D(
			renderTextureSrvIndex_,
			renderTextureResource.Get(),
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			1);

		// 2枚目：PostEffectTexture 用 SRV
		postEffectSrvIndex_ = srvManager_->Allocate();
		srvManager_->CreateSRVforTexture2D(
			postEffectSrvIndex_,
			postEffectTextureResource.Get(),
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			1);
	}

	void DirectXCommon::ApplyFog(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv) {

		if (!fogInitialized_ || !inputTex || !outputTex) {
			return;
		}

		// ===== 1. 入力テクスチャだけ RT → PS にする =====
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = inputTex;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &barrier);

		// ===== 2. 出力RTVセット & 描画 =====
		commandList->OMSetRenderTargets(1, &outputRtv, false, nullptr);

		// パイプライン設定
		commandList->SetGraphicsRootSignature(fogRootSignature_.Get());
		commandList->SetPipelineState(fogPipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// SRV ヒープ + t0
		if (srvManager_) {
			ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSrvDescriptorHeap().Get() };
			commandList->SetDescriptorHeaps(1, heaps);
			srvManager_->SetGraphicsRootDescriptorTable(0, inputSrvIndex);
		}

		// b0: 定数バッファ
		if (fogConstantBuffer_) {
			commandList->SetGraphicsRootConstantBufferView(
				1, fogConstantBuffer_->GetGPUVirtualAddress());
		}

		// フルスクリーントライアングル
		commandList->DrawInstanced(3, 1, 0, 0);

		// ===== 3. 入力テクスチャだけ PS → RT に戻す =====
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);
	}

	void DirectXCommon::DrawTextureToSwapchain(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex) {

		assert(copyImageInitialized_ && "InitializeCopyImagePipeline を先に呼んでください");
		assert(inputTex && "入力テクスチャがありません");

		// 入力テクスチャを PixelShaderResource へ
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = inputTex;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			commandList->ResourceBarrier(1, &barrier);
		}

		// SwapChain の現在の RTV 取得
		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandles[backBufferIndex];
		commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// パイプライン / RootSignature 設定
		commandList->SetGraphicsRootSignature(copyImageRootSignature_.Get());
		commandList->SetPipelineState(copyImagePipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// SRV ヒープ + t0
		if (srvManager_) {
			ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSrvDescriptorHeap().Get() };
			commandList->SetDescriptorHeaps(1, heaps);
			srvManager_->SetGraphicsRootDescriptorTable(0, inputSrvIndex);
		}

		// フルスクリーントライアングル
		commandList->DrawInstanced(3, 1, 0, 0);

		// 入力テクスチャを RenderTarget 戻し
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = inputTex;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			commandList->ResourceBarrier(1, &barrier);
		}
	}

	void DirectXCommon::BeginDrawToSwapchain() {

		// 現在のバックバッファインデックス取得
		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		// Present → RenderTarget へ遷移
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);

		// RTV / DSV を Swapchain 用にセット
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandles[backBufferIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		// 画面クリア（お好みの色でOK）
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,
			0,
			0,
			nullptr
		);

		// ビューポート / シザー設定
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
	}

	void DirectXCommon::InitializeCopyImagePipeline() {
		if (copyImageInitialized_) { return; }

		// 1. シェーダコンパイル
		// パスは自分のフォルダ構成に合わせて調整
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/CopyImage.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/CopyImage.PS.hlsl", L"ps_6_0");

		// 2. RootSignature 作成
		//   - t0: SRV (RenderTexture)
		//   - s0: Sampler
		CD3DX12_DESCRIPTOR_RANGE range{};
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0, 1枚

		CD3DX12_ROOT_PARAMETER rootParam{};
		rootParam.InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.MinLOD = 0.0f;
		sampler.MipLODBias = 0.0f;
		sampler.MaxAnisotropy = 1;
		sampler.ShaderRegister = 0; // s0
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Init(
			1, &rootParam,
			1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&rsBlob,
			&errorBlob
		);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&copyImageRootSignature_)
		);
		assert(SUCCEEDED(hr));

		// 3. PSO 作成
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = copyImageRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		// フルスクリーン三角形なので InputLayout なし
		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// レンダーターゲット設定（Swapchain と同じ）
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;

		// ブレンド / ラスタライザ / 深度ステンシル
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		// 深度は使わない（PDF18Pの DepthStencilState = false）
		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = FALSE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&copyImagePipelineState_));
		assert(SUCCEEDED(hr));

		copyImageInitialized_ = true;
	}

	void DirectXCommon::DrawRenderTextureToSwapchain() {

		assert(copyImageInitialized_ && "InitializeCopyImagePipeline を先に呼んでください");
		assert(renderTextureResource && "RenderTexture が作られていません");

		// 1. RenderTarget → PixelShaderResource へバリア
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = renderTextureResource.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			commandList->ResourceBarrier(1, &barrier);
		}

		// 2. パイプライン / RootSignature 設定
		commandList->SetGraphicsRootSignature(copyImageRootSignature_.Get());
		commandList->SetPipelineState(copyImagePipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 3. SRV ヒープをセットし、t0 に RenderTexture の SRV をバインド
		if (srvManager_) {
			ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSrvDescriptorHeap().Get() };
			commandList->SetDescriptorHeaps(1, heaps);

			// RootParameter0 の DescriptorTable に renderTextureSrvIndex_ をセット
			srvManager_->SetGraphicsRootDescriptorTable(0, renderTextureSrvIndex_);
		}

		// 4. フルスクリーン三角形を描画 (頂点数3)
		commandList->DrawInstanced(3, 1, 0, 0);

		// 5. PixelShaderResource → RenderTarget に戻す
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = renderTextureResource.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			commandList->ResourceBarrier(1, &barrier);
		}
	}

	void DirectXCommon::InitializeRadialBlurPipeline() {
		if (radialBlurInitialized_) { return; }

		// まず CopyImage 側が初期化されていることを保証
		if (!copyImageInitialized_) {
			InitializeCopyImagePipeline();
		}

		// VS は CopyImage と同じフルスクリーントライアングル
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/CopyImage.VS.hlsl", L"vs_6_0");
		// PS だけ RadialBlur
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/RadialBlur.PS.hlsl", L"ps_6_0");

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = copyImageRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		// フルスクリーン三角形なので InputLayout なし
		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// レンダーターゲット設定（Swapchain と同じ）
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;

		// ブレンド / ラスタライザ / 深度ステンシル
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		// 深度は使わない
		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = FALSE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		HRESULT hr = device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&radialBlurPipelineState_));
		assert(SUCCEEDED(hr));

		radialBlurInitialized_ = true;
	}

	void DirectXCommon::ApplyRadialBlur(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv) {

		if (!radialBlurInitialized_ || !inputTex || !outputTex) {
			return;
		}

		// ===== 1. 入力テクスチャだけ RT → PS にする =====
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = inputTex;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &barrier);

		// ===== 2. 出力RTVセット & 描画 =====
		commandList->OMSetRenderTargets(1, &outputRtv, false, nullptr);

		commandList->SetGraphicsRootSignature(copyImageRootSignature_.Get());
		commandList->SetPipelineState(radialBlurPipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (srvManager_) {
			ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSrvDescriptorHeap().Get() };
			commandList->SetDescriptorHeaps(1, heaps);
			srvManager_->SetGraphicsRootDescriptorTable(0, inputSrvIndex);
		}

		commandList->DrawInstanced(3, 1, 0, 0);

		// ===== 3. 入力テクスチャだけ PS → RT に戻す =====
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);
	}

	void DirectXCommon::InitializeVignettingPipeline() {
		if (vignettingInitialized_) { return; }

		// VS は CopyImage と共通
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/CopyImage.VS.hlsl", L"vs_6_0");
		// PS は Vignetting 専用
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/Vignetting.PS.hlsl", L"ps_6_0");

		// RootSignature
		CD3DX12_DESCRIPTOR_RANGE range{};
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

		CD3DX12_ROOT_PARAMETER rootParams[2];
		// 0: SRV テーブル (t0)
		rootParams[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
		// 1: 定数バッファ (b0)
		rootParams[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.MinLOD = 0.0f;
		sampler.MipLODBias = 0.0f;
		sampler.MaxAnisotropy = 1;
		sampler.ShaderRegister = 0; // s0
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Init(
			_countof(rootParams), rootParams,
			1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&rsBlob,
			&errorBlob
		);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&vignettingRootSignature_)
		);
		assert(SUCCEEDED(hr));

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = vignettingRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = FALSE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		hr = device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&vignettingPipelineState_));
		assert(SUCCEEDED(hr));

		// 定数バッファ作成＆マップ
		vignettingConstantBuffer_ = CreateBufferResource(sizeof(VignettingCB));
		vignettingConstantBuffer_->Map(
			0, nullptr, &vignettingMappedData_);

		// 初期値
		auto* cb = reinterpret_cast<VignettingCB*>(vignettingMappedData_);
		cb->color = { 0.0f, 0.0f, 0.0f, 1.0f }; // 黒縁
		cb->intensity = 0.8f;
		cb->radius = 0.6f;
		cb->softness = 0.4f;
		cb->padding = 0.0f;

		vignettingInitialized_ = true;
	}

	void DirectXCommon::ApplyVignetting(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv) {

		if (!vignettingInitialized_ || !inputTex) {
			return;
		}

		// 入力テクスチャを PixelShaderResource へ
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = inputTex;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			commandList->ResourceBarrier(1, &barrier);
		}

		// 出力RTVセット
		commandList->OMSetRenderTargets(1, &outputRtv, false, nullptr);

		// パイプライン設定
		commandList->SetGraphicsRootSignature(vignettingRootSignature_.Get());
		commandList->SetPipelineState(vignettingPipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// SRV ヒープ + t0
		if (srvManager_) {
			ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSrvDescriptorHeap().Get() };
			commandList->SetDescriptorHeaps(1, heaps);
			srvManager_->SetGraphicsRootDescriptorTable(0, inputSrvIndex);
		}

		// b0: 定数バッファ
		if (vignettingConstantBuffer_) {
			commandList->SetGraphicsRootConstantBufferView(
				1, vignettingConstantBuffer_->GetGPUVirtualAddress());
		}

		// フルスクリーントライアングル
		commandList->DrawInstanced(3, 1, 0, 0);

		// 入力テクスチャを RenderTarget 戻し
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = inputTex;
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			commandList->ResourceBarrier(1, &barrier);
		}
	}

	void DirectXCommon::InitializeWaterRipplePipeline() {
		if (rippleInitialized_) { return; }

		// VS は CopyImage と共通
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/CopyImage.VS.hlsl", L"vs_6_0");
		// PS は WaterRipple 専用
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/Ripple.PS.hlsl", L"ps_6_0");

		// RootSignature（Vignetting とほぼ同じ構成：t0 + b0 + s0）
		CD3DX12_DESCRIPTOR_RANGE range{};
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

		CD3DX12_ROOT_PARAMETER rootParams[2];
		// 0: SRV テーブル (t0)
		rootParams[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
		// 1: 定数バッファ (b0)
		rootParams[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.MinLOD = 0.0f;
		sampler.MipLODBias = 0.0f;
		sampler.MaxAnisotropy = 1;
		sampler.ShaderRegister = 0; // s0
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Init(
			_countof(rootParams), rootParams,
			1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&rsBlob,
			&errorBlob
		);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&rippleRootSignature_)
		);
		assert(SUCCEEDED(hr));

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rippleRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = FALSE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		hr = device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&ripplePipelineState_));
		assert(SUCCEEDED(hr));

		// 定数バッファ作成＆マップ
		rippleConstantBuffer_ = CreateBufferResource(sizeof(WaterRippleCB));
		rippleConstantBuffer_->Map(0, nullptr, &rippleMappedData_);

		// 初期値
		auto* cb = reinterpret_cast<WaterRippleCB*>(rippleMappedData_);
		cb->center = { 0.5f, 0.5f };
		cb->radius = 0.0f;
		cb->amplitude = 0.0f;
		cb->frequency = 40.0f;
		cb->width = 40.0f;
		cb->color = { 1.0f, 1.0f, 1.0f }; // デフォルト白
		cb->colorIntensity = 0.0f;        // 初期は色なし

		rippleInitialized_ = true;
	}

	void DirectXCommon::InitializeFogPipeline() {
		if (fogInitialized_) { return; }

		// VS は CopyImage と共通
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/CopyImage.VS.hlsl", L"vs_6_0");
		// PS は Fog 専用
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/Fog.PS.hlsl", L"ps_6_0");

		// RootSignature（Vignetting / Ripple と同じ：t0 + b0 + s0）
		CD3DX12_DESCRIPTOR_RANGE range{};
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

		CD3DX12_ROOT_PARAMETER rootParams[2];
		// 0: SRV テーブル (t0)
		rootParams[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);
		// 1: 定数バッファ (b0)
		rootParams[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.MinLOD = 0.0f;
		sampler.MipLODBias = 0.0f;
		sampler.MaxAnisotropy = 1;
		sampler.ShaderRegister = 0; // s0
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Init(
			_countof(rootParams), rootParams,
			1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&rsBlob,
			&errorBlob
		);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&fogRootSignature_)
		);
		assert(SUCCEEDED(hr));

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = fogRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = FALSE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		hr = device->CreateGraphicsPipelineState(
			&psoDesc, IID_PPV_ARGS(&fogPipelineState_));
		assert(SUCCEEDED(hr));

		// 定数バッファ作成＆マップ
		fogConstantBuffer_ = CreateBufferResource(sizeof(FogCB));
		fogConstantBuffer_->Map(0, nullptr, &fogMappedData_);

		// 初期値
		auto* cb = reinterpret_cast<FogCB*>(fogMappedData_);
		cb->FogColor = { 0.9f, 0.9f, 1.0f };
		cb->FogDensity = 1.0f;
		cb->FogStart = 0.0f;
		cb->FogEnd = 1.0f;
		cb->NoiseScale = 4.0f;
		cb->NoiseStrength = 0.0f;
		cb->Time = 0.0f;
		cb->worldScale = 0.02f;                 // とりあえず適当な値
		cb->worldPos = { 0.0f, 0.0f, 0.0f };
		cb->padding2 = 0.0f;

		fogInitialized_ = true;
	}

	void DirectXCommon::InitializeAuraPipeline() {
		if (auraInitialized_) { return; }

		// VS は CopyImage と共通
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/CopyImage.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/Aura.PS.hlsl", L"ps_6_0");

		// RootSignature（t0 + b0 + sampler）
		CD3DX12_DESCRIPTOR_RANGE range{};
		range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

		CD3DX12_ROOT_PARAMETER rootParams[2];
		rootParams[0].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL); // SRV
		rootParams[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);  // b0

		D3D12_STATIC_SAMPLER_DESC sampler{};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0; // s0
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Init(
			_countof(rootParams), rootParams,
			1, &sampler,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &errorBlob);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&auraRootSignature_));
		assert(SUCCEEDED(hr));

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = auraRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
		psoDesc.InputLayout = { nullptr, 0 };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = FALSE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&auraPipelineState_));
		assert(SUCCEEDED(hr));

		// 定数バッファ
		auraConstantBuffer_ = CreateBufferResource(sizeof(AuraCB));
		auraConstantBuffer_->Map(0, nullptr, &auraMappedData_);

		auto* cb = reinterpret_cast<AuraCB*>(auraMappedData_);
		cb->CenterUV = { 0.5f, 0.5f };
		cb->TopUV = { 0.5f, 0.35f };
		cb->BottomUV = { 0.5f, 0.70f };
		cb->Aspect = WindowsAPI::kClientWidth / (float)WindowsAPI::kClientHeight;
		cb->Time = 0.0f;
		cb->Radius = 0.22f;
		cb->Intensity = 0.0f;
		cb->UseRing = 1.0f;
		cb->RingRadius = 0.12f;
		cb->RingWidth = 22.0f;
		cb->ColorA = { 0.2f, 0.6f, 1.0f };
		cb->_pad0 = 0.0f;
		cb->ColorB = { 1.0f, 0.85f, 0.2f };
		cb->Mix = 0.25f;
		// 立体っぽい揺れ用
		cb->Taper = 0.65f;
		cb->NoiseScale = 7.0f;
		cb->NoiseSpeed = 1.4f;
		cb->FlameStrength = 1.4f;
		cb->EdgePower = 2.2f;
		cb->VerticalFade = 0.12f;

		auraInitialized_ = true;
	}

	void DirectXCommon::InitializeAuraVolumePipeline() {
		if (auraVolumeInitialized_) { return; }

		// シェーダ
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/AuraVolume.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/AuraVolume.PS.hlsl", L"ps_6_0");

		// RootSignature: b0 だけ（VS/PS共通）
		CD3DX12_ROOT_PARAMETER rootParams[1];
		rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Init(
			1, rootParams,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&rsBlob,
			&errorBlob
		);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&auraVolumeRootSignature_)
		);
		assert(SUCCEEDED(hr));

		// InputLayout（Quad）
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = auraVolumeRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // RenderTextureと合わせ
		psoDesc.SampleDesc.Count = 1;

		// Rasterizer
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // 両面でOK

		// Depth: ZTest ON / ZWrite OFF
		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = TRUE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// Blend: 加算（ONE + ONE）
		D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState = blendDesc;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&auraVolumePipelineState_));
		assert(SUCCEEDED(hr));

		// 定数バッファ作成&マップ
		auraVolumeConstantBuffer_ = CreateBufferResource(sizeof(AuraVolumeCB));
		auraVolumeConstantBuffer_->Map(0, nullptr, &auraVolumeMappedData_);

		// 頂点バッファ（Quad 6頂点）
		struct Vtx { float px, py, pz; float u, v; };
		Vtx v[6] = {
			{-1, 0, 0, 0, 1},
			{-1, 1, 0, 0, 0},
			{ 1, 1, 0, 1, 0},

			{-1, 0, 0, 0, 1},
			{ 1, 1, 0, 1, 0},
			{ 1, 0, 0, 1, 1},
		};

		auraVolumeVB_ = CreateBufferResource(sizeof(v));
		void* mapped = nullptr;
		auraVolumeVB_->Map(0, nullptr, &mapped);
		memcpy(mapped, v, sizeof(v));
		auraVolumeVB_->Unmap(0, nullptr);

		auraVolumeVBView_.BufferLocation = auraVolumeVB_->GetGPUVirtualAddress();
		auraVolumeVBView_.SizeInBytes = (UINT)sizeof(v);
		auraVolumeVBView_.StrideInBytes = sizeof(Vtx);

		auraVolumeInitialized_ = true;
	}

	void DirectXCommon::InitializeFogVolumePipeline() {
		if (fogVolumeInitialized_) { return; }

		// シェーダ
		Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
			CompileShader(L"resources/shaders/FogVolume.VS.hlsl", L"vs_6_0");
		Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
			CompileShader(L"resources/shaders/FogVolume.PS.hlsl", L"ps_6_0");

		// RootSignature: b0 のみ
		CD3DX12_ROOT_PARAMETER rootParams[1];
		rootParams[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.Init(
			1, rootParams,
			0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		Microsoft::WRL::ComPtr<ID3DBlob> rsBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &errorBlob
		);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&fogVolumeRootSignature_)
		);
		assert(SUCCEEDED(hr));

		// InputLayout（Quad）
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = fogVolumeRootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;

		// Rasterizer
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		// Depth: ZTest ON / ZWrite OFF
		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = TRUE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		// Blend: αブレンド（霧）
		D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState = blendDesc;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&fogVolumePipelineState_));
		assert(SUCCEEDED(hr));

		// 定数バッファ
		fogVolumeConstantBuffer_ = CreateBufferResource(sizeof(FogVolumeCB));
		fogVolumeConstantBuffer_->Map(0, nullptr, &fogVolumeMappedData_);

		// 頂点バッファ（Quad 6頂点 / XY は [-1..1]）
		struct Vtx { float px, py, pz; float u, v; };
		Vtx v[6] = {
			{-1, -1, 0, 0, 1},
			{-1,  1, 0, 0, 0},
			{ 1,  1, 0, 1, 0},

			{-1, -1, 0, 0, 1},
			{ 1,  1, 0, 1, 0},
			{ 1, -1, 0, 1, 1},
		};

		fogVolumeVB_ = CreateBufferResource(sizeof(v));
		void* mapped = nullptr;
		fogVolumeVB_->Map(0, nullptr, &mapped);
		memcpy(mapped, v, sizeof(v));
		fogVolumeVB_->Unmap(0, nullptr);

		fogVolumeVBView_.BufferLocation = fogVolumeVB_->GetGPUVirtualAddress();
		fogVolumeVBView_.SizeInBytes = (UINT)sizeof(v);
		fogVolumeVBView_.StrideInBytes = sizeof(Vtx);

		fogVolumeInitialized_ = true;
	}

	void DirectXCommon::ApplyWaterRipple(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv) {

		// ===== 1. 入力テクスチャだけ RT → PS にする =====
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = inputTex;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &barrier);

		// ===== 2. 出力RTVセット & 描画 =====
		commandList->OMSetRenderTargets(1, &outputRtv, false, nullptr);

		commandList->SetGraphicsRootSignature(rippleRootSignature_.Get());
		commandList->SetPipelineState(ripplePipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (srvManager_) {
			ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSrvDescriptorHeap().Get() };
			commandList->SetDescriptorHeaps(1, heaps);
			srvManager_->SetGraphicsRootDescriptorTable(0, inputSrvIndex);
		}

		if (rippleConstantBuffer_) {
			commandList->SetGraphicsRootConstantBufferView(
				1, rippleConstantBuffer_->GetGPUVirtualAddress());
		}

		commandList->DrawInstanced(3, 1, 0, 0);

		// ===== 3. 入力テクスチャだけ PS → RT に戻す =====
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		commandList->ResourceBarrier(1, &barrier);
	}

	void DirectXCommon::Initialize(WindowsAPI* windowsAPI) {
		//FPS初期化固定
		InitializeFixFPS();

		//NULL検出
		assert(windowsAPI);

		//借りてきたWinAppのインスタンスを記録
		this->windowsAPI = windowsAPI;

		// DirectX初期化
		InitializeDevice(); // デバイス初期化
		InitializeCommand(); // コマンド初期化
		GenerateSwapChain(); // スワップチェーン生成
		GenerateZBuffer(); // Zバッファ生成
		GenerateDescpitorHeap(); // デスクリプタヒープ生成
		GenerateDXC(); // DXC生成
		InitializeRTV(); // RTV初期化
		InitializeDSV(); // DSV初期化
		InitializeFence(); // フェンス初期化
		InitializeViewport(); // ビューポート初期化
		InitializeScissorRect(); // シザー矩形初期化
	}

	void DirectXCommon::InitializeDevice() {
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
		for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) { //アダプターの列挙
			//アダプターの情報を取得する
			DXGI_ADAPTER_DESC3 adapterDesc{};
			hr = useAdapter->GetDesc3(&adapterDesc);
			assert(SUCCEEDED(hr));//取得できないのは一大事

			//ソフトウェアアダプタでなければ採用
			if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) { //ソフトウェアアダプタでなければ
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
		D3D_FEATURE_LEVEL featureLevels[] = { //試す機能レベル一覧
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

		// 情報キューを取得してメッセージのフィルタリング設定を行う
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

	void DirectXCommon::InitializeCommand() {
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

	void DirectXCommon::GenerateSwapChain() {
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

	void DirectXCommon::GenerateDescpitorHeap() {
		//DescriptorSizeを取得しておく
		descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

#pragma region ディスクリプタヒープの生成

		//RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
		rtvDescriptorHeap = this->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

#pragma endregion
	}

	void DirectXCommon::GenerateDXC() {
		HRESULT hr;

		// DXCのユーティリティとコンパイラのインスタンスを生成
		hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
		assert(SUCCEEDED(hr));
		// DXCコンパイラのインスタンスを生成
		hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
		assert(SUCCEEDED(hr));
		// インクルードハンドラの生成
		hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
		assert(SUCCEEDED(hr));

	}

	void DirectXCommon::ApplyAura(
		ID3D12Resource* inputTex,
		uint32_t        inputSrvIndex,
		ID3D12Resource* outputTex,
		D3D12_CPU_DESCRIPTOR_HANDLE outputRtv) {

		if (!auraInitialized_ || !inputTex || !outputTex) {
			return;
		}

		// 入力だけ RT → PS
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.pResource = inputTex;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		commandList->ResourceBarrier(1, &barrier);

		// 出力
		commandList->OMSetRenderTargets(1, &outputRtv, false, nullptr);

		commandList->SetGraphicsRootSignature(auraRootSignature_.Get());
		commandList->SetPipelineState(auraPipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		if (srvManager_) {
			ID3D12DescriptorHeap* heaps[] = { srvManager_->GetSrvDescriptorHeap().Get() };
			commandList->SetDescriptorHeaps(1, heaps);
			srvManager_->SetGraphicsRootDescriptorTable(0, inputSrvIndex);
		}

		if (auraConstantBuffer_) {
			commandList->SetGraphicsRootConstantBufferView(
				1, auraConstantBuffer_->GetGPUVirtualAddress());
		}

		commandList->DrawInstanced(3, 1, 0, 0);

		// 入力だけ PS → RT
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		commandList->ResourceBarrier(1, &barrier);
	}

	void DirectXCommon::DrawAuraVolume(
		const Matrix4x4& viewProj,
		const Vector3& centerWS,
		float radius,
		float height,
		uint32_t sliceCount,
		float time,
		const Vector3& color,
		float intensity,
		float noiseScale,
		float noiseSpeed,
		float rimPower,
		float alphaBase)
	{
		if (!auraVolumeInitialized_) { InitializeAuraVolumePipeline(); }
		if (!auraVolumeMappedData_) { return; }
		if (!commandList) { return; }

		// CB更新
		auto* cb = reinterpret_cast<AuraVolumeCB*>(auraVolumeMappedData_);
		cb->ViewProj = viewProj;
		cb->CenterWS = centerWS;
		cb->Radius = radius;
		cb->Height = height;
		cb->SliceCount = sliceCount;
		cb->Time = time;
		cb->_pad0 = 0.0f;

		cb->Color = color;
		cb->Intensity = intensity;

		cb->NoiseScale = noiseScale;
		cb->NoiseSpeed = noiseSpeed;
		cb->RimPower = rimPower;
		cb->AlphaBase = alphaBase;

		// パイプライン
		commandList->SetGraphicsRootSignature(auraVolumeRootSignature_.Get());
		commandList->SetPipelineState(auraVolumePipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &auraVolumeVBView_);

		// b0
		commandList->SetGraphicsRootConstantBufferView(
			0, auraVolumeConstantBuffer_->GetGPUVirtualAddress());

		// 描画（6頂点のQuadを sliceCount 枚インスタンス）
		commandList->DrawInstanced(6, sliceCount, 0, 0);
	}

	void DirectXCommon::DrawFogVolume(
		const Matrix4x4& viewProj,
		const Vector3& centerWS,
		const Vector3& halfSizeWS,
		const Vector3& camRightWS,
		const Vector3& camUpWS,
		const Vector3& camFwdWS,
		uint32_t sliceCount,
		float time,
		const Vector3& fogColor,
		float density,
		float noiseScale,
		float noiseSpeed,
		float softness) {

		if (!fogVolumeInitialized_) { InitializeFogVolumePipeline(); }

		auto* cb = reinterpret_cast<FogVolumeCB*>(fogVolumeMappedData_);
		cb->ViewProj = viewProj;

		cb->CenterWS = centerWS;
		cb->_pad0 = 0.0f;

		cb->HalfSizeWS = halfSizeWS;
		cb->Density = density;

		cb->CamRightWS = camRightWS; cb->_pad1 = 0.0f;
		cb->CamUpWS = camUpWS;       cb->_pad2 = 0.0f;
		cb->CamFwdWS = camFwdWS;     cb->_pad3 = 0.0f;

		cb->SliceCount = sliceCount;
		cb->Time = time;
		cb->NoiseScale = noiseScale;
		cb->NoiseSpeed = noiseSpeed;

		cb->FogColor = fogColor;
		cb->Softness = softness;

		cb->FogStart = 0.15f;      // 下側が濃くなる開始（0..1）
		cb->FogEnd = 0.95f;      // 濃くなる上限（0..1）
		cb->NoiseStrength = 0.55f; // Fog.PSと同じ感じ
		cb->WorldScale = 1.0f;     // ノイズ座標のスケール
		cb->WorldPos = centerWS;   // とりあえず中心基準が分かりやすい


		commandList->SetGraphicsRootSignature(fogVolumeRootSignature_.Get());
		commandList->SetPipelineState(fogVolumePipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->IASetVertexBuffers(0, 1, &fogVolumeVBView_);
		commandList->SetGraphicsRootConstantBufferView(0, fogVolumeConstantBuffer_->GetGPUVirtualAddress());

		// 6頂点 × sliceCount インスタンス
		commandList->DrawInstanced(6, (UINT)max(sliceCount, 1u), 0, 0);
	}

	void DirectXCommon::DrawPostEffectToSwapchain() {

		// CopyImage パイプラインが未初期化なら初期化
		if (!copyImageInitialized_) {
			InitializeCopyImagePipeline();
		}

		// オフスク2枚がなければ従来どおり
		if (!renderTextureResource || !postEffectTextureResource) {
			DrawRenderTextureToSwapchain();
			return;
		}

		// チェーン用の src/dst
		ID3D12Resource* srcTex = renderTextureResource.Get();
		uint32_t        srcSrv = renderTextureSrvIndex_;
		ID3D12Resource* dstTex = postEffectTextureResource.Get();
		uint32_t        dstSrv = postEffectSrvIndex_;

		// dstTex に対応する RTV を返すヘルパー
		auto getRtvFor = [&](ID3D12Resource* tex) {
			if (tex == renderTextureResource.Get()) {
				return rtvHandles[2]; // RenderTexture 用
			} else {
				return rtvHandles[3]; // PostEffectTexture 用
			}
			};

		// 1. RadialBlur
		if (radialBlurEffect_ && radialBlurEffect_->IsActive()) {
			if (!radialBlurInitialized_) {
				InitializeRadialBlurPipeline();
			}
			ApplyRadialBlur(srcTex, srcSrv, dstTex, getRtvFor(dstTex));
			std::swap(srcTex, dstTex);
			std::swap(srcSrv, dstSrv);
		}

		// 2. Ripple
		if (rippleEffect_ && rippleEffect_->IsActive()) {
			if (!rippleInitialized_) {
				InitializeWaterRipplePipeline();
			}
			ApplyWaterRipple(srcTex, srcSrv, dstTex, getRtvFor(dstTex));
			std::swap(srcTex, dstTex);
			std::swap(srcSrv, dstSrv);
		}

		// 3. Vignetting
		if (vignettingEffect_ && vignettingEffect_->IsActive()) {
			if (!vignettingInitialized_) {
				InitializeVignettingPipeline();
			}
			ApplyVignetting(srcTex, srcSrv, dstTex, getRtvFor(dstTex));
			std::swap(srcTex, dstTex);
			std::swap(srcSrv, dstSrv);
		}

		// 4. Aura
		if (auraEffect_ && auraEffect_->IsActive()) {
			if (!auraInitialized_) {
				InitializeAuraPipeline();
			}
			ApplyAura(srcTex, srcSrv, dstTex, getRtvFor(dstTex));
			std::swap(srcTex, dstTex);
			std::swap(srcSrv, dstSrv);
		}

		// 5. Fog
		if (fogEffect_ && fogEffect_->IsActive()) {
			if (!fogInitialized_) {
				InitializeFogPipeline();
			}
			ApplyFog(srcTex, srcSrv, dstTex, getRtvFor(dstTex));
			std::swap(srcTex, dstTex);
			std::swap(srcSrv, dstSrv);
		}

		// 最後は srcTex を Swapchain へ
		DrawTextureToSwapchain(srcTex, srcSrv);
	}


	Microsoft::WRL::ComPtr<IDxcBlob> DirectXCommon::CompileShader(const std::wstring& filePath, const wchar_t* profile) {
		Log(ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));

		// シェーダソースコードを読み込む
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
		HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
		assert(SUCCEEDED(hr));
		// シェーダソースコードをコンパイルする
		DxcBuffer shaderSourceBuffer;
		shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
		shaderSourceBuffer.Size = shaderSource->GetBufferSize();
		shaderSourceBuffer.Encoding = DXC_CP_UTF8;

		// コンパイルオプションの設定
		LPCWSTR arguments[] = {
			filePath.c_str(),
			L"-E", L"main",
			L"-T", profile,
			L"-Zi", L"-Qembed_debug",
			L"-Od",
			L"-Zpr"
		};

		// コンパイル実行
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

		// コンパイル結果の取得とログ出力
		Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
		shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
		if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
			Log(shaderError->GetStringPointer());
			assert(false);
		}

		// コンパイル結果のバイナリを取得
		Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
		hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		assert(SUCCEEDED(hr));

		Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

		return shaderBlob;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateBufferResource(size_t sizeInBytes) {
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
		hr = device->CreateCommittedResource( // ヒープの設定
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

	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::CreateTextureResource(const DirectX::TexMetadata& metadata) {
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
			D3D12_RESOURCE_STATE_COPY_DEST,//初回のResourceState。	Textureは基本読むだけ
			nullptr,//Clear最適値。使わないのでnullptr
			IID_PPV_ARGS(&resource)//作成するResourceポインタへのポインタ
		);
		assert(SUCCEEDED(hr));
		return resource;
	}

	[[nodiscard]]
	Microsoft::WRL::ComPtr<ID3D12Resource> DirectXCommon::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
		//SubresourceDataの配列を用意して、画像データを詰め込む
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;
		DirectX::PrepareUpload(device.Get(), mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
		uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(intermediateSize);
		UpdateSubresources(commandList.Get(), texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
		// textureへの状態遷移 -> D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = texture;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		commandList->ResourceBarrier(1, &barrier);
		return intermediateResource;
	}

	void DirectXCommon::InitializeRTV() {
		HRESULT hr;

		rtvHeap_ = this->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4, false); // RTV用のヒープを作成

#pragma region SwapChainからResourceを引っ張てくる

		//SwapChainからResourceを引っ張てくる
		hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
		//うまく取得できなければ起動できない
		assert(SUCCEEDED(hr));
		hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
		assert(SUCCEEDED(hr));

#pragma endregion

		//RTVの設定
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;	//出力結果をSRGBに変換して書き込む
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;	//2dテスクチャとして書き込む
		//ディスクリプタの先頭を取得する
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
		device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);
		device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);
	}

	void DirectXCommon::InitializeDSV() {
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

	void DirectXCommon::InitializeFence() {
		HRESULT hr;

		//フェンスの生成
		fence = nullptr;
		fenceValue = 0;
		hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		assert(SUCCEEDED(hr));

		fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // フェンス完了通知用イベントの作成
		assert(fenceEvent != nullptr);
	}

	void DirectXCommon::InitializeViewport() {
		//ビューポート矩形の設定
		viewport.Width = WindowsAPI::kClientWidth;
		viewport.Height = WindowsAPI::kClientHeight;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
	}

	void DirectXCommon::InitializeScissorRect() {
		//シザリング矩形の設定
		scissorRect.left = 0;
		scissorRect.right = WindowsAPI::kClientWidth;
		scissorRect.top = 0;
		scissorRect.bottom = WindowsAPI::kClientHeight;
	}

	void DirectXCommon::SetVignettingParam(
		const Vector4& color, float intensity, float radius, float softness) {
		if (!vignettingMappedData_) { return; }

		auto* cb = reinterpret_cast<VignettingCB*>(vignettingMappedData_);
		cb->color = color;
		cb->intensity = intensity;
		cb->radius = radius;
		cb->softness = softness;
	}

	void DirectXCommon::SetWaterRippleParam(
		const Vector2& centerUV,
		float radius,
		float amplitude,
		float frequency,
		float width,
		const Vector3& color,
		float colorIntensity)
	{
		if (!rippleMappedData_) return;

		auto* cb = reinterpret_cast<WaterRippleCB*>(rippleMappedData_);
		cb->center = centerUV;
		cb->radius = radius;
		cb->amplitude = amplitude;
		cb->frequency = frequency;
		cb->width = width;
		cb->color = color;
		cb->colorIntensity = colorIntensity;
	}

	void DirectXCommon::SetFogParam(const Vector3& color,
		float density, float start, float end,
		float noiseScale, float noiseStrength,
		float time,
		const Vector3& worldPos, float worldScale) {

		if (!fogMappedData_) { return; }

		auto* cb = reinterpret_cast<FogCB*>(fogMappedData_);
		cb->FogColor = color;
		cb->FogDensity = density;
		cb->FogStart = start;
		cb->FogEnd = end;
		cb->NoiseScale = noiseScale;
		cb->NoiseStrength = noiseStrength;
		cb->Time = time;
		cb->worldScale = worldScale;
		cb->worldPos = worldPos;
	}

	void DirectXCommon::SetAuraParam(
		const Vector2& centerUV,
		const Vector2& topUV,
		const Vector2& bottomUV,
		float aspect,
		float time,
		float radius,
		float intensity,
		float useRing,
		float ringRadius,
		float ringWidth,
		const Vector3& colorA,
		const Vector3& colorB,
		float mix,
		float taper,
		float noiseScale,
		float noiseSpeed,
		float flameStrength,
		float edgePower,
		float verticalFade)
	{
		if (!auraInitialized_) { InitializeAuraPipeline(); }
		if (!auraMappedData_) { return; }

		auto* cb = reinterpret_cast<AuraCB*>(auraMappedData_);
		cb->CenterUV = centerUV;
		cb->TopUV = topUV;
		cb->BottomUV = bottomUV;
		cb->Aspect = aspect;

		cb->Time = time;
		cb->Radius = radius;

		cb->Intensity = intensity;
		cb->UseRing = useRing;
		cb->RingRadius = ringRadius;
		cb->RingWidth = ringWidth;

		cb->ColorA = colorA;
		cb->ColorB = colorB;
		cb->Mix = mix;

		cb->Taper = taper;
		cb->NoiseScale = noiseScale;
		cb->NoiseSpeed = noiseSpeed;
		cb->FlameStrength = flameStrength;
		cb->EdgePower = edgePower;
		cb->VerticalFade = verticalFade;
	}

	void DirectXCommon::InitializeFixFPS() {
		//現在時間を記録する
		reference_ = std::chrono::steady_clock::now();
	}

	void DirectXCommon::UpdateFixFPS() {

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

	void DirectXCommon::PreDraw() {

		// =========================================
		// RenderTexture を描画先にする (9ページ)
		// =========================================
		// kRenderTextureRTVIndex は DirectXCommon.h で 2 に定義されている
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandles[kRenderTextureRTVIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		// RenderTarget / Depth をセット
		commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		// =========================================
		// RenderTexture をクリア (今は赤で確認用)
		// =========================================
		float clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // 背景色：赤
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,
			0,
			0,
			nullptr
		);

		// =========================================
		// ビューポート＆シザー設定
		// =========================================
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
	}

	void DirectXCommon::PostDraw() {
		HRESULT hr;

		// これから書き込むバックバッファのインデックスを取得    
		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		// 1. 描画コマンドの記録 (リソースバリア設定)
		barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandList->ResourceBarrier(1, &barrier);

		// 2. コマンドリストの終了
		hr = commandList->Close();
		assert(SUCCEEDED(hr));

		// 3. コマンドリストをキューに送信
		ID3D12CommandList* commandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(1, commandLists);

		// 4. Present の呼び出し（フレームを表示）
		swapChain->Present(1, 0);

		// 5. フェンスでGPU処理が終了するまで待機
		fenceValue++;
		hr = commandQueue->Signal(fence.Get(), fenceValue);
		assert(SUCCEEDED(hr));

		//FPS固定
		UpdateFixFPS();

		// GPUがコマンドリストの実行を終了するまで待つ
		if (fence->GetCompletedValue() < fenceValue) {
			hr = fence->SetEventOnCompletion(fenceValue, fenceEvent);
			assert(SUCCEEDED(hr));
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		// 6. アロケータのリセット（GPU完了後に実行）
		hr = commandAllocator->Reset();
		assert(SUCCEEDED(hr));

		// 7. コマンドリストのリセット（次のフレームの準備）
		hr = commandList->Reset(commandAllocator.Get(), nullptr);
		assert(SUCCEEDED(hr));
	}
}