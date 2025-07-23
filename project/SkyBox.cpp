#include "Skybox.h"
#include <externals/DirectXTex/d3dx12.h>
#include <dxcapi.h> // DXC関連
#include "externals/imGui/imgui.h"

void Skybox::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, const std::string& texturePath) {
    dxCommon_ = dxCommon;

	TextureManager::GetInstance()->LoadTexture(texturePath);
	srvHandleGPU_ = TextureManager::GetInstance()->GetSrvHandleGPU(texturePath);

    // 定数バッファ
    constantBuffer_ = dxCommon_->CreateBufferResource(sizeof(TransformationMatrix));
    constantBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData_));

    materialBuffer_ = dxCommon_->CreateBufferResource(sizeof(Material));
    materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterial_));

    *mappedMaterial_ = {
        {1.0f, 1.0f, 1.0f, 1.0f},
        1,
        MyMath::MakeIdentity4x4(),
        1.0f
    };

    CreateVertexBuffer();
    CreateRootSignature();
    CreatePipelineState();
}


void Skybox::CreateVertexBuffer() {
	// 1x1x1のキューブを構成する36頂点
	std::vector<Vertex> vertices = {
		// +Y面（上）
		{{-1,  1, -1}}, {{ 1,  1, -1}}, {{ 1,  1,  1}},
		{{-1,  1, -1}}, {{ 1,  1,  1}}, {{-1,  1,  1}},

		// -Y面（下）
		{{-1, -1, -1}}, {{ 1, -1,  1}}, {{ 1, -1, -1}},
		{{-1, -1, -1}}, {{-1, -1,  1}}, {{ 1, -1,  1}},

		// -X面（左）
		{{-1, -1, -1}}, {{-1,  1,  1}}, {{-1,  1, -1}},
		{{-1, -1, -1}}, {{-1, -1,  1}}, {{-1,  1,  1}},

		// +X面（右）
		{{ 1, -1, -1}}, {{ 1,  1, -1}}, {{ 1,  1,  1}},
		{{ 1, -1, -1}}, {{ 1,  1,  1}}, {{ 1, -1,  1}},

		// -Z面（奥）
		{{-1, -1, -1}}, {{ 1,  1, -1}}, {{ 1, -1, -1}},
		{{-1, -1, -1}}, {{-1,  1, -1}}, {{ 1,  1, -1}},

		// +Z面（手前）
		{{-1, -1,  1}}, {{ 1, -1,  1}}, {{ 1,  1,  1}},
		{{-1, -1,  1}}, {{ 1,  1,  1}}, {{-1,  1,  1}},
	};


	vertexCount_ = static_cast<UINT>(vertices.size());
	size_t bufferSize = sizeof(Vertex) * vertexCount_;

	// 頂点バッファ生成
	vertexBuffer_ = dxCommon_->CreateBufferResource(bufferSize);

	// 書き込み
	void* mapped = nullptr;
	vertexBuffer_->Map(0, nullptr, &mapped);
	memcpy(mapped, vertices.data(), bufferSize);
	vertexBuffer_->Unmap(0, nullptr);

	// ビューの設定
	vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
	vertexBufferView_.StrideInBytes = sizeof(Vertex);
}

void Skybox::CreateRootSignature() {
	D3D12_DESCRIPTOR_RANGE descRange{};
	descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange.NumDescriptors = 1;
	descRange.BaseShaderRegister = 0;
	descRange.RegisterSpace = 0;
	descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParams[3]{};

	// b0: VP行列用CBV（VS用）
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// b0: Material (PS) ← これを追加！
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].Descriptor.ShaderRegister = 0;
	rootParams[1].Descriptor.RegisterSpace = 0;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// t0: Cubemap用SRV（PS用）
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &descRange;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
	rootSigDesc.NumParameters = _countof(rootParams);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = &samplerDesc;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	hr = dxCommon_->GetDevice()->CreateRootSignature(
		0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));
}


void Skybox::CreatePipelineState() {
	HRESULT hr;

	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	Microsoft::WRL::ComPtr<IDxcBlob> vsBlob;
	vsBlob = dxCommon_->CompileShader(L"SkyBox.VS.hlsl", L"vs_6_0");
	Microsoft::WRL::ComPtr<IDxcBlob> psBlob;
	psBlob = dxCommon_->CompileShader(L"SkyBox.PS.hlsl", L"ps_6_0");

	assert(vsBlob != nullptr);
	assert(psBlob != nullptr);

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
		  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	// Skybox用深度設定
	psoDesc.DepthStencilState.DepthEnable = true;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));
}

void Skybox::Draw() {
	if (!camera_) return;

	ID3D12GraphicsCommandList* cmdList = dxCommon_->GetCommandList();
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// カメラの View / Projection を取得してスカイボックス用に調整
	Matrix4x4 view = camera_->GetViewMatrix();
	Matrix4x4 proj = camera_->GetProjectionMatrix();

	// カメラ位置除去
	view.m[3][0] = 0.0f;
	view.m[3][1] = 0.0f;
	view.m[3][2] = 0.0f;

	Matrix4x4 world = MyMath::MakeScaleMatrix(scale_);

	mappedData_->viewProjection = MyMath::Multiply(view, proj);
	mappedData_->world = world;

	cmdList->SetGraphicsRootConstantBufferView(0, constantBuffer_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, materialBuffer_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootDescriptorTable(2, srvHandleGPU_);

	cmdList->DrawInstanced(vertexCount_, 1, 0, 0);
}

void Skybox::ImGuiUpdate()
{
	ImGui::Begin("Skybox");
	ImGui::DragFloat3("Scale", &scale_.x, 0.1f, 0.0f, 100.0f);
	/*ImGui::DragFloat3("Rotation", &rotation_.x, 0.1f, 0.0f, 360.0f);
	ImGui::DragFloat3("Translation", &translation_.x, 0.1f, -10.0f, 10.0f);*/
	ImGui::End();
}


