#include "JudgementPortalRenderer.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "MyMath.h"
#include "d3dx12.h"
#include <cassert>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

namespace TKM {

	void JudgementPortalRenderer::Initialize(DirectXCommon* dxCommon) {
		assert(dxCommon);
		dxCommon_ = dxCommon;

		CreatePipeline_();
		CreateResources_();
	}

	void JudgementPortalRenderer::Update(float dt) {
		time_ += dt;
	}

	void JudgementPortalRenderer::Draw(
		DirectXCommon* dxCommon,
		const Camera& camera,
		const Vector3& center,
		float charge01,
		int index
	) {
		if (!dxCommon || !pipelineState_ || !rootSignature_) {
			return;
		}

		if (index < 0 || index >= kMaxPortal_) {
			return;
		}

		if (charge01 < 0.0f) charge01 = 0.0f;
		if (charge01 > 1.0f) charge01 = 1.0f;

		Matrix4x4 camWorld = camera.GetWorldMatrix();

		Vector3 camRight = MyMath::Normalize({ camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2] });
		Vector3 camUp = MyMath::Normalize({ camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2] });
		Vector3 camFwd = MyMath::Normalize({ camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2] });

		constMap_[index]->viewProj = camera.GetViewProjectionMatrix();
		constMap_[index]->centerWS = center;
		constMap_[index]->time = time_;

		constMap_[index]->camRight = camRight;
		constMap_[index]->size = size_;

		constMap_[index]->camUp = camUp;
		constMap_[index]->charge01 = charge01;

		constMap_[index]->camFwd = camFwd;
		constMap_[index]->intensity = intensity_;

		auto* commandList = dxCommon->GetCommandList();

		commandList->SetGraphicsRootSignature(rootSignature_.Get());
		commandList->SetPipelineState(pipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vbView_);
		commandList->SetGraphicsRootConstantBufferView(
			0,
			constBuffer_[index]->GetGPUVirtualAddress()
		);

		commandList->DrawInstanced(6, 1, 0, 0);
	}

	void JudgementPortalRenderer::CreatePipeline_() {
		auto device = dxCommon_->GetDevice();

		D3D12_ROOT_PARAMETER rootParam{};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam.Descriptor.ShaderRegister = 0;
		rootParam.Descriptor.RegisterSpace = 0;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.NumParameters = 1;
		rsDesc.pParameters = &rootParam;
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;

		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&sigBlob,
			&errBlob
		);
		assert(SUCCEEDED(hr));

		hr = device->CreateRootSignature(
			0,
			sigBlob->GetBufferPointer(),
			sigBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature_)
		);
		assert(SUCCEEDED(hr));

		auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/JudgementPortal.VS.hlsl", L"vs_6_0");
		auto psBlob = dxCommon_->CompileShader(L"resources/shaders/JudgementPortal.PS.hlsl", L"ps_6_0");

		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rootSignature_.Get();
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
		psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.SampleDesc.Count = 1;

		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		D3D12_DEPTH_STENCIL_DESC dsDesc{};
		dsDesc.DepthEnable = TRUE;
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		dsDesc.StencilEnable = FALSE;
		psoDesc.DepthStencilState = dsDesc;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState = blendDesc;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
		assert(SUCCEEDED(hr));
	}

	void JudgementPortalRenderer::CreateResources_() {
		Vertex vertices[6] = {
			{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
			{{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
			{{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},

			{{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
			{{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
			{{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
		};

		vertexBuffer_ = dxCommon_->CreateBufferResource(sizeof(vertices));

		void* mapped = nullptr;
		vertexBuffer_->Map(0, nullptr, &mapped);
		memcpy(mapped, vertices, sizeof(vertices));
		vertexBuffer_->Unmap(0, nullptr);

		vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		vbView_.SizeInBytes = sizeof(vertices);
		vbView_.StrideInBytes = sizeof(Vertex);

		for (int i = 0; i < kMaxPortal_; ++i) {
			constBuffer_[i] = dxCommon_->CreateBufferResource(sizeof(ConstBuffer));
			constBuffer_[i]->Map(0, nullptr, reinterpret_cast<void**>(&constMap_[i]));
			assert(constMap_[i]);
		}
	}
}