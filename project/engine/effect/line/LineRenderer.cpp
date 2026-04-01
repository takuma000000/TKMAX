#include "LineRenderer.h"
#include "d3dx12.h"
#include "MyMath.h"
#include <cmath>

using Microsoft::WRL::ComPtr;

namespace TKM {
	LineRenderer* LineRenderer::GetInstance() {
		static LineRenderer inst;
		return &inst;
	}

	void LineRenderer::Initialize(TKM::DirectXCommon* dxCommon, size_t maxLines) {
		dx_ = dxCommon;
		maxVertices_ = maxLines * 2;
		vertices_.reserve(maxVertices_);

		CreateBuffer();
		CreatePipeline();
	}

	void LineRenderer::BeginFrame() {
		vertices_.clear();
	}

	void LineRenderer::AddLine(const Vector3& a, const Vector3& b, const Color& c) {
		if (vertices_.size() + 2 > maxVertices_) {
			return;
		}
		vertices_.push_back({ a, c });
		vertices_.push_back({ b, c });
	}

	void LineRenderer::Draw(const Matrix4x4& viewProj) {
		if (vertices_.empty() || !dx_) return;

		auto* device = dx_->GetDevice();
		auto* cmdList = dx_->GetCommandList();

		// ---- 頂点データをVBに書き込む（基本は毎フレーム全書き換えでOK） ----
		{
			Vertex* mapped = nullptr;
			D3D12_RANGE readRange{ 0, 0 };
			vertexBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&mapped));
			memcpy(mapped, vertices_.data(), sizeof(Vertex) * vertices_.size());
			vertexBuffer_->Unmap(0, nullptr);

			vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
			vbView_.StrideInBytes = sizeof(Vertex);
			vbView_.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertices_.size());
		}

		// ---- パイプラインセットアップ ----
		cmdList->SetPipelineState(pso_.Get());
		cmdList->SetGraphicsRootSignature(rootSig_.Get());

		cmdList->SetGraphicsRoot32BitConstants(
			0,                         // root param index
			sizeof(Matrix4x4) / 4,     // 32bit単位の数
			&viewProj,                 // データ
			0
		);

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		cmdList->IASetVertexBuffers(0, 1, &vbView_);
		cmdList->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);
	}

	void LineRenderer::AddAABB(const Vector3& center, const Vector3& size, const Color& color) {
		float hx = size.x * 0.5f;
		float hy = size.y * 0.5f;
		float hz = size.z * 0.5f;

		Vector3 p[8] = {
			{ center.x - hx, center.y - hy, center.z - hz },
			{ center.x + hx, center.y - hy, center.z - hz },
			{ center.x - hx, center.y + hy, center.z - hz },
			{ center.x + hx, center.y + hy, center.z - hz },
			{ center.x - hx, center.y - hy, center.z + hz },
			{ center.x + hx, center.y - hy, center.z + hz },
			{ center.x - hx, center.y + hy, center.z + hz },
			{ center.x + hx, center.y + hy, center.z + hz },
		};

		auto add = [&](int a, int b) {
			AddLine(p[a], p[b], color);
			};

		add(0, 1); add(1, 3); add(3, 2); add(2, 0);
		add(4, 5); add(5, 7); add(7, 6); add(6, 4);
		add(0, 4); add(1, 5); add(2, 6); add(3, 7);
	}

	void LineRenderer::AddAABBWithRayHighlight(
		const Vector3& center,
		const Vector3& size,
		const Vector3& rayOrigin,
		const Vector3& rayDirRaw,
		const Color& normalColor,
		const Color& hitColor
	) {
		Vector3 dir = rayDirRaw;
		float len = MyMath::Length(dir);
		if (len > 0.001f) {
			dir = MyMath::Normalize(dir);
		}

		Vector3 rayEnd = rayOrigin + dir * 150.0f; // 距離は今まで通り

		AABB box(center, size);
		bool hit = box.IsIntersectSegment(rayOrigin, rayEnd);

		const Color& col = hit ? hitColor : normalColor;
		AddAABB(center, size, col);
	}

	void LineRenderer::AddEllipsoid(
		const Vector3& center,
		const Vector3& radius,
		const Color& color,
		int segments
	) {
		if (segments < 3) {
			segments = 3;
		}

		const float kPi = 3.1415926535f;
		const float step = (2.0f * kPi) / static_cast<float>(segments);

		for (int i = 0; i < segments; ++i) {
			const float t0 = step * static_cast<float>(i);
			const float t1 = step * static_cast<float>(i + 1);

			// XY平面
			Vector3 xy0 = {
				center.x + std::cos(t0) * radius.x,
				center.y + std::sin(t0) * radius.y,
				center.z
			};
			Vector3 xy1 = {
				center.x + std::cos(t1) * radius.x,
				center.y + std::sin(t1) * radius.y,
				center.z
			};
			AddLine(xy0, xy1, color);

			// XZ平面
			Vector3 xz0 = {
				center.x + std::cos(t0) * radius.x,
				center.y,
				center.z + std::sin(t0) * radius.z
			};
			Vector3 xz1 = {
				center.x + std::cos(t1) * radius.x,
				center.y,
				center.z + std::sin(t1) * radius.z
			};
			AddLine(xz0, xz1, color);

			// YZ平面
			Vector3 yz0 = {
				center.x,
				center.y + std::cos(t0) * radius.y,
				center.z + std::sin(t0) * radius.z
			};
			Vector3 yz1 = {
				center.x,
				center.y + std::cos(t1) * radius.y,
				center.z + std::sin(t1) * radius.z
			};
			AddLine(yz0, yz1, color);
		}
	}

	void LineRenderer::CreatePipeline() {
		auto* device = dx_->GetDevice();

		// === 1) RootSignature ===
		CD3DX12_ROOT_PARAMETER rootParams[1];
		rootParams[0].InitAsConstants(
			sizeof(Matrix4x4) / 4, // 16個のfloat = 16 DWORD
			0,                      // b0
			0,
			D3D12_SHADER_VISIBILITY_VERTEX
		);

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc;
		rsDesc.Init(
			_countof(rootParams),
			rootParams,
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		ComPtr<ID3DBlob> rsBlob;
		ComPtr<ID3DBlob> errorBlob;
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&rsBlob,
			&errorBlob
		);
		if (FAILED(hr)) {
			if (errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			assert(false);
		}

		hr = device->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSig_)
		);
		assert(SUCCEEDED(hr));

		// === 2) シェーダロード ===
		// パスは自分のプロジェクト構成に合わせて変えてね
		ComPtr<IDxcBlob> vsBlob = dx_->CompileShader(
			L"resources/shaders/LineRenderer.VS.hlsl", L"vs_6_0");
		ComPtr<IDxcBlob> psBlob = dx_->CompileShader(
			L"resources/shaders/LineRenderer.PS.hlsl", L"ps_6_0");

		// === 3) 頂点レイアウト ===
		D3D12_INPUT_ELEMENT_DESC inputElems[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
			  0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,
			  0, D3D12_APPEND_ALIGNED_ELEMENT,
			  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// === 4) ラスタライザ / ブレンド / 深度 ===
		D3D12_RASTERIZER_DESC rastDesc{};
		rastDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rastDesc.CullMode = D3D12_CULL_MODE_NONE; // 線なのでカリングなし
		rastDesc.FrontCounterClockwise = FALSE;
		rastDesc.DepthClipEnable = TRUE;

		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthDesc.StencilEnable = FALSE;

		// === 5) PSO 設定 ===
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rootSig_.Get();
		psoDesc.InputLayout = { inputElems, _countof(inputElems) };
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
		psoDesc.RasterizerState = rastDesc;
		psoDesc.BlendState = blendDesc;
		psoDesc.DepthStencilState = depthDesc;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;

		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
		assert(SUCCEEDED(hr));
	}

	void LineRenderer::CreateBuffer() {
		assert(dx_);

		// ライン最大数ぶんの頂点バッファを作る（1ライン=2頂点）
		size_t sizeInBytes = sizeof(Vertex) * maxVertices_;

		// アップロードヒープのバッファを作成（中身は毎フレーム書き換える）
		vertexBuffer_ = dx_->CreateBufferResource(sizeInBytes);
		assert(vertexBuffer_);

		// VBビュー初期化（SizeInBytes は Draw のときに更新する）
		vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		vbView_.StrideInBytes = sizeof(Vertex);
		vbView_.SizeInBytes = 0;
	}
}