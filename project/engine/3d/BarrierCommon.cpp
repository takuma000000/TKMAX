#include "BarrierCommon.h"
#include "Logger.h"
using namespace Logger;

namespace TKM {
	BarrierCommon* BarrierCommon::GetInstance() {
		static BarrierCommon instance;
		return &instance;
	}

	void BarrierCommon::Initialize(DirectXCommon* dxCommon) {
		dxCommon_ = dxCommon;
		GenerateGraphicsPipeline();
	}

	void BarrierCommon::DrawSetCommon() {
		dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
		dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState_.Get());
		dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void BarrierCommon::GenerateRootSignature() {
		descriptionRootSignature_.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;
		descriptorRange[0].NumDescriptors = 1;
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE descriptorRange2[1] = {};
		descriptorRange2[0].BaseShaderRegister = 1;
		descriptorRange2[0].NumDescriptors = 1;
		descriptorRange2[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange2[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_ROOT_PARAMETER rootParameters[10] = {};

		// 0 : Material (PS b0)
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].Descriptor.ShaderRegister = 0;

		// 1 : WVP (VS b0)
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].Descriptor.ShaderRegister = 0;

		// 2 : Texture (PS t0)
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

		// 3 : DirectionalLight (PS b1)
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].Descriptor.ShaderRegister = 1;

		// 4 : Camera (PS b2)
		rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[4].Descriptor.ShaderRegister = 2;

		// 5 : PointLight (PS b3)
		rootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[5].Descriptor.ShaderRegister = 3;

		// 6 : SpotLight (PS b4)
		rootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[6].Descriptor.ShaderRegister = 4;

		// 7 : Environment texture (PS t1)
		rootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[7].DescriptorTable.pDescriptorRanges = descriptorRange2;
		rootParameters[7].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange2);

		// 8 : Environment CB (PS b5)
		rootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[8].Descriptor.ShaderRegister = 5;
		rootParameters[8].Descriptor.RegisterSpace = 0;

		// 9 : Barrier shader param (PS b6)
		rootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[9].Descriptor.ShaderRegister = 6;
		rootParameters[9].Descriptor.RegisterSpace = 0;

		descriptionRootSignature_.pParameters = rootParameters;
		descriptionRootSignature_.NumParameters = _countof(rootParameters);

		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplers[0].ShaderRegister = 0;
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		descriptionRootSignature_.pStaticSamplers = staticSamplers;
		descriptionRootSignature_.NumStaticSamplers = _countof(staticSamplers);

		HRESULT hr;
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlog = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlog = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature_, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlog, &errorBlog);
		if (FAILED(hr)) {
			Log(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
			assert(false);
		}

		hr = dxCommon_->GetDevice()->CreateRootSignature(
			0,
			signatureBlog->GetBufferPointer(),
			signatureBlog->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature_)
		);
		assert(SUCCEEDED(hr));

		inputElementDescs_[0].SemanticName = "POSITION";
		inputElementDescs_[0].SemanticIndex = 0;
		inputElementDescs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		inputElementDescs_[1].SemanticName = "TEXCOORD";
		inputElementDescs_[1].SemanticIndex = 0;
		inputElementDescs_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		inputElementDescs_[2].SemanticName = "NORMAL";
		inputElementDescs_[2].SemanticIndex = 0;
		inputElementDescs_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc_.AlphaToCoverageEnable = FALSE;
		blendDesc_.IndependentBlendEnable = FALSE;

		auto& rt0 = blendDesc_.RenderTarget[0];
		rt0.BlendEnable = TRUE;
		rt0.LogicOpEnable = FALSE;
		rt0.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rt0.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rt0.BlendOp = D3D12_BLEND_OP_ADD;
		rt0.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt0.DestBlendAlpha = D3D12_BLEND_ZERO;
		rt0.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt0.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		resterizerDesc_.CullMode = D3D12_CULL_MODE_NONE;
		resterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID;

		depthStencilDesc_.DepthEnable = true;
		depthStencilDesc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc_.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	void BarrierCommon::GenerateGraphicsPipeline() {
		GenerateRootSignature();

		HRESULT hr;

		vertexShaderBlob_ = dxCommon_->CompileShader(L"resources/shaders/Object3d.VS.hlsl", L"vs_6_0");
		pixelShaderBlob_ = dxCommon_->CompileShader(L"resources/shaders/Barrier.PS.hlsl", L"ps_6_0");

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs_;
		inputLayoutDesc.NumElements = _countof(inputElementDescs_);

		graphicPipelineStateDesc_.pRootSignature = rootSignature_.Get();
		graphicPipelineStateDesc_.InputLayout = inputLayoutDesc;
		graphicPipelineStateDesc_.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
		graphicPipelineStateDesc_.PS = { pixelShaderBlob_->GetBufferPointer(), pixelShaderBlob_->GetBufferSize() };
		graphicPipelineStateDesc_.BlendState = blendDesc_;
		graphicPipelineStateDesc_.RasterizerState = resterizerDesc_;
		graphicPipelineStateDesc_.NumRenderTargets = 1;
		graphicPipelineStateDesc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		graphicPipelineStateDesc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		graphicPipelineStateDesc_.SampleDesc.Count = 1;
		graphicPipelineStateDesc_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		graphicPipelineStateDesc_.DepthStencilState = depthStencilDesc_;
		graphicPipelineStateDesc_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc_, IID_PPV_ARGS(&graphicsPipelineState_));
		assert(SUCCEEDED(hr));
	}
}