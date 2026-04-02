#define NOMINMAX
#include "TrailRibbonRenderer.h"
#include <d3dcompiler.h>
#include <cassert>
#include <algorithm>
#include <cmath>
#ifdef USE_IMGUI
#include "imgui.h"
#endif

#pragma comment(lib, "d3dcompiler.lib")

namespace TKM {

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader_(
		const wchar_t* path,
		const char* entry,
		const char* target
	) {
		UINT flags = 0;
#if defined(_DEBUG)
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // デバッグ時は最適化をオフにして、デバッグ情報を埋め込む
#endif

		Microsoft::WRL::ComPtr<ID3DBlob> shader; // コンパイルされたシェーダーコードを格納するID3DBlob
		Microsoft::WRL::ComPtr<ID3DBlob> errors; // コンパイルエラーのメッセージを格納するID3DBlob

		// D3DCompileFromFile関数を呼び出して、指定されたファイルからシェーダーコードをコンパイルします。
		HRESULT hr = D3DCompileFromFile(
			path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entry, target, flags, 0, &shader, &errors
		);

		// コンパイルに失敗した場合、エラーメッセージをデバッグ出力に表示し、アサートで停止します。
		if (FAILED(hr)) {
			// エラーがある場合は、その内容をデバッグ出力に表示します。エラーの内容は、errorsというID3DBlobに格納されているため、GetBufferPointer()を呼び出して文字列として取得します。
			if (errors) {
				OutputDebugStringA((char*)errors->GetBufferPointer()); // エラーメッセージをデバッグ出力に表示
			}
			assert(false && "Shader compile failed."); // アサートで停止して、シェーダーのコンパイルが失敗したことを示します。
			return nullptr; // コンパイルに失敗した場合は、nullptrを返します。
		}
		return shader; // コンパイルに成功した場合は、コンパイルされたシェーダーコードを格納するID3DBlobを返します。
	}

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer_(
		ID3D12Device* device,
		size_t sizeBytes
	) {
		D3D12_HEAP_PROPERTIES heap{}; // ヒーププロパティを設定します。ここでは、アップロード用のヒープを指定しています。
		heap.Type = D3D12_HEAP_TYPE_UPLOAD; // ヒープの種類をアップロード用に設定します。これにより、CPUからGPUへのデータ転送が効率的になります。

		// リソースの説明を設定します。ここでは、バッファリソースを作成するための説明を指定しています。
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // リソースの次元をバッファに設定します。
		desc.Width = (UINT64)sizeBytes; // バッファのサイズをバイト単位で指定します。sizeBytesは、作成するバッファのサイズを表す引数です。
		desc.Height = 1; // バッファは1行のデータとして扱うため、高さを1に設定します。
		desc.DepthOrArraySize = 1; // バッファは3Dテクスチャや配列ではないため、深さまたは配列サイズを1に設定します。
		desc.MipLevels = 1; // バッファはミップマップを使用しないため、ミップレベル数を1に設定します。
		desc.SampleDesc.Count = 1; // バッファはマルチサンプリングを使用しないため、サンプル数を1に設定します。 
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // バッファは行優先のレイアウトであるため、テクスチャレイアウトを行優先に設定します。

		// CreateCommittedResource関数を呼び出して、指定されたヒーププロパティとリソース説明に基づいて、コミットされたリソースを作成します。
		Microsoft::WRL::ComPtr<ID3D12Resource> res;
		// この関数は、リソースを作成するためのヒープを自動的に割り当てます。D3D12_HEAP_FLAG_NONEは、ヒープのフラグを指定します。ここでは、特別なフラグは使用しないため、NONEを指定しています。
		HRESULT hr = device->CreateCommittedResource(
			&heap, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&res)
		);
		assert(SUCCEEDED(hr)); // リソースの作成に成功したことを確認します。もし失敗していた場合は、アサートで停止します。
		return res; // 作成されたリソースを返します。呼び出し元は、このリソースを使用してデータの転送や描画などの操作を行うことができます。
	}

	void TrailRibbonRenderer::Initialize(DirectXCommon* dxCommon) {
		assert(dxCommon);
		CreatePipeline_(dxCommon); // パイプラインステートとルートシグネチャの作成

		auto device = dxCommon->GetDevice();
		(void)device;

		drawCount_ = 0;
		for (int i = 0; i < kFrameRing_; ++i) {
			drawVB_[i].clear();
			drawIB_[i].clear();
			drawCB_[i].clear();

			drawVBMapped_[i].clear();
			drawIBMapped_[i].clear();
			drawCBMapped_[i].clear();

			drawVBCapacity_[i].clear();
			drawIBCapacity_[i].clear();
		}
	}

	void TrailRibbonRenderer::Update(float dt) {
		time_ += dt;

		// フレームごとにリングバッファを回す
		frameIndex_ = (frameIndex_ + 1) % kFrameRing_;
		drawCount_ = 0;

		// デバッグ表示
		ImGuiDebug();
	}

	void TrailRibbonRenderer::EnsureBuffers_(ID3D12Device* device, uint32_t maxVerts, uint32_t maxIndices) {
		if (vb_ && vbCapacity_ >= maxVerts && ib_ && ibCapacity_ >= maxIndices) { return; } // 既に十分な容量のバッファがある場合は、何もしません。これにより、不要なバッファの再作成を避けることができます。
		// 必要に応じてバッファを拡張します。新しい容量は、現在の容量と要求された最大容量のうち大きい方になります。これにより、将来の描画で同じサイズのバッファを再利用できるようになります。
		vbCapacity_ = std::max(vbCapacity_, maxVerts);
		// インデックスバッファの容量を更新します。新しい容量は、現在の容量と要求された最大容量のうち大きい方になります。これにより、将来の描画で同じサイズのバッファを再利用できるようになります。
		ibCapacity_ = std::max(ibCapacity_, maxIndices);

		// 新しいバッファを作成します。頂点バッファとインデックスバッファの両方を作成します。これらのバッファは、指定された最大頂点数と最大インデックス数に基づいてサイズが決定されます。
		vb_ = CreateUploadBuffer_(device, sizeof(Vertex) * (size_t)vbCapacity_);
		// 頂点バッファを作成します。サイズは、Vertex構造体のサイズに基づいて、要求された最大頂点数に応じて決定されます。
		ib_ = CreateUploadBuffer_(device, sizeof(uint16_t) * (size_t)ibCapacity_);

		// 頂点バッファビューとインデックスバッファビューを設定します。これらのビューは、描画コマンドで使用されるバッファの場所とサイズを指定します。
		vbView_.BufferLocation = vb_->GetGPUVirtualAddress();
		vbView_.StrideInBytes = sizeof(Vertex);
		vbView_.SizeInBytes = (UINT)(sizeof(Vertex) * vbCapacity_);
		// 頂点バッファビューを設定します。BufferLocationは、頂点バッファのGPU仮想アドレスを指定します。StrideInBytesは、各頂点のサイズをバイト単位で指定します。SizeInBytesは、頂点バッファ全体のサイズをバイト単位で指定します。
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
		if (points.size() < 2) { return; }

		// メッシュ生成
		tmpVerts_.clear();
		tmpIndices_.clear();
		BuildRibbonMesh_(camera, points, headWidth, tailWidth, color, uvTiling, tmpVerts_, tmpIndices_);
		if (tmpVerts_.empty() || tmpIndices_.empty()) { return; }

		auto device = dxCommon->GetDevice();
		const uint32_t needVerts = static_cast<uint32_t>(tmpVerts_.size());
		const uint32_t needIndices = static_cast<uint32_t>(tmpIndices_.size());

		// 今フレームで使うスロット番号
		const uint32_t slot = drawCount_++;
		const int fi = frameIndex_;

		// スロット数を必要数まで拡張
		if (drawVB_[fi].size() <= slot) {
			drawVB_[fi].resize(slot + 1);
			drawIB_[fi].resize(slot + 1);
			drawCB_[fi].resize(slot + 1);

			drawVBMapped_[fi].resize(slot + 1, nullptr);
			drawIBMapped_[fi].resize(slot + 1, nullptr);
			drawCBMapped_[fi].resize(slot + 1, nullptr);

			drawVBCapacity_[fi].resize(slot + 1, 0);
			drawIBCapacity_[fi].resize(slot + 1, 0);
		}

		// =========================
		// VB を必要なら再作成
		// =========================
		if (!drawVB_[fi][slot] || drawVBCapacity_[fi][slot] < needVerts) {
			uint32_t newCapacity = needVerts;
			if (drawVBCapacity_[fi][slot] > 0) {
				newCapacity = (std::max)(needVerts, drawVBCapacity_[fi][slot] * 2);
			}

			drawVB_[fi][slot] = CreateUploadBuffer_(device, sizeof(Vertex) * static_cast<size_t>(newCapacity));
			drawVBMapped_[fi][slot] = nullptr;
			drawVB_[fi][slot]->Map(0, nullptr, reinterpret_cast<void**>(&drawVBMapped_[fi][slot]));
			drawVBCapacity_[fi][slot] = newCapacity;
		}

		// =========================
		// IB を必要なら再作成
		// =========================
		if (!drawIB_[fi][slot] || drawIBCapacity_[fi][slot] < needIndices) {
			uint32_t newCapacity = needIndices;
			if (drawIBCapacity_[fi][slot] > 0) {
				newCapacity = (std::max)(needIndices, drawIBCapacity_[fi][slot] * 2);
			}

			drawIB_[fi][slot] = CreateUploadBuffer_(device, sizeof(uint16_t) * static_cast<size_t>(newCapacity));
			drawIBMapped_[fi][slot] = nullptr;
			drawIB_[fi][slot]->Map(0, nullptr, reinterpret_cast<void**>(&drawIBMapped_[fi][slot]));
			drawIBCapacity_[fi][slot] = newCapacity;
		}

		// =========================
		// CB は1スロット1個を使い回す
		// =========================
		if (!drawCB_[fi][slot]) {
			UINT cbSize = static_cast<UINT>(sizeof(CB));
			cbSize = (cbSize + 255) & ~255u;

			drawCB_[fi][slot] = CreateUploadBuffer_(device, cbSize);
			drawCBMapped_[fi][slot] = nullptr;
			drawCB_[fi][slot]->Map(0, nullptr, reinterpret_cast<void**>(&drawCBMapped_[fi][slot]));
		}

		// =========================
		// データ書き込み
		// =========================
		memcpy(drawVBMapped_[fi][slot], tmpVerts_.data(), sizeof(Vertex) * tmpVerts_.size());
		memcpy(drawIBMapped_[fi][slot], tmpIndices_.data(), sizeof(uint16_t) * tmpIndices_.size());

		CB* cbPtr = drawCBMapped_[fi][slot];
		cbPtr->viewProj = camera.GetViewProjectionMatrix();
		cbPtr->time = time_;
		cbPtr->uvScroll = uvScroll;
		cbPtr->intensity = intensity;
		cbPtr->pad0 = 0.0f;

		// この draw で使うビューを作成
		D3D12_VERTEX_BUFFER_VIEW vbView{};
		vbView.BufferLocation = drawVB_[fi][slot]->GetGPUVirtualAddress();
		vbView.StrideInBytes = sizeof(Vertex);
		vbView.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * tmpVerts_.size());

		D3D12_INDEX_BUFFER_VIEW ibView{};
		ibView.BufferLocation = drawIB_[fi][slot]->GetGPUVirtualAddress();
		ibView.Format = DXGI_FORMAT_R16_UINT;
		ibView.SizeInBytes = static_cast<UINT>(sizeof(uint16_t) * tmpIndices_.size());

		// 描画
		auto cmd = dxCommon->GetCommandList();
		cmd->SetPipelineState(pso_.Get());
		cmd->SetGraphicsRootSignature(rootSig_.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmd->IASetVertexBuffers(0, 1, &vbView);
		cmd->IASetIndexBuffer(&ibView);
		cmd->SetGraphicsRootConstantBufferView(0, drawCB_[fi][slot]->GetGPUVirtualAddress());
		cmd->DrawIndexedInstanced(static_cast<UINT>(tmpIndices_.size()), 1, 0, 0, 0);
	}

	void TrailRibbonRenderer::ImGuiDebug() {
#ifdef USE_IMGUI
		ImGui::Begin("トレイル(リボン) 調整");

		ImGui::Checkbox("有効", &debug_.enable);

		ImGui::SeparatorText("太さ");
		ImGui::SliderFloat("先端の太さ", &debug_.headWidth, 0.01f, 5.0f, "%.3f");
		ImGui::SliderFloat("末端の太さ", &debug_.tailWidth, 0.01f, 5.0f, "%.3f");

		ImGui::SeparatorText("見た目");
		ImGui::SliderFloat("明るさ", &debug_.intensity, 0.0f, 20.0f, "%.3f");

		// ImGuiのColorEditはfloat[3]なので一旦配列に
		float c[3] = { debug_.color.x, debug_.color.y, debug_.color.z };
		if (ImGui::ColorEdit3("色", c)) {
			debug_.color = { c[0], c[1], c[2] };
		}

		ImGui::SeparatorText("UV");
		ImGui::SliderFloat("UVタイル", &debug_.uvTiling, 0.0f, 5.0f, "%.3f");
		ImGui::SliderFloat("UV流れ速度", &debug_.uvScroll, 0.0f, 10.0f, "%.3f");

		ImGui::End();
#endif
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
		// points[0] = tail（古い点）
		// points.back() = head（最新の点）
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
			if (tLen < 0.0001f) { t = { 0,0,1 }; } else { t = t / tLen; }

			// ===== 安定版 side 計算（紙みたいに消えるのを防ぐ）=====

			// カメラ方向（長さが距離で暴れないように正規化）
			Vector3 camVec = camPos - p;
			float camLen = MyMath::Length(camVec);
			if (camLen < 0.0001f) { camVec = { 0,0,1 }; } else { camVec = camVec / camLen; }

			// 基本： view方向 × tangent で “画面に幅が出る” side を作る
			Vector3 side = MyMath::Cross(camVec, t);
			float sideLen = MyMath::Length(side);

			// しきい値：小さすぎると「ほぼ平行」を拾って暴れる
			const float kEps = 0.01f;

			if (sideLen < kEps) {
				// ① prevSide を camVec に直交化してから使う（ここが本命）
				// side = prevSide - camVec * dot(prevSide, camVec)
				Vector3 s = prevSide - camVec * MyMath::Dot(prevSide, camVec);
				float sLen = MyMath::Length(s);

				if (sLen >= kEps) {
					side = s / sLen;
				} else {
					// ② それでもダメなら camVec から安定軸を作って side を再構築
					Vector3 worldUp = { 0, 1, 0 };
					Vector3 camRight = MyMath::Cross(worldUp, camVec);
					float rLen = MyMath::Length(camRight);

					if (rLen < kEps) {
						worldUp = { 1, 0, 0 }; // 真上向き対策
						camRight = MyMath::Cross(worldUp, camVec);
						rLen = MyMath::Length(camRight);
					}

					if (rLen < kEps) {
						// 最終保険：前回維持
						side = prevSide;
					} else {
						camRight = camRight / rLen;

						Vector3 s2 = MyMath::Cross(t, camRight);
						float s2Len = MyMath::Length(s2);

						if (s2Len < kEps) {
							side = prevSide;
						} else {
							side = s2 / s2Len;
						}
					}
				}
			} else {
				side = side / sideLen;
			}

			// 向き反転を抑える（連続性）
			if (MyMath::Dot(side, prevSide) < 0.0f) {
				side = side * -1.0f;
			}
			prevSide = side; // ←重要：fallbackでも必ず更新する

			// points[0] = tail（古い点）
			// points.back() = head（最新の点）
			// age01 は head=0, tail=1
			float a = (n <= 1) ? 1.0f : 1.0f - (float)i / (float)(n - 1);
			// a は tail=1 -> head=0
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
				outIndices.push_back(base + 1);
				outIndices.push_back(base + 2);

				outIndices.push_back(base + 1);
				outIndices.push_back(base + 3);
				outIndices.push_back(base + 2);
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

		// ルートシグネチャの説明を設定します。ここでは、1つのルートパラメータを持つルートシグネチャを定義しています。ルートパラメータは、定数バッファビュー（CBV）で、シェーダーレジスタ0にバインドされます。ShaderVisibilityは、すべてのシェーダーステージでこのルートパラメータが使用されることを示しています。
		D3D12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.NumParameters = 1;
		rsDesc.pParameters = rp;
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;
		HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob);

		// ルートシグネチャのシリアライズに失敗した場合、エラーメッセージをデバッグ出力に表示し、アサートで停止します。
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
		rt.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rt.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		rt.BlendOp = D3D12_BLEND_OP_ADD;
		rt.SrcBlendAlpha = D3D12_BLEND_ONE;
		rt.DestBlendAlpha = D3D12_BLEND_ONE;
		rt.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rt.LogicOp = D3D12_LOGIC_OP_NOOP;
		rt.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// PSO
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
		ds.DepthEnable = FALSE;
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