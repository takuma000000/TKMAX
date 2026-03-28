#pragma once
#include "DirectXCommon.h"

namespace TKM {
	class BarrierCommon {
	public:
		static BarrierCommon* GetInstance();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="dxCommon"></param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// 描画前の共通セット
		/// </summary>
		void DrawSetCommon();

	private:
		DirectXCommon* dxCommon_ = nullptr;

		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc_{};
		D3D12_INPUT_ELEMENT_DESC inputElementDescs_[3] = {};

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_ = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

		D3D12_BLEND_DESC blendDesc_{};
		D3D12_RASTERIZER_DESC resterizerDesc_{};
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{};

		void GenerateRootSignature();
		void GenerateGraphicsPipeline();

	private:
		BarrierCommon() = default;
		~BarrierCommon() = default;
		BarrierCommon(BarrierCommon&) = delete;
		BarrierCommon& operator=(BarrierCommon&) = delete;
	};
}