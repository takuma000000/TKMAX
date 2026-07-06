#include "JudgementBackgroundRenderer.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "MyMath.h"
#include <cassert>
#include <d3dcompiler.h>
#include "d3dx12.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace TKM {

	void JudgementBackgroundRenderer::Initialize(DirectXCommon* dxCommon) {
		assert(dxCommon);
		dxCommon_ = dxCommon;

		CreatePipeline_();
		CreateResources_();
	}

	void JudgementBackgroundRenderer::Update(float dt) {
		time_ += dt;

		float target = active_ ? 1.0f : 0.0f;
		float speed = active_ ? 2.5f : 3.5f;

		if (fade_ < target) {
			fade_ += speed * dt;
			if (fade_ > target) {
				fade_ = target;
			}
		} else if (fade_ > target) {
			fade_ -= speed * dt;
			if (fade_ < target) {
				fade_ = target;
			}
		}
	}

	void JudgementBackgroundRenderer::Draw(DirectXCommon* dxCommon, const Camera& camera, const Vector3& center) {
		if (!dxCommon || !pipelineState_ || !rootSignature_ || fade_ <= 0.001f) {
			return;
		}

		DrawInternal_(dxCommon, camera, center, fade_);
	}
	void JudgementBackgroundRenderer::WarmUpDraw(DirectXCommon* dxCommon, const Camera& camera, const Vector3& center) {
		if (warmedUp_) {
			return;
		}
		if (!dxCommon || !pipelineState_ || !rootSignature_) {
			return;
		}

		// intensity 0.0 なので画面には出ないが、
		// PSO / RootSignature / Shader / VB / CB の初回使用だけ済ませる
		DrawInternal_(dxCommon, camera, center, 0.0f);

		warmedUp_ = true;
	}
	void JudgementBackgroundRenderer::DrawInternal_(DirectXCommon* dxCommon, const Camera& camera, const Vector3& center, float intensity) {
		Matrix4x4 camWorld = camera.GetWorldMatrix();

		Vector3 camRight = MyMath::Normalize({ camWorld.m[0][0], camWorld.m[0][1], camWorld.m[0][2] });
		Vector3 camUp = MyMath::Normalize({ camWorld.m[1][0], camWorld.m[1][1], camWorld.m[1][2] });
		Vector3 camFwd = MyMath::Normalize({ camWorld.m[2][0], camWorld.m[2][1], camWorld.m[2][2] });

		constMap_->viewProj = camera.GetViewProjectionMatrix();
		constMap_->centerWS = center;
		constMap_->time = time_;
		constMap_->camRight = camRight;
		constMap_->intensity = intensity;
		constMap_->camUp = camUp;
		constMap_->width = width_;
		constMap_->camFwd = camFwd;
		constMap_->height = height_;

		auto* commandList = dxCommon->GetCommandList();

		commandList->SetGraphicsRootSignature(rootSignature_.Get());
		commandList->SetPipelineState(pipelineState_.Get());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &vbView_);
		commandList->SetGraphicsRootConstantBufferView(0, constBuffer_->GetGPUVirtualAddress());

		commandList->DrawInstanced(6, 1, 0, 0);
	}

	void JudgementBackgroundRenderer::CreatePipeline_() {
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

		auto vsBlob = dxCommon_->CompileShader(L"resources/shaders/JudgementBackground.VS.hlsl", L"vs_6_0");
		auto psBlob = dxCommon_->CompileShader(L"resources/shaders/JudgementBackground.PS.hlsl", L"ps_6_0");

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
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		psoDesc.BlendState = blendDesc;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
		assert(SUCCEEDED(hr));
	}

	void JudgementBackgroundRenderer::CreateResources_() {
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

		constBuffer_ = dxCommon_->CreateBufferResource(sizeof(ConstBuffer));
		constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
		assert(constMap_);
	}
}