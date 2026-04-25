#include "LineRenderer.h"
#include "d3dx12.h"
#include "MyMath.h"
#include <cmath>

using Microsoft::WRL::ComPtr;

namespace TKM {

	LineRenderer* LineRenderer::GetInstance() {
		// LineRendererはシングルトンとして扱う
		static LineRenderer inst;
		return &inst;
	}

	void LineRenderer::Initialize(TKM::DirectXCommon* dxCommon, size_t maxLines) {
		// DirectXCommonを保持する
		dx_ = dxCommon;

		// 1ラインは2頂点なので、最大ライン数から最大頂点数を計算する
		maxVertices_ = maxLines * 2;

		// 毎フレーム使う頂点配列の容量を先に確保しておく
		vertices_.reserve(maxVertices_);

		// ライン描画用の頂点バッファを作成する
		CreateBuffer();

		// ライン描画用のパイプラインを作成する
		CreatePipeline();
	}

	void LineRenderer::BeginFrame() {
		// 1フレーム分のライン情報をクリアする
		vertices_.clear();
	}

	void LineRenderer::AddLine(const Vector3& a, const Vector3& b, const Color& c) {
		// 最大頂点数を超える場合は追加しない
		if (vertices_.size() + 2 > maxVertices_) {
			return;
		}

		// ラインの始点と終点を追加する
		vertices_.push_back({ a, c });
		vertices_.push_back({ b, c });
	}

	void LineRenderer::Draw(const Matrix4x4& viewProj) {
		// 描画するラインがない、またはDirectXCommonがない場合は何もしない
		if (vertices_.empty() || !dx_) {
			return;
		}

		auto* device = dx_->GetDevice();
		auto* cmdList = dx_->GetCommandList();

		// ---- 頂点データをVBに書き込む（基本は毎フレーム全書き換え） ----
		{
			Vertex* mapped = nullptr;
			D3D12_RANGE readRange{ 0, 0 };

			// CPUから頂点バッファへ書き込むためにMapする
			vertexBuffer_->Map(0, &readRange, reinterpret_cast<void**>(&mapped));

			// 今フレーム分のライン頂点をまとめてコピーする
			memcpy(mapped, vertices_.data(), sizeof(Vertex) * vertices_.size());

			// 書き込みが終わったのでUnmapする
			vertexBuffer_->Unmap(0, nullptr);

			// 今回描画する頂点数に合わせてVBビューを更新する
			vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
			vbView_.StrideInBytes = sizeof(Vertex);
			vbView_.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * vertices_.size());
		}

		// ---- パイプラインセットアップ ----
		// ライン描画用のPSOとRootSignatureを設定する
		cmdList->SetPipelineState(pso_.Get());
		cmdList->SetGraphicsRootSignature(rootSig_.Get());

		// ビュー射影行列をRootConstantsとして頂点シェーダへ渡す
		cmdList->SetGraphicsRoot32BitConstants(
			0,
			sizeof(Matrix4x4) / 4,
			&viewProj,
			0
		);

		// ラインリストとして描画する
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		// 頂点バッファを設定する
		cmdList->IASetVertexBuffers(0, 1, &vbView_);

		// 登録されているライン頂点を描画する
		cmdList->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);
	}

	void LineRenderer::AddAABB(const Vector3& center, const Vector3& size, const Color& color) {
		// サイズから各軸の半分の長さを求める
		float hx = size.x * 0.5f;
		float hy = size.y * 0.5f;
		float hz = size.z * 0.5f;

		// AABBの8頂点を作る
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

		// 指定した2頂点を結ぶ線を追加する
		auto add = [&](int a, int b) {
			AddLine(p[a], p[b], color);
			};

		// 手前側の四角形
		add(0, 1);
		add(1, 3);
		add(3, 2);
		add(2, 0);

		// 奥側の四角形
		add(4, 5);
		add(5, 7);
		add(7, 6);
		add(6, 4);

		// 手前と奥をつなぐ辺
		add(0, 4);
		add(1, 5);
		add(2, 6);
		add(3, 7);
	}

	void LineRenderer::AddAABBWithRayHighlight(
		const Vector3& center,
		const Vector3& size,
		const Vector3& rayOrigin,
		const Vector3& rayDirRaw,
		const Color& normalColor,
		const Color& hitColor
	) {
		// レイ方向を正規化するためにコピーする
		Vector3 dir = rayDirRaw;

		// 長さが十分ある場合だけ正規化する
		float len = MyMath::Length(dir);
		if (len > 0.001f) {
			dir = MyMath::Normalize(dir);
		}

		// レイの終点を作る
		Vector3 rayEnd = rayOrigin + dir * 150.0f;

		// AABBとレイ線分の交差判定を行う
		AABB box(center, size);
		bool hit = box.IsIntersectSegment(rayOrigin, rayEnd);

		// 当たっていればhitColor、当たっていなければnormalColorで描画する
		const Color& col = hit ? hitColor : normalColor;
		AddAABB(center, size, col);
	}

	void LineRenderer::AddEllipsoid(
		const Vector3& center,
		const Vector3& radius,
		const Color& color,
		int segments
	) {
		// 最低でも三角形相当の分割数にする
		if (segments < 3) {
			segments = 3;
		}

		const float kPi = 3.1415926535f;
		const float step = (2.0f * kPi) / static_cast<float>(segments);

		for (int i = 0; i < segments; ++i) {
			const float t0 = step * static_cast<float>(i);
			const float t1 = step * static_cast<float>(i + 1);

			// XY平面上の楕円線を追加する
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

			// XZ平面上の楕円線を追加する
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

			// YZ平面上の楕円線を追加する
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

		// viewProj行列をRootConstantsとして頂点シェーダに渡す
		rootParams[0].InitAsConstants(
			sizeof(Matrix4x4) / 4,
			0,
			0,
			D3D12_SHADER_VISIBILITY_VERTEX
		);

		CD3DX12_ROOT_SIGNATURE_DESC rsDesc;

		// 入力アセンブラを使うRootSignatureとして設定する
		rsDesc.Init(
			_countof(rootParams),
			rootParams,
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		ComPtr<ID3DBlob> rsBlob;
		ComPtr<ID3DBlob> errorBlob;

		// RootSignatureをシリアライズする
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&rsBlob,
			&errorBlob
		);

		// シリアライズに失敗した場合はエラー内容を出力する
		if (FAILED(hr)) {
			if (errorBlob) {
				OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			}
			assert(false);
		}

		// RootSignatureを作成する
		hr = device->CreateRootSignature(
			0,
			rsBlob->GetBufferPointer(),
			rsBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSig_)
		);
		assert(SUCCEEDED(hr));

		// === 2) シェーダロード ===
		// ライン描画用の頂点シェーダとピクセルシェーダを読み込む
		ComPtr<IDxcBlob> vsBlob = dx_->CompileShader(
			L"resources/shaders/LineRenderer.VS.hlsl", L"vs_6_0"
		);
		ComPtr<IDxcBlob> psBlob = dx_->CompileShader(
			L"resources/shaders/LineRenderer.PS.hlsl", L"ps_6_0"
		);

		// === 3) 頂点レイアウト ===
		// 1頂点につき、位置と色を持つ
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
		rastDesc.CullMode = D3D12_CULL_MODE_NONE;
		rastDesc.FrontCounterClockwise = FALSE;
		rastDesc.DepthClipEnable = TRUE;

		// アルファブレンドを有効にする
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// 深度テストを有効にして、3D空間上の位置関係に合わせる
		D3D12_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthDesc.StencilEnable = FALSE;

		// === 5) PSO 設定 ===
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

		// RootSignatureとシェーダを設定する
		psoDesc.pRootSignature = rootSig_.Get();
		psoDesc.InputLayout = { inputElems, _countof(inputElems) };
		psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
		psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

		// 描画状態を設定する
		psoDesc.RasterizerState = rastDesc;
		psoDesc.BlendState = blendDesc;
		psoDesc.DepthStencilState = depthDesc;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		// ライン描画用のPrimitiveTopologyにする
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		// レンダーターゲットと深度フォーマットを設定する
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;

		// PSOを作成する
		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
		assert(SUCCEEDED(hr));
	}

	void LineRenderer::CreateBuffer() {
		assert(dx_);

		// 最大ライン数ぶんの頂点バッファサイズを計算する
		size_t sizeInBytes = sizeof(Vertex) * maxVertices_;

		// 毎フレームCPUから書き換えるため、アップロード用バッファを作成する
		vertexBuffer_ = dx_->CreateBufferResource(sizeInBytes);
		assert(vertexBuffer_);

		// 頂点バッファビューを初期化する
		vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
		vbView_.StrideInBytes = sizeof(Vertex);

		// 実際の描画サイズはDraw時に今フレームの頂点数で更新する
		vbView_.SizeInBytes = 0;
	}

} // namespace TKM