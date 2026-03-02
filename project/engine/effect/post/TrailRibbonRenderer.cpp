#define NOMINMAX
#include "TrailRibbonRenderer.h"
#include <d3dcompiler.h>
#include <cassert>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "d3dcompiler.lib")

namespace TKM {

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader_(
		const wchar_t* path,
		const char* entry,
		const char* target
	) {
		UINT flags = 0;
#if defined(_DEBUG)
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		Microsoft::WRL::ComPtr<ID3DBlob> shader;
		Microsoft::WRL::ComPtr<ID3DBlob> errors;
		HRESULT hr = D3DCompileFromFile(
			path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entry, target, flags, 0, &shader, &errors
		);

		if (FAILED(hr)) {
			if (errors) {
				OutputDebugStringA((char*)errors->GetBufferPointer());
			}
			assert(false && "Shader compile failed.");
			return nullptr;
		}
		return shader;
	}

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer_(
		ID3D12Device* device,
		size_t sizeBytes
	) {
		D3D12_HEAP_PROPERTIES heap{};
		heap.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = (UINT64)sizeBytes;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> res;
		HRESULT hr = device->CreateCommittedResource(
			&heap, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&res)
		);
		assert(SUCCEEDED(hr));
		return res;
	}

	void TrailRibbonRenderer::Initialize(DirectXCommon* dxCommon) {
		assert(dxCommon);
		CreatePipeline_(dxCommon);

		// CB
		auto device = dxCommon->GetDevice();
		cb_ = CreateUploadBuffer_(device, sizeof(CB));
		cb_->Map(0, nullptr, (void**)&cbMapped_);
		assert(cbMapped_);

		// まず小さめ確保（必要に応じて拡張）
		EnsureBuffers_(device, 2048, 4096);
	}

	void TrailRibbonRenderer::Update(float dt) {
		time_ += dt;
		if (cbMapped_) {
			cbMapped_->time = time_;
		}
	}

	void TrailRibbonRenderer::EnsureBuffers_(ID3D12Device* device, uint32_t maxVerts, uint32_t maxIndices) {
		if (vb_ && vbCapacity_ >= maxVerts && ib_ && ibCapacity_ >= maxIndices) { return; }

		vbCapacity_ = std::max(vbCapacity_, maxVerts);
		ibCapacity_ = std::max(ibCapacity_, maxIndices);

		vb_ = CreateUploadBuffer_(device, sizeof(Vertex) * (size_t)vbCapacity_);
		ib_ = CreateUploadBuffer_(device, sizeof(uint16_t) * (size_t)ibCapacity_);

		vbView_.BufferLocation = vb_->GetGPUVirtualAddress();
		vbView_.StrideInBytes = sizeof(Vertex);
		vbView_.SizeInBytes = (UINT)(sizeof(Vertex) * vbCapacity_);

		ibView_.BufferLocation = ib_->GetGPUVirtualAddress();
		ibView_.Format = DXGI_FORMAT_R16_UINT;
		ibView_.SizeInBytes = (UINT)(sizeof(uint16_t) * ibCapacity_);
	}

	void TrailRibbonRenderer::DrawRibbon(
		DirectXCommon* dxCommon,
		const Camera& camera,
		const std::vector<Vector3>& points,
		float headWidth,
		float tailWidth,
		float intensity,
		const Vector3& color,
		float uvTiling,
		float uvScroll
	) {
		if (!dxCommon) { return; }
		if (!cbMapped_) { return; }
		if (points.size() < 2) { return; }

		// メッシュ生成
		tmpVerts_.clear();
		tmpIndices_.clear();
		BuildRibbonMesh_(camera, points, headWidth, tailWidth, color, uvTiling, tmpVerts_, tmpIndices_);

		if (tmpVerts_.empty() || tmpIndices_.empty()) { return; }

		// バッファ確保
		auto device = dxCommon->GetDevice();
		EnsureBuffers_(device, (uint32_t)tmpVerts_.size(), (uint32_t)tmpIndices_.size());

		// Uploadへ書き込み
		{
			void* vbMap = nullptr;
			vb_->Map(0, nullptr, &vbMap);
			memcpy(vbMap, tmpVerts_.data(), sizeof(Vertex) * tmpVerts_.size());
			vb_->Unmap(0, nullptr);

			void* ibMap = nullptr;
			ib_->Map(0, nullptr, &ibMap);
			memcpy(ibMap, tmpIndices_.data(), sizeof(uint16_t) * tmpIndices_.size());
			ib_->Unmap(0, nullptr);
		}

		// CB更新
		cbMapped_->viewProj = camera.GetViewProjectionMatrix();
		cbMapped_->uvScroll = uvScroll;
		cbMapped_->intensity = intensity;

		// 描画
		auto cmd = dxCommon->GetCommandList();
		cmd->SetPipelineState(pso_.Get());
		cmd->SetGraphicsRootSignature(rootSig_.Get());

		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->IASetVertexBuffers(0, 1, &vbView_);
		cmd->IASetIndexBuffer(&ibView_);

		// RootParam0 = CBV
		cmd->SetGraphicsRootConstantBufferView(0, cb_->GetGPUVirtualAddress());

		cmd->DrawIndexedInstanced((UINT)tmpIndices_.size(), 1, 0, 0, 0);
	}

	void TrailRibbonRenderer::BuildRibbonMesh_(
		const Camera& camera,
		const std::vector<Vector3>& points,
		float headWidth,
		float tailWidth,
		const Vector3& color,
		float uvTiling,
		std::vector<Vertex>& outVerts,
		std::vector<uint16_t>& outIndices
	) {
		// points[0] = head(最新) を想定
		const uint32_t n = (uint32_t)points.size();

		// 累積距離（u用）
		std::vector<float> dist(n, 0.0f);
		for (uint32_t i = 1; i < n; ++i) {
			Vector3 d = points[i] - points[i - 1];
			float len = MyMath::Length(d);
			dist[i] = dist[i - 1] + len;
		}

		outVerts.reserve(n * 2);
		outIndices.reserve((n - 1) * 6);

		Vector3 camPos = camera.GetTranslate(); // ← あなたのCameraに合わせて要調整の可能性あり
		Vector3 prevSide = { 1,0,0 };

		for (uint32_t i = 0; i < n; ++i) {
			const Vector3& p = points[i];

			// tangent
			Vector3 prev = (i == 0) ? points[i] : points[i - 1];
			Vector3 next = (i + 1 < n) ? points[i + 1] : points[i];
			Vector3 t = next - prev;
			float tLen = MyMath::Length(t);
			if (tLen < 0.0001f) {
				t = { 0,0,1 };
			} else {
				t = t / tLen;
			}

			// world up 基準で side を作る（完全ワールド固定）
			Vector3 worldUp = { 0.0f, 1.0f, 0.0f };

			Vector3 side = MyMath::Cross(t, worldUp);
			float sLen = MyMath::Length(side);

			if (sLen < 0.0001f) {
				worldUp = { 1.0f, 0.0f, 0.0f }; // fallback
				side = MyMath::Cross(t, worldUp);
				sLen = MyMath::Length(side);
			}

			side = side / sLen;

			float a = (n <= 1) ? 1.0f : (float)i / (float)(n - 1); // 0=head,1=tail ではなく逆なので注意
			// headを太く：i=0がhead
			float width = headWidth + (tailWidth - headWidth) * a;

			Vector3 left = p - side * (width * 0.5f);
			Vector3 right = p + side * (width * 0.5f);

			float u = dist[i] * uvTiling;

			Vertex vl{};
			vl.pos = left;
			vl.uv = { u, 0.0f };
			vl.color = { color.x, color.y, color.z, 1.0f };
			vl.age01 = a;

			Vertex vr{};
			vr.pos = right;
			vr.uv = { u, 1.0f };
			vr.color = { color.x, color.y, color.z, 1.0f };
			vr.age01 = a;

			uint16_t base = (uint16_t)outVerts.size();
			outVerts.push_back(vl);
			outVerts.push_back(vr);

			if (i + 1 < n) {
				// (base, base+1, base+2, base+3) -> 2 triangles
				outIndices.push_back(base + 0);
				outIndices.push_back(base + 2);
				outIndices.push_back(base + 1);

				outIndices.push_back(base + 1);
				outIndices.push_back(base + 2);
				outIndices.push_back(base + 3);
			}
		}
	}

	void TrailRibbonRenderer::CreatePipeline_(DirectXCommon* dxCommon) {
		auto device = dxCommon->GetDevice();

		// RootSig: CBV(b0) だけ（最短構成）
		D3D12_ROOT_PARAMETER rp[1]{};
		rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rp[0].Descriptor.ShaderRegister = 0;
		rp[0].Descriptor.RegisterSpace = 0;
		rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.NumParameters = 1;
		rsDesc.pParameters = rp;
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;
		HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);
		if (FAILED(hr)) {
			if (errBlob) { OutputDebugStringA((char*)errBlob->GetBufferPointer()); }
			assert(false);
		}

		hr = device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig_));
		assert(SUCCEEDED(hr));

		// Shaders
		auto vs = CompileShader_(L"resources/shaders/TrailRibbon.VS.hlsl", "main", "vs_5_0");
		auto ps = CompileShader_(L"resources/shaders/TrailRibbon.PS.hlsl", "main", "ps_5_0");

		// InputLayout
		D3D12_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT,       0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// Blend(Additive)
		D3D12_BLEND_DESC blend{};
		blend.AlphaToCoverageEnable = FALSE;
		blend.IndependentBlendEnable = FALSE;
		auto& rt = blend.RenderTarget[0];
		rt.BlendEnable = TRUE;
		rt.LogicOpEnable = FALSE;
		rt.SrcBlend = D3D12_BLEND_ONE;
		rt.DestBlend = D3D12_BLEND_ONE;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ONE;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt.LogicOp = D3D12_LOGIC_OP_NOOP;
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rootSig_.Get();
		psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
		psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
		psoDesc.BlendState = blend;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// Rasterizer
		D3D12_RASTERIZER_DESC rast{};
		rast.FillMode = D3D12_FILL_MODE_SOLID;
		rast.CullMode = D3D12_CULL_MODE_NONE;
		rast.FrontCounterClockwise = FALSE;
		rast.DepthClipEnable = TRUE;
		psoDesc.RasterizerState = rast;

		// Depth: test ON / write OFF
		D3D12_DEPTH_STENCIL_DESC ds{};
		ds.DepthEnable = TRUE;
		ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		ds.StencilEnable = FALSE;
		psoDesc.DepthStencilState = ds;

		psoDesc.InputLayout = { layout, _countof(layout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// ※あなたのDirectXCommonに合わせて RTV/DSV format を調整する必要がある場合あり
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
		assert(SUCCEEDED(hr));
	}

} // namespace TKM