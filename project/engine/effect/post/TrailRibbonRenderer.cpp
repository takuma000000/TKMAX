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
		// デバッグ時は最適化を切り、デバッグ情報を含める
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		// コンパイル済みシェーダーとエラー情報の受け取り先
		Microsoft::WRL::ComPtr<ID3DBlob> shader;
		Microsoft::WRL::ComPtr<ID3DBlob> errors;

		// 指定ファイルからHLSLシェーダーをコンパイルする
		HRESULT hr = D3DCompileFromFile(
			path,
			nullptr,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entry,
			target,
			flags,
			0,
			&shader,
			&errors
		);

		// コンパイルに失敗した場合は、エラー内容を出力して停止する
		if (FAILED(hr)) {
			if (errors) {
				OutputDebugStringA((char*)errors->GetBufferPointer());
			}

			assert(false && "Shader compile failed.");
			return nullptr;
		}

		// コンパイル済みシェーダーを返す
		return shader;
	}

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer_(
		ID3D12Device* device,
		size_t sizeBytes
	) {
		// CPUから書き込めるアップロードヒープを使う
		D3D12_HEAP_PROPERTIES heap{};
		heap.Type = D3D12_HEAP_TYPE_UPLOAD;

		// バッファリソースとして作成する
		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Width = (UINT64)sizeBytes;
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

		Microsoft::WRL::ComPtr<ID3D12Resource> res;

		// 指定サイズのアップロードバッファを作成する
		HRESULT hr = device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&res)
		);

		assert(SUCCEEDED(hr));

		// 作成したリソースを返す
		return res;
	}

	void TrailRibbonRenderer::Initialize(DirectXCommon* dxCommon) {
		assert(dxCommon);

		// リボン描画用のRootSignatureとPSOを作成する
		CreatePipeline_(dxCommon);

		auto device = dxCommon->GetDevice();
		(void)device;

		// 今フレームの描画スロット数を初期化する
		drawCount_ = 0;

		// フレームリングごとの描画用バッファ管理配列を初期化する
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
		// シェーダーへ渡す時間を進める
		time_ += dt;

		// フレームごとにリングバッファを回す
		frameIndex_ = (frameIndex_ + 1) % kFrameRing_;

		// 今フレームの描画数をリセットする
		drawCount_ = 0;

		// ImGui調整項目を表示する
		ImGuiDebug();
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
		// DirectXCommonがない場合は描画できない
		if (!dxCommon) {
			return;
		}

		// 2点未満ではリボン形状を作れない
		if (points.size() < 2) {
			return;
		}

		//=============================================================
		// リボンメッシュ生成
		//=============================================================
		tmpVerts_.clear();
		tmpIndices_.clear();

		// 点列から、カメラに対して見えるリボンメッシュを作る
		BuildRibbonMesh_(camera, points, headWidth, tailWidth, color, uvTiling, tmpVerts_, tmpIndices_);

		// 頂点またはインデックスが作れなかった場合は描画しない
		if (tmpVerts_.empty() || tmpIndices_.empty()) {
			return;
		}

		auto device = dxCommon->GetDevice();

		// 今回必要な頂点数・インデックス数を取得する
		const uint32_t needVerts = static_cast<uint32_t>(tmpVerts_.size());
		const uint32_t needIndices = static_cast<uint32_t>(tmpIndices_.size());

		// 今フレームで使う描画スロット番号
		const uint32_t slot = drawCount_++;

		// 現在使うフレームリング番号
		const int fi = frameIndex_;

		//=============================================================
		// スロット配列拡張
		//=============================================================
		// 今回使うスロットが存在しない場合は、各管理配列を拡張する
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

		//=============================================================
		// VBを必要なら再作成
		//=============================================================
		if (!drawVB_[fi][slot] || drawVBCapacity_[fi][slot] < needVerts) {
			// 初回は必要数ぴったり、既存がある場合は倍々で余裕を持たせる
			uint32_t newCapacity = needVerts;
			if (drawVBCapacity_[fi][slot] > 0) {
				newCapacity = (std::max)(needVerts, drawVBCapacity_[fi][slot] * 2);
			}

			// 頂点用アップロードバッファを作成する
			drawVB_[fi][slot] = CreateUploadBuffer_(device, sizeof(Vertex) * static_cast<size_t>(newCapacity));

			// Mapし直して、CPUから書き込めるポインタを保持する
			drawVBMapped_[fi][slot] = nullptr;
			drawVB_[fi][slot]->Map(0, nullptr, reinterpret_cast<void**>(&drawVBMapped_[fi][slot]));

			// 現在の頂点バッファ容量を保存する
			drawVBCapacity_[fi][slot] = newCapacity;
		}

		//=============================================================
		// IBを必要なら再作成
		//=============================================================
		if (!drawIB_[fi][slot] || drawIBCapacity_[fi][slot] < needIndices) {
			// 初回は必要数ぴったり、既存がある場合は倍々で余裕を持たせる
			uint32_t newCapacity = needIndices;
			if (drawIBCapacity_[fi][slot] > 0) {
				newCapacity = (std::max)(needIndices, drawIBCapacity_[fi][slot] * 2);
			}

			// インデックス用アップロードバッファを作成する
			drawIB_[fi][slot] = CreateUploadBuffer_(device, sizeof(uint16_t) * static_cast<size_t>(newCapacity));

			// Mapし直して、CPUから書き込めるポインタを保持する
			drawIBMapped_[fi][slot] = nullptr;
			drawIB_[fi][slot]->Map(0, nullptr, reinterpret_cast<void**>(&drawIBMapped_[fi][slot]));

			// 現在のインデックスバッファ容量を保存する
			drawIBCapacity_[fi][slot] = newCapacity;
		}

		//=============================================================
		// CBは1スロット1個を使い回す
		//=============================================================
		if (!drawCB_[fi][slot]) {
			// 定数バッファサイズは256バイト境界に揃える
			UINT cbSize = static_cast<UINT>(sizeof(CB));
			cbSize = (cbSize + 255) & ~255u;

			// 定数バッファ用アップロードバッファを作成する
			drawCB_[fi][slot] = CreateUploadBuffer_(device, cbSize);

			// Mapして、CPUから書き込めるポインタを保持する
			drawCBMapped_[fi][slot] = nullptr;
			drawCB_[fi][slot]->Map(0, nullptr, reinterpret_cast<void**>(&drawCBMapped_[fi][slot]));
		}

		//=============================================================
		// データ書き込み
		//=============================================================
		// 生成した頂点とインデックスをGPU用バッファへコピーする
		memcpy(drawVBMapped_[fi][slot], tmpVerts_.data(), sizeof(Vertex) * tmpVerts_.size());
		memcpy(drawIBMapped_[fi][slot], tmpIndices_.data(), sizeof(uint16_t) * tmpIndices_.size());

		// 定数バッファへ描画用パラメータを書き込む
		CB* cbPtr = drawCBMapped_[fi][slot];
		cbPtr->viewProj = camera.GetViewProjectionMatrix();
		cbPtr->time = time_;
		cbPtr->uvScroll = uvScroll;
		cbPtr->intensity = intensity;
		cbPtr->pad0 = 0.0f;

		//=============================================================
		// 頂点バッファビュー・インデックスバッファビュー作成
		//=============================================================
		D3D12_VERTEX_BUFFER_VIEW vbView{};
		vbView.BufferLocation = drawVB_[fi][slot]->GetGPUVirtualAddress();
		vbView.StrideInBytes = sizeof(Vertex);
		vbView.SizeInBytes = static_cast<UINT>(sizeof(Vertex) * tmpVerts_.size());

		D3D12_INDEX_BUFFER_VIEW ibView{};
		ibView.BufferLocation = drawIB_[fi][slot]->GetGPUVirtualAddress();
		ibView.Format = DXGI_FORMAT_R16_UINT;
		ibView.SizeInBytes = static_cast<UINT>(sizeof(uint16_t) * tmpIndices_.size());

		//=============================================================
		// 描画
		//=============================================================
		auto cmd = dxCommon->GetCommandList();

		// リボン描画用パイプラインを設定する
		cmd->SetPipelineState(pso_.Get());
		cmd->SetGraphicsRootSignature(rootSig_.Get());

		// 三角形リストとして描画する
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 頂点・インデックスバッファを設定する
		cmd->IASetVertexBuffers(0, 1, &vbView);
		cmd->IASetIndexBuffer(&ibView);

		// 定数バッファをシェーダーへ渡す
		cmd->SetGraphicsRootConstantBufferView(0, drawCB_[fi][slot]->GetGPUVirtualAddress());

		// リボンメッシュを描画する
		cmd->DrawIndexedInstanced(static_cast<UINT>(tmpIndices_.size()), 1, 0, 0, 0);
	}

	void TrailRibbonRenderer::ImGuiDebug() {
#ifdef USE_IMGUI
		ImGui::Begin("トレイル(リボン) 調整");

		// リボン描画の有効・無効を切り替える
		ImGui::Checkbox("有効", &debug_.enable);

		ImGui::SeparatorText("太さ");

		// リボン先端・末端の太さを調整する
		ImGui::SliderFloat("先端の太さ", &debug_.headWidth, 0.01f, 5.0f, "%.3f");
		ImGui::SliderFloat("末端の太さ", &debug_.tailWidth, 0.01f, 5.0f, "%.3f");

		ImGui::SeparatorText("見た目");

		// 発光感・色味を調整する
		ImGui::SliderFloat("明るさ", &debug_.intensity, 0.0f, 20.0f, "%.3f");

		// ImGuiのColorEdit用にVector3をfloat配列へ移す
		float c[3] = { debug_.color.x, debug_.color.y, debug_.color.z };
		if (ImGui::ColorEdit3("色", c)) {
			debug_.color = { c[0], c[1], c[2] };
		}

		ImGui::SeparatorText("UV");

		// UVの繰り返し量と流れる速度を調整する
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

		//=============================================================
		// 累積距離計算
		//=============================================================
		// UVのU座標に使うため、点列に沿った距離を前から積み上げる
		std::vector<float> dist(n, 0.0f);

		for (uint32_t i = 1; i < n; ++i) {
			Vector3 d = points[i] - points[i - 1];
			float len = MyMath::Length(d);
			dist[i] = dist[i - 1] + len;
		}

		// リボンは各点につき左右2頂点、区間ごとに三角形2枚を作る
		outVerts.reserve(n * 2);
		outIndices.reserve((n - 1) * 6);

		// カメラ方向を使って、常に見える幅方向を作る
		Vector3 camPos = camera.GetTranslate();
		Vector3 prevSide = { 1,0,0 };

		for (uint32_t i = 0; i < n; ++i) {
			const Vector3& p = points[i];

			//=========================================================
			// 接線方向計算
			//=========================================================
			// 端点では自分自身を片側に使い、中央では前後から方向を取る
			Vector3 prev = (i == 0) ? points[i] : points[i - 1];
			Vector3 next = (i + 1 < n) ? points[i + 1] : points[i];

			Vector3 t = next - prev;
			float tLen = MyMath::Length(t);

			// 接線が作れない場合は仮の前方向を使う
			if (tLen < 0.0001f) {
				t = { 0,0,1 };
			} else {
				t = t / tLen;
			}

			//=========================================================
			// 安定版side計算
			//=========================================================
			// カメラ方向を正規化して、距離による幅方向のブレを防ぐ
			Vector3 camVec = camPos - p;
			float camLen = MyMath::Length(camVec);

			if (camLen < 0.0001f) {
				camVec = { 0,0,1 };
			} else {
				camVec = camVec / camLen;
			}

			// view方向 × tangent で、画面に幅が出るsideを作る
			Vector3 side = MyMath::Cross(camVec, t);
			float sideLen = MyMath::Length(side);

			// しきい値が小さすぎると、ほぼ平行な時にリボンが暴れる
			const float kEps = 0.01f;

			if (sideLen < kEps) {
				// prevSideをcamVecに直交化して、前回の幅方向をなるべく維持する
				Vector3 s = prevSide - camVec * MyMath::Dot(prevSide, camVec);
				float sLen = MyMath::Length(s);

				if (sLen >= kEps) {
					side = s / sLen;
				} else {
					// それでも無理な場合は、ワールド上方向からカメラ右方向を作る
					Vector3 worldUp = { 0, 1, 0 };
					Vector3 camRight = MyMath::Cross(worldUp, camVec);
					float rLen = MyMath::Length(camRight);

					// 真上・真下方向で右方向が作れない場合は、別軸を使う
					if (rLen < kEps) {
						worldUp = { 1, 0, 0 };
						camRight = MyMath::Cross(worldUp, camVec);
						rLen = MyMath::Length(camRight);
					}

					if (rLen < kEps) {
						// 最終保険として前回のsideを使う
						side = prevSide;
					} else {
						// カメラ右方向を正規化する
						camRight = camRight / rLen;

						// 接線とカメラ右方向から再度sideを作る
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
				// 通常時はそのまま正規化して使う
				side = side / sideLen;
			}

			// sideの向きが急に反転するとリボンがねじれるため、前回方向と揃える
			if (MyMath::Dot(side, prevSide) < 0.0f) {
				side = side * -1.0f;
			}

			// 次の点のfallback用に、今回のsideを保存する
			prevSide = side;

			//=========================================================
			// 幅計算
			//=========================================================
			// points[0] = tail（古い点）
			// points.back() = head（最新の点）
			// age01はhead=0, tail=1
			float a = (n <= 1) ? 1.0f : 1.0f - (float)i / (float)(n - 1);

			// 古い点ほどtailWidth、新しい点ほどheadWidthへ近づける
			float width = headWidth + (tailWidth - headWidth) * a;

			// 中心点からside方向へ左右に広げる
			Vector3 left = p - side * (width * 0.5f);
			Vector3 right = p + side * (width * 0.5f);

			// 距離に応じたUVのU座標を作る
			float u = dist[i] * uvTiling;

			// 左側頂点を作る
			Vertex vl{};
			vl.pos = left;
			vl.uv = { u, 0.0f };
			vl.color = { color.x, color.y, color.z, 1.0f };
			vl.age01 = a;

			// 右側頂点を作る
			Vertex vr{};
			vr.pos = right;
			vr.uv = { u, 1.0f };
			vr.color = { color.x, color.y, color.z, 1.0f };
			vr.age01 = a;

			// 今回追加する左右2頂点の開始番号
			uint16_t base = (uint16_t)outVerts.size();

			// 左右の頂点を追加する
			outVerts.push_back(vl);
			outVerts.push_back(vr);

			// 次の点がある場合、今回の2頂点と次の2頂点をつなぐインデックスを予約形式で追加する
			if (i + 1 < n) {
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

		//=============================================================
		// RootSignature作成
		//=============================================================
		// RootSignatureは定数バッファ1つだけの最小構成にする
		D3D12_ROOT_PARAMETER rp[1]{};
		rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rp[0].Descriptor.ShaderRegister = 0;
		rp[0].Descriptor.RegisterSpace = 0;
		rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		// 入力アセンブラを使うRootSignatureとして設定する
		D3D12_ROOT_SIGNATURE_DESC rsDesc{};
		rsDesc.NumParameters = 1;
		rsDesc.pParameters = rp;
		rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		Microsoft::WRL::ComPtr<ID3DBlob> sigBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;

		// RootSignatureをシリアライズする
		HRESULT hr = D3D12SerializeRootSignature(
			&rsDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&sigBlob,
			&errBlob
		);

		// シリアライズに失敗した場合は、エラー内容を出力して停止する
		if (FAILED(hr)) {
			if (errBlob) {
				OutputDebugStringA((char*)errBlob->GetBufferPointer());
			}
			assert(false);
		}

		// RootSignatureを作成する
		hr = device->CreateRootSignature(
			0,
			sigBlob->GetBufferPointer(),
			sigBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSig_)
		);
		assert(SUCCEEDED(hr));

		//=============================================================
		// シェーダー読み込み
		//=============================================================
		auto vs = CompileShader_(L"resources/shaders/TrailRibbon.VS.hlsl", "main", "vs_5_0");
		auto ps = CompileShader_(L"resources/shaders/TrailRibbon.PS.hlsl", "main", "ps_5_0");

		//=============================================================
		// InputLayout設定
		//=============================================================
		D3D12_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,      0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT,         0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		//=============================================================
		// Blend設定
		//=============================================================
		D3D12_BLEND_DESC blend{};
		blend.AlphaToCoverageEnable = FALSE;
		blend.IndependentBlendEnable = FALSE;

		// 半透明合成用の設定
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

		//=============================================================
		// PSO設定
		//=============================================================
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
		psoDesc.pRootSignature = rootSig_.Get();
		psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
		psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
		psoDesc.BlendState = blend;
		psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		//=============================================================
		// Rasterizer設定
		//=============================================================
		D3D12_RASTERIZER_DESC rast{};
		rast.FillMode = D3D12_FILL_MODE_SOLID;
		rast.CullMode = D3D12_CULL_MODE_NONE;
		rast.FrontCounterClockwise = FALSE;
		rast.DepthClipEnable = TRUE;
		psoDesc.RasterizerState = rast;

		//=============================================================
		// Depth設定
		//=============================================================
		D3D12_DEPTH_STENCIL_DESC ds{};
		ds.DepthEnable = FALSE;
		ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		ds.StencilEnable = FALSE;

		psoDesc.DepthStencilState = ds;
		psoDesc.InputLayout = { layout, _countof(layout) };
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// レンダーターゲットと深度フォーマットを設定する
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;

		// PSOを作成する
		hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso_));
		assert(SUCCEEDED(hr));
	}

} // namespace TKM