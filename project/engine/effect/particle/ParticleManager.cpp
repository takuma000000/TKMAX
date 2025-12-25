#define NOMINMAX
#include "ParticleManager.h"
#include "TextureManager.h"
#include "MyMath.h"
#include <numbers>
#include <algorithm>

namespace TKM {
	ParticleManager* ParticleManager::instance = nullptr;

	ParticleManager* ParticleManager::GetInstance() {
		if (instance == nullptr) {
			instance = new ParticleManager();
		}

		return instance;
	}

	void ParticleManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager, TKM::Camera* camera) {
		//引数で受け取る
		dxCommon_ = dxCommon;
		srvManager_ = srvManager;
		camera_ = camera;

		// --- 加速度フィールド初期化 ---
		acc.acc = { 0.0f,0.0f,0.0f };
		acc.area.min = { -1.0f,-1.00f,-1.0f };
		acc.area.max = { 1.0f,1.0f,1.0f };

		// --- 永続マテリアルCB作成 ---
		materialCB_ = dxCommon_->CreateBufferResource(sizeof(Material));

		// 一度だけマップして使い回す
		materialCB_->Map(0, nullptr, reinterpret_cast<void**>(&materialCPU_));
		assert(materialCPU_); // 念のためチェック

		//ランダムエンジンの初期化
		std::random_device seedGenerator;
		std::mt19937 randomEngine(seedGenerator());

		// --- パイプライン生成 ---
		CreatePipeline(); //パイプライン生成
		InitializeVD(); //頂点データ初期化
		CreateVR(); //頂点リソース生成
		CreateVB(); //頂点バッファビュー生成
		WriteResource(); //マテリアルリソース生成
	}

	void ParticleManager::Update(float dt) {
		MakeBillboardMatrix(); //ビルボードマトリクス作成

		for (std::unordered_map<std::string, ParticleGroup>::iterator particleGroupIterator = particleGroups.begin(); particleGroupIterator != particleGroups.end();) { //各パーティクルグループの更新
			//パーティクルグループのポインタを取得
			ParticleGroup* particleGroup = &(particleGroupIterator->second);
			particleGroupIterator->second.kNumInstance = 0;

			for (std::list<Particle>::iterator particleIterator = particleGroup->particles.begin(); particleIterator != particleGroup->particles.end();) { //各パーティクルの更新
				if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {//生存期間を過ぎていたら更新せず描画対象にしない
					particleIterator = particleGroup->particles.erase(particleIterator);
					continue;
				}
				//ワールド行列計算
				Matrix4x4 scaleMatrix = MyMath::MakeScaleMatrix((*particleIterator).transform.scale);
				Matrix4x4 translateMatrix = MyMath::MakeTranslateMatrix((*particleIterator).transform.translate);
				Matrix4x4 rotateMatrix = MyMath::MakeRotateMatrix((*particleIterator).transform.rotate);
				Matrix4x4 worldMatrix = scaleMatrix * rotateMatrix * billboardMatrix * translateMatrix;
				Matrix4x4 cameraMatrix = MyMath::MakeAffineMatrix(camera_->GetScale(), camera_->GetRotate(), camera_->GetTranslate());
				Matrix4x4 viewMatrix = camera_->GetViewMatrix();
				Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();
				Matrix4x4 worldViewProjectionMatrix =
					MyMath::Multiply(worldMatrix, MyMath::Multiply(viewMatrix, projectionMatrix));
				if (particleGroupIterator->second.kNumInstance < kNumMaxInstance) { //最大インスタンス数以下なら更新と描画対象にする
					//フィールドの範囲内のParticleには加速度を適用する
					if (IsCollision(acc.area, (*particleIterator).transform.translate)) { //当たり判定
						(*particleIterator).velocity += acc.acc * kDeltaTime;
					}
					(*particleIterator).transform.translate += (*particleIterator).velocity * kDeltaTime; //速度を元に位置を更新
					(*particleIterator).currentTime += kDeltaTime;//経過時間を足す
					//インスタンスデータ更新
					particleGroup->instancingData[particleGroupIterator->second.kNumInstance].wvp = worldViewProjectionMatrix;
					particleGroup->instancingData[particleGroupIterator->second.kNumInstance].World = worldMatrix;
					particleGroup->instancingData[particleGroupIterator->second.kNumInstance].color = (*particleIterator).color;
					float alpha = 1.0f - ((*particleIterator).currentTime / (*particleIterator).lifeTime); //アルファ値計算(0~1)
					particleGroup->instancingData[particleGroupIterator->second.kNumInstance].color.w = alpha;
					++particleGroupIterator->second.kNumInstance;//生きているParticleの数を1つカウントする
				}
				++particleIterator; //次のパーティクルへ
			}
			++particleGroupIterator; //次のパーティクルグループへ
		}
	}

	void ParticleManager::Draw() {
		auto* cmd = dxCommon_->GetCommandList(); // コマンドリスト取得

		// 共通セット
		cmd->SetGraphicsRootSignature(rootSignature.Get());
		cmd->SetPipelineState(graphicsPipelineState.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 頂点数取得
		const UINT vtxCountNormal = static_cast<UINT>(modelData.vertices.size());
		const UINT vtxCountRing = static_cast<UINT>(ringModelData.vertices.size());
		const UINT vtxCountCylinder = static_cast<UINT>(cylinderModelData.vertices.size());
		//const UINT vtxCountRibbon = static_cast<UINT>(ribbonModelData.vertices.size());

		for (auto it = particleGroups.begin(); it != particleGroups.end(); ++it) { //各パーティクルグループの描画
			ParticleGroup& group = it->second;

			// ① インスタンス0なら描かない
			if (group.kNumInstance == 0) {
				continue;
			}

			// ② モデル頂点数0も弾く（型ごと）
			if (group.type == ParticleType::NORMAL && vtxCountNormal == 0) continue;
			if (group.type == ParticleType::RING && vtxCountRing == 0) continue;
			if (group.type == ParticleType::CYLINDER && vtxCountCylinder == 0) continue;
			//if (group.type == ParticleType::RIBBON && vtxCountRibbon == 0) continue;

			// ③ 永続CBに値を書くだけ（Create/Releaseしない）
			//    ※ Initialize() で materialCB_ を UploadHeap で作って materialCPU_ を永続Map済み
			materialCPU_->color = Vector4(1, 1, 1, 1);
			materialCPU_->enableLighting = true;
			materialCPU_->uvTransform = MyMath::MakeIdentity4x4();

			// ④ ルートバインド
			cmd->SetGraphicsRootConstantBufferView(0, materialCB_->GetGPUVirtualAddress());
			cmd->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group.srvIndex));                     // 粒子個別のSRV（頂点/インスタンス用など）
			cmd->SetGraphicsRootDescriptorTable(2, srvManager_->GetGPUDescriptorHandle(group.materialData.textureIndex));    // テクスチャ

			// ⑤ VB切替 & DrawInstanced
			if (group.type == ParticleType::NORMAL) {
				cmd->IASetVertexBuffers(0, 1, &vertexBufferView);
				cmd->DrawInstanced(vtxCountNormal, group.kNumInstance, 0, 0);
			} else if (group.type == ParticleType::RING) {
				cmd->IASetVertexBuffers(0, 1, &ringVertexBufferView);
				cmd->DrawInstanced(vtxCountRing, group.kNumInstance, 0, 0);
			} else if (group.type == ParticleType::CYLINDER) {
				cmd->IASetVertexBuffers(0, 1, &cylinderVertexBufferView);
				cmd->DrawInstanced(vtxCountCylinder, group.kNumInstance, 0, 0);
			}
		}
	}

	void ParticleManager::CreatePipeline() {
		HRESULT hr;

		//呼び出し
		CreateRootSigunature();

		//InputLayoutの設定
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
		inputElementDescs[0].SemanticName = "POSITION";
		inputElementDescs[0].SemanticIndex = 0;
		inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[1].SemanticName = "TEXCOORD";
		inputElementDescs[1].SemanticIndex = 0;
		inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs[2].SemanticName = "NORMAL";
		inputElementDescs[2].SemanticIndex = 0;
		inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs;
		inputLayoutDesc.NumElements = _countof(inputElementDescs);

		//ブレンドステートの設定
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

		//ラスタライザーステートの設定
		D3D12_RASTERIZER_DESC resterizerDesc{};
		resterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		resterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

		//シェーダーの読み込み
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
		assert(vertexShaderBlob != nullptr);
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");
		assert(pixelShaderBlob != nullptr);

		//DepthStencilStateの設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		//Depthの機能を有効化する
		depthStencilDesc.DepthEnable = true;
		//書き込みします
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		//比較関数はLessEqual。つまり、近ければ描画される
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

		//グラフィックスパイプラインの設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
		graphicPipelineStateDesc.pRootSignature = rootSignature.Get();
		graphicPipelineStateDesc.InputLayout = inputLayoutDesc;
		graphicPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };
		graphicPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };
		graphicPipelineStateDesc.BlendState = blendDesc;
		graphicPipelineStateDesc.RasterizerState = resterizerDesc;
		graphicPipelineStateDesc.NumRenderTargets = 1;
		graphicPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		graphicPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		graphicPipelineStateDesc.SampleDesc.Count = 1;
		graphicPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		//DepthStencilの設定
		graphicPipelineStateDesc.DepthStencilState = depthStencilDesc;
		graphicPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		//グラフィックスパイプラインの生成
		hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
		assert(SUCCEEDED(hr));
	}

	void ParticleManager::CreateRootSigunature() {
		HRESULT hr;

		//RootSignature作成
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		//DescriptorRange作成。PixelShaderのTexture用
		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
		descriptorRange[0].BaseShaderRegister = 0;//0から始まる
		descriptorRange[0].NumDescriptors = 1;//数は1つ
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//Offsetを自動計算

		//DescriptorRange作成。VertexShaderのInstancing用
		D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
		descriptorRangeForInstancing[0].BaseShaderRegister = 0;  // シェーダーレジスタ t0 にバインド
		descriptorRangeForInstancing[0].NumDescriptors = 1;
		descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;  // SRV (Shader Resource View) として設定
		descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		//RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform
		D3D12_ROOT_PARAMETER rootParameters[4] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;	//レジスタ番号0とバインド

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;	//DescriptorTableを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	//VertexShaderで使う
		rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;	//Tableの中身の配列を指定
		rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);	//Tableで利用する数

		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableを使う
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;//Tableの中身の配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//Tableで利用する数

		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
		rootParameters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1を使う

		descriptionRootSignature.pParameters = rootParameters;	//ルートパラメータ配列へのポインタ
		descriptionRootSignature.NumParameters = _countof(rootParameters);	//配列の長さ

		//Samplerの設定
		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;//バイリニアフィルタ
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;//0～1の範囲外をリピート
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較しない
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;//ありったけのMipMapを使う
		staticSamplers[0].ShaderRegister = 0;//レジスタ番号0を使う
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		descriptionRootSignature.pStaticSamplers = staticSamplers;
		descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

		//シリアライズしてバイナリにする
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlog = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlog = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlog, &errorBlog);
		if (FAILED(hr)) { //エラーなら
			Logger::Log(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
			assert(false);
		}

		//バイナリを元に生成
		rootSignature = nullptr;
		hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlog->GetBufferPointer(), signatureBlog->GetBufferSize(), IID_PPV_ARGS(&rootSignature)); //生成
		assert(SUCCEEDED(hr));
	}

	void ParticleManager::InitializeVD() {
		//四角形の頂点データ
		modelData.vertices.push_back({ .position = {1.0f,1.0f,0.0f,1.0f},.texcoord = {0.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
		modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
		modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
		modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
		modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
		modelData.vertices.push_back({ .position = {-1.0f,-1.0f,0.0f,1.0f},.texcoord = {1.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
		modelData.material.textureFilePath = "./resources/circle.png"; //テクスチャパス

		CreateRingVertices(); //リング頂点データ作成
		ringModelData.material.textureFilePath = "./resources/gradationLine.png"; //テクスチャパス

		CreateCylinderVertices(); //シリンダー頂点データ作成
		cylinderModelData.material.textureFilePath = "./resources/gradationLine.png"; //テクスチャパス

		// リボン（細長い板） 
		/*CreateRibbonVertices();
		ribbonModelData.material.textureFilePath = "./resources/circle.png";*/
	}

	void ParticleManager::CreateVR() {
		//頂点リソースを作る
		vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
		//リングの頂点リソースを作る
		ringVertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * ringModelData.vertices.size());
		//cylinderの頂点リソースを作る
		cylinderVertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * cylinderModelData.vertices.size());
		// リボン
		//ribbonVertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * ribbonModelData.vertices.size());
	}

	void ParticleManager::CreateVB() {
		//頂点バッファビューを作成する
		vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
		vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
		vertexBufferView.StrideInBytes = sizeof(VertexData);

		//リングの頂点リソースを作成する
		ringVertexBufferView.BufferLocation = ringVertexResource->GetGPUVirtualAddress();
		ringVertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * ringModelData.vertices.size());
		ringVertexBufferView.StrideInBytes = sizeof(VertexData);

		//cylinderの頂点リソースを作成する
		cylinderVertexBufferView.BufferLocation = cylinderVertexResource->GetGPUVirtualAddress();
		cylinderVertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * cylinderModelData.vertices.size());
		cylinderVertexBufferView.StrideInBytes = sizeof(VertexData);

		// RIBBON
		//ribbonVertexBufferView.BufferLocation = ribbonVertexResource->GetGPUVirtualAddress();
		//ribbonVertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * ribbonModelData.vertices.size());
		//ribbonVertexBufferView.StrideInBytes = sizeof(VertexData);
	}

	void ParticleManager::WriteResource() {
		//頂点リソースにデータを書き込む
		VertexData* vertexData = nullptr;
		//書き込むためのアドレスを取得
		vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
		std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

		//リングの頂点リソースを作成する
		VertexData* ringVertexData = nullptr;
		//書き込むためのアドレスを取得
		ringVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&ringVertexData));
		std::memcpy(ringVertexData, ringModelData.vertices.data(), sizeof(VertexData) * ringModelData.vertices.size());

		//cylinderの頂点リソースを作成する
		VertexData* cylinderVertexData = nullptr;
		//書き込むためのアドレスを取得
		cylinderVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&cylinderVertexData));
		std::memcpy(cylinderVertexData, cylinderModelData.vertices.data(), sizeof(VertexData) * cylinderModelData.vertices.size());

		// RIBBON
		/*VertexData* ribbonVertexData = nullptr;
		ribbonVertexResource->Map(0, nullptr, reinterpret_cast<void**>(&ribbonVertexData));
		std::memcpy(ribbonVertexData, ribbonModelData.vertices.data(), sizeof(VertexData) * ribbonModelData.vertices.size());*/

	}

	void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& textureFilePath, ParticleType type) {
		// すでに存在するなら何もしない（安全な再呼び出し対応）
		if (particleGroups.find(name) != particleGroups.end()) {
			return;
		}

		// 新規作成
		ParticleGroup newGroup;
		newGroup.materialData.textureFilePath = textureFilePath;
		newGroup.type = type;

		// テクスチャ読み込み＆SRV取得
		TKM::TextureManager::GetInstance()->LoadTexture(textureFilePath);
		uint32_t srvIndex = TKM::TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
		newGroup.materialData.textureIndex = srvIndex;
		// インスタンシング用バッファ作成
		newGroup.kNumInstance = kNumMaxInstance;
		size_t bufferSize = sizeof(ParticleForGPU) * newGroup.kNumInstance;
		newGroup.instancingResource = dxCommon_->CreateBufferResource(bufferSize);
		newGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.instancingData));

		// インスタンシング用SRV作成
		uint32_t instanceSrvIndex = srvManager_->Allocate();
		srvManager_->CreateSRVforStructureBuffer(instanceSrvIndex, newGroup.instancingResource.Get(), newGroup.kNumInstance, sizeof(ParticleForGPU));
		newGroup.srvIndex = instanceSrvIndex;

		particleGroups[name] = newGroup; // 登録
	}

	void ParticleManager::MakeBillboardMatrix() {
		//カメラの向きに回転するビルボード行列を作成
		Matrix4x4 backToFrontMatrix = MyMath::MakeRotateYMatrix(std::numbers::pi_v<float>);
		//ビルボード行列 = カメラのワールド行列 × Z180度回転行列
		billboardMatrix = MyMath::Multiply(backToFrontMatrix, camera_->GetWorldMatrix());

		billboardMatrix.m[3][0] = 0.0f; //平行移動成分はいらない
		billboardMatrix.m[3][1] = 0.0f; //平行移動成分はいらない
		billboardMatrix.m[3][2] = 0.0f; //平行移動成分はいらない

	}

	void ParticleManager::Emit(const std::string name, Vector3& pos, uint32_t count) {
		assert(particleGroups.find(name) != particleGroups.end());
		ParticleGroup& group = particleGroups[name]; // パーティクルグループの参照を取得

		const size_t kHardCap = std::max<size_t>(group.kNumInstance, 200); // 下限200
		for (uint32_t i = 0; i < count; ++i) {
			// 超過してたら古い順に削除（重さ対策）
			while (group.particles.size() >= kHardCap) {
				group.particles.pop_front();
			}
			Particle newParticle = MakeNewParticle(randomEngine, name, pos);
			group.particles.push_back(newParticle);
		}
	}

	ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& rng, const std::string& groupName, const Vector3& center) {
		Particle p{}; // 新規パーティクル

		// 共通：発生位置を中心±オフセット
		std::uniform_real_distribution<float> offXY(-0.3f, 0.3f);
		std::uniform_real_distribution<float> offZ(-0.3f, 0.3f);
		Vector3 offset{ offXY(rng), offXY(rng) * 0.6f, offZ(rng) };
		p.transform.translate = center + offset;

		if (groupName == "irisOpen") { //── 開幕用：中心から“放出”する粒 ──
			// ── 開幕用：中心へ“吸い込む”柔らかい粒 ──
			// 方向＝中心へ向かう（= -offset の方向）
			Vector3 dir = MyMath::Normalize(-offset);
			std::uniform_real_distribution<float> spd(0.06f, 0.14f);
			float s = spd(rng);
			p.velocity = dir * s;

			// 小さめ＆短命、青白〜白
			std::uniform_real_distribution<float> scl(0.6f, 1.2f);
			float sc = scl(rng);
			p.transform.scale = { sc, sc, sc };

			float life = std::uniform_real_distribution<float>(0.35f, 0.65f)(rng);
			p.lifeTime = life; p.currentTime = 0.0f;

			float c = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);
			p.color = { 0.85f * c, 0.90f * c, 1.00f, 1.0f };
		} else if (groupName == "irisFire") { //── 開幕用：中心から“放出”する粒 ──
			// --- 花火演出（画面全体に放射） ---
			// 広い範囲にオフセット
			std::uniform_real_distribution<float> offXY(-20.0f, 20.0f);
			std::uniform_real_distribution<float> offZ(-20.0f, 20.0f);
			Vector3 offset = { offXY(rng), offXY(rng), offZ(rng) };

			// ランダム方向ベクトル（正規化）
			Vector3 dir = MyMath::Normalize(offset);

			// 強めの速度
			std::uniform_real_distribution<float> spd(0.5f, 2.5f);
			p.velocity = dir * spd(rng);

			// 大小ランダム
			std::uniform_real_distribution<float> scl(0.8f, 1.6f);
			float sc = scl(rng);
			p.transform.scale = { sc, sc, sc };

			// 寿命長め（広く散っても見えるように）
			std::uniform_real_distribution<float> life(0.8f, 1.5f);
			p.lifeTime = life(rng);
			p.currentTime = 0.0f;

			// 明るくランダムカラー（花火っぽく）
			float hue = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 0.9f + 0.1f * sin(hue * 6.283f);
			float g = 0.8f + 0.2f * cos(hue * 6.283f);
			float b = 1.0f - 0.3f * sin(hue * 3.142f);
			p.color = { r, g, b, 1.0f };
		} else if (groupName == "jetSmoke") {
			// ─────────────────────────────
			// Player 後ろのスピード感ジェット
			//  コア炎 + もくもく煙 + スピードスパーク
			// ─────────────────────────────

			// center は Player のケツあたり。共通オフセットは一旦無視して自前で決める
			float kind = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);

			// ランダムヘルパー
			auto frand = [&rng](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 角度ランダム（円周上オフセット用）
			float ang = frand(0.0f, 2.0f * std::numbers::pi_v<float>);
			float radCore = frand(0.0f, 0.25f);  // コア用の半径
			float radSmoke = frand(0.15f, 0.60f); // 煙用の半径

			if (kind < 0.25f) {
				// ============================
				// ① コア炎：細くて明るいジェット
				// ============================
				Vector3 local = {
					std::cos(ang) * radCore * 0.4f,   // X：あまり広げない
					frand(-0.10f, 0.15f),             // Y：ちょい上下
					-0.4f                              // Z：少しだけ機体の後ろ側へ
				};
				p.transform.translate = center + local;

				// ガッと後ろへ吹く
				p.velocity = {
					frand(-0.3f, 0.3f),
					frand(0.0f, 0.15f),
					frand(-80.0f, -60.0f)             // 強く −Z 方向へ
				};

				// 細長い炎コア
				float len = frand(0.6f, 1.0f);
				float thick = frand(0.18f, 0.30f);
				p.transform.scale = { thick, len, thick };

				// 寿命はかなり短い（キュッと消える）
				p.lifeTime = frand(0.18f, 0.35f);
				p.currentTime = 0.0f;

				// 青～白寄りの噴射炎
				Vector3 col3 = { 0.6f, 0.8f, 1.0f };
				float hot = frand(0.9f, 1.3f);
				col3 = col3 * hot;
				p.color = { col3.x, col3.y, col3.z, 1.0f };

			} else if (kind < 0.85f) {
				// ============================
				// ② メインのもくもく白煙
				// ============================
				Vector3 local = {
					std::cos(ang) * radSmoke,
					frand(-0.15f, 0.25f),
					frand(-0.8f, -0.3f)               // コアより少し後ろで発生
				};
				p.transform.translate = center + local;

				// コアより遅めに後ろへ流れる
				p.velocity = {
					frand(-0.25f, 0.25f),
					frand(0.03f, 0.20f),             // 少し上昇
					frand(-45.0f, -25.0f)
				};

				// 大きめの丸煙
				float sc = frand(0.9f, 2.0f);
				p.transform.scale = { sc, sc, sc };

				// 長めに残って尾を引く
				p.lifeTime = frand(1.2f, 2.4f);
				p.currentTime = 0.0f;

				// 白〜薄いグレー
				float t = frand(0.0f, 1.0f);
				Vector3 col3 = {
					0.82f + 0.05f * t,
					0.84f + 0.04f * t,
					0.86f + 0.02f * t
				};
				float bright = frand(0.8f, 1.0f);
				col3 = col3 * bright;
				p.color = { col3.x, col3.y, col3.z, 1.0f };

			} else {
				// ============================
				// ③ スピードスパーク：速さの“線”
				// ============================
				Vector3 local = {
					std::cos(ang) * radSmoke * 0.8f,
					frand(-0.10f, 0.10f),
					frand(-0.5f, -0.2f)
				};
				p.transform.translate = center + local;

				// 細くて速い粒
				p.velocity = {
					frand(-0.4f, 0.4f),
					frand(-0.05f, 0.10f),
					frand(-90.0f, -70.0f)
				};

				float len = frand(0.8f, 1.4f);
				float thin = frand(0.10f, 0.18f);
				p.transform.scale = { thin, len, thin };

				p.lifeTime = frand(0.20f, 0.45f);
				p.currentTime = 0.0f;

				// 白～薄いシアンで「スピード線」っぽく
				Vector3 col3 = { 0.8f, 0.9f, 1.0f };
				float bright = frand(0.9f, 1.4f);
				col3 = col3 * bright;
				p.color = { col3.x, col3.y, col3.z, 1.0f };
			}
		} else if (groupName == "trail_rb") {
			// RB：青いスパーク（クールで安定）
			std::uniform_real_distribution<float> velX(-0.03f, 0.03f);
			std::uniform_real_distribution<float> velY(-0.03f, 0.03f);
			std::uniform_real_distribution<float> velZ(-2.0f, -0.6f);
			p.velocity = { velX(rng), velY(rng), velZ(rng) };

			float sc = std::uniform_real_distribution<float>(0.10f, 0.22f)(rng);
			p.transform.scale = { sc, sc, sc };
			p.lifeTime = std::uniform_real_distribution<float>(0.20f, 0.35f)(rng);
			p.currentTime = 0.0f;

			// 青～水色
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = { 0.2f + 0.1f * t, 0.5f + 0.3f * t, 1.0f };
			p.color = { 0.1f, 0.3f, 1.0f, 1.0f };  // 鮮やかな青（R10%, G30%, B100%）
		} else if (groupName == "trail_lb") {
			// LB：黄〜金色の尾（エネルギー感）
			std::uniform_real_distribution<float> velX(-0.02f, 0.02f);
			std::uniform_real_distribution<float> velY(-0.02f, 0.02f);
			std::uniform_real_distribution<float> velZ(-2.2f, -0.8f);
			p.velocity = { velX(rng), velY(rng), velZ(rng) };

			float sc = std::uniform_real_distribution<float>(0.12f, 0.26f)(rng);
			p.transform.scale = { sc, sc, sc };
			p.lifeTime = std::uniform_real_distribution<float>(0.25f, 0.45f)(rng);
			p.currentTime = 0.0f;

			// 明るい黄～金色
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = { 1.0f, 0.8f + 0.2f * t, 0.1f + 0.2f * t };
			p.color = { 1.0f, 0.9f, 0.1f, 1.0f };  // ほぼ純黄色（R100%, G90%, B10%）
		} else if (groupName == "trail_rt") {
			// RT：赤い尾（情熱・攻撃的）
			std::uniform_real_distribution<float> velX(-0.015f, 0.015f);
			std::uniform_real_distribution<float> velY(-0.015f, 0.015f);
			std::uniform_real_distribution<float> velZ(-2.8f, -1.2f);
			p.velocity = { velX(rng), velY(rng), velZ(rng) };

			float sc = std::uniform_real_distribution<float>(0.20f, 0.40f)(rng);
			p.transform.scale = { sc, sc, sc };
			p.lifeTime = std::uniform_real_distribution<float>(0.35f, 0.60f)(rng);
			p.currentTime = 0.0f;

			// 純赤～オレンジ寄り
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = { 1.0f, 0.2f + 0.3f * t, 0.1f };
			p.color = { 1.0f, 0.05f, 0.05f, 1.0f };  // 強い赤（R100%, G5%, B5%）
		} else if (groupName == "trail_lt") {

			// =========================================
			// LT 必殺技：気弾（実サイズ版）
			// ・巨大コアを常時生成
			// ・オーラは補助
			// ・煙にならない
			// =========================================

			p.transform.translate = center;

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			float kind = frand(0.0f, 1.0f);

			// ============================
			// ① メインコア（最重要）
			// ============================
			if (kind < 0.50f) {

				// ほぼ静止（球として見せる）
				p.velocity = { 0.0f, 0.0f, 0.0f };

				// 大きな球
				float sc = frand(2.0f, 3.5f);
				p.transform.scale = { sc, sc, sc };

				p.lifeTime = frand(0.08f, 0.14f);
				p.currentTime = 0.0f;

				float c = frand(2.8f, 3.8f);
				p.color = {
					0.95f * c,
					0.98f * c,
					1.00f * c,
					1.0f
				};
				return p;
			}

			// ============================
			// ② 外側オーラ
			// ============================
			if (kind < 0.85f) {

				Vector3 off{
					frand(-0.5f, 0.5f),
					frand(-0.5f, 0.5f),
					frand(-0.5f, 0.5f)
				};
				p.transform.translate = center + off;

				Vector3 dir =
					(MyMath::Length(off) > 0.001f) ?
					MyMath::Normalize(off) :
					Vector3{ 0,1,0 };

				p.velocity = dir * frand(0.6f, 1.2f);

				float sc = frand(1.6f, 2.8f);
				p.transform.scale = { sc, sc, sc };

				p.lifeTime = frand(0.05f, 0.09f);
				p.currentTime = 0.0f;

				p.color = {
					0.25f,
					0.75f,
					1.10f,
					1.0f
				};
				return p;
			}

			// ============================
			// ③ 火花（補助）
			// ============================
			Vector3 off{
				frand(-0.6f, 0.6f),
				frand(-0.6f, 0.6f),
				frand(-0.6f, 0.6f)
			};
			p.transform.translate = center + off;

			Vector3 dir = MyMath::Normalize(off);
			p.velocity = dir * frand(2.0f, 3.0f);

			float sc = frand(0.2f, 0.4f);
			p.transform.scale = { sc, sc, sc };

			p.lifeTime = frand(0.03f, 0.06f);
			p.currentTime = 0.0f;

			float c = frand(2.5f, 4.0f);
			p.color = { c, c, c, 1.0f };

		} else if (groupName == "damageSpark") { //── 故障スパーク ──
			// 放射状に高速で飛ぶ、短命、明るくチカチカ
			std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
			Vector3 v = { dir(rng), dir(rng) * 0.6f, dir(rng) };
			Vector3 n = (MyMath::Length(v) > 0.001f) ? MyMath::Normalize(v) : Vector3{ 0,0,1 };
			float spd = std::uniform_real_distribution<float>(1.2f, 2.4f)(rng);
			p.velocity = n * spd;

			float sc = std::uniform_real_distribution<float>(0.08f, 0.18f)(rng);
			p.transform.scale = { sc, sc, sc };

			p.lifeTime = std::uniform_real_distribution<float>(0.18f, 0.35f)(rng);
			p.currentTime = 0.0f;

			// 強い黄～白（火花）
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 1.0f;
			float g = 0.85f + 0.15f * t;
			float b = 0.1f + 0.2f * (1.0f - t);
			p.color = { r, g, b, 1.0f };
		} else if (groupName == "crashFlame") {
			// 基本は上向き。横に少し拡散して“躍る”感じ
			std::uniform_real_distribution<float> velX(-0.06f, 0.06f);
			std::uniform_real_distribution<float> velY(1.20f, 2.40f); // ↑ ぐっと強く
			std::uniform_real_distribution<float> velZ(-0.06f, 0.06f);
			p.velocity = { velX(rng), velY(rng), velZ(rng) };

			// 粒は大きめ（炎舌が見えるサイズ）
			float sc = std::uniform_real_distribution<float>(0.28f, 0.55f)(rng);
			p.transform.scale = { sc, sc, sc };

			// ほんの少し長命（バースト直後の見栄えを持たせる）
			p.lifeTime = std::uniform_real_distribution<float>(0.35f, 0.60f)(rng);
			p.currentTime = 0.0f;

			// “灼熱コア”～“黄炎”に振る（加算でギラッと出る）
			// tが小さいほど赤寄りコア、tが大きいほど黄寄り
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float r = 1.0f;
			float g = 0.55f + 0.40f * (1.0f - t);  // 0.95..0.55
			float b = 0.05f + 0.20f * t;           // 0.05..0.25
			p.color = { r, g, b, 1.0f };
		} else if (groupName == "fallStreak") {
			Particle p{};

			// 線を少し長く
			p.transform.scale = { 0.10f, 2.6f, 1.0f };
			p.transform.rotate = { 0.0f, 0.0f, 0.0f };
			p.transform.translate = center;

			// ほぼ垂直にゆっくり落下（見やすさ重視）
			float vx = ((rand() % 40) - 20) / 800.0f;     // ±0.025
			float vz = ((rand() % 40) - 20) / 1200.0f;    // ±0.016
			float vy = -(1.2f + (rand() % 40) / 100.0f);  // -1.2 ～ -1.6
			p.velocity = { vx, vy, vz };

			// 深紅
			p.color = { 1.35f, 0.10f, 0.06f, 1.0f };

			// 画面下まで十分に保つ寿命
			p.lifeTime = 10.0f + (rand() % 80) / 100.0f; // 10.0 ～ 10.8秒
			p.currentTime = 0.0f;

			return p;
		} else if (groupName == "fallStreakUp") {
			Particle p{};

			// 下→上に向かうストリーク
			p.transform.scale = { 0.10f, 2.6f, 1.0f };
			p.transform.rotate = { 0.0f, 0.0f, 0.0f };
			p.transform.translate = center;

			// ゆっくり上昇（反対方向）
			float vx = ((rand() % 40) - 20) / 800.0f;
			float vz = ((rand() % 40) - 20) / 1200.0f;
			float vy = (1.2f + (rand() % 40) / 100.0f);  // +1.2 ～ +1.6
			p.velocity = { vx, vy, vz };

			// 色は上昇らしく少し淡く
			p.color = { 1.2f, 0.25f, 0.15f, 1.0f };

			// 寿命長め
			p.lifeTime = 10.0f + (rand() % 80) / 100.0f;
			p.currentTime = 0.0f;

			return p;
		} else if (groupName == "fw_launch") {
			// 上にまっすぐ伸びる光の線
			p.transform.scale = { 1.5f, 3.5f, 1.5f };
			p.velocity = { 0, 18.0f + (float)(rand() % 5), 0 };
			p.color = { 1.0f, 0.8f, 0.3f, 1.0f };
			p.lifeTime = 0.40f;
		} else if (groupName == "fw_flash") {
			// 爆発直後のまぶしい閃光
			p.transform.scale = { 5.0f, 5.0f, 5.0f };
			p.velocity = { 0, 0, 0 };
			p.color = { 1, 1, 1, 1 };
			p.lifeTime = 0.2f;
		} else if (groupName == "fw_burst") {
			// 花火本体（放射状）
			float a1 = (float)rand() / RAND_MAX * 6.28f;
			float a2 = (float)rand() / RAND_MAX * 3.14f;

			Vector3 dir;
			dir.x = std::cos(a1) * std::sin(a2);
			dir.y = std::cos(a2) * 0.8f; // 上に散りすぎ防止
			dir.z = std::sin(a1) * std::sin(a2);

			float spd = 10.0f + ((float)rand() / RAND_MAX * 12.0f);
			p.velocity = dir * spd;

			float sc = 1.5f + ((float)rand() / RAND_MAX * 1.2f);
			p.transform.scale = { sc, sc, sc };

			// カラフル！（鮮やか〜中間）
			float r = 0.4f + ((float)rand() / RAND_MAX * 0.6f);
			float g = 0.4f + ((float)rand() / RAND_MAX * 0.6f);
			float b = 0.4f + ((float)rand() / RAND_MAX * 0.6f);

			p.color = { r, g, b, 1.0f };
			p.lifeTime = 3.0f;
		} else if (groupName == "airStreak") {
			// ─────────────────────
			// 空で飛んでるときの「風の筋」
			// ─────────────────────

			// 細長いライン（ビルボードでカメラ向きになる）
			p.transform.scale = { 0.09f, 0.09f, 0.09f }; // 幅, 高さ, 奥行き
			p.transform.rotate = { 0.0f, 0.0f, 0.0f };
			p.transform.translate.z = 50.0f;

			// ちょっとだけブレを入れながら手前(-Z)に流す
			float vx = ((rand() % 40) - 20) / 200.0f; // -0.1 ～ +0.1
			float vy = ((rand() % 40) - 20) / 200.0f; // -0.1 ～ +0.1

			float baseSpeed = 30.0f + (rand() % 40) / 10.0f; // 12.0 ～ 16.0 くらい
			float vz = -baseSpeed; // カメラ手前方向（-Z）へシュッと流れる

			p.velocity = { vx, vy, vz };

			// ほぼ白～薄い青でうっすら
			float c = 0.85f + (rand() % 15) / 100.0f; // 0.85 ～ 1.0
			// 濃い砂埃の色
			p.color = { 0.9f * c, 0.9f * c, 1.0f * c, 0.6f }; // 少し透明感あり

			p.lifeTime = 6.0f;   // だいたい3秒くらい生きる
			p.currentTime = 0.0f; // 初期化

		} else if (groupName == "ribbonTest") {

			// リボンは横長の板を想定
			//   X方向に長く、Y方向は少しだけ
			p.transform.scale = { 8.0f, 1.0f, 1.0f };

			// 少しだけ上にフワっと浮く
			p.velocity = { 0.0f, 3.0f, 0.0f };

			// 色（薄い紫っぽく）
			p.color = { 0.8f, 0.6f, 1.0f, 1.0f };

			// 1秒くらい残る
			p.lifeTime = 1.0f;
			p.currentTime = 0.0f;

		} else if (groupName == "enemySpawn") {
			//=========================================================
			// 敵出現用：中心で「ボフッ」と光って、周りに粒が広がる
			//   ・25%くらいは中心のフラッシュ
			//   ・残りは円状に飛び散る粒
			//=========================================================
			std::uniform_real_distribution<float> patternDist(0.0f, 1.0f);
			float pattern = patternDist(rng);

			if (pattern < 0.25f) {
				// ── 中央フラッシュ ──
				// 敵のど真ん中で大きく光るだけ（ほぼ動かない）
				p.transform.translate = center;
				p.velocity = { 0.0f, 0.0f, 0.0f };

				// 大きめサイズで「出現した！」感
				float sc = std::uniform_real_distribution<float>(1.8f, 2.6f)(rng);
				p.transform.scale = { sc, sc, sc };

				// 短命だけど強く光る
				p.lifeTime = std::uniform_real_distribution<float>(0.25f, 0.40f)(rng);
				p.currentTime = 0.0f;

				// 白に近いシアン系（コアがピカッと光るイメージ）
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				float rCol = 0.3f * (1.0f - t);
				float gCol = 0.9f;
				float bCol = 1.2f - 0.2f * t;
				p.color = { rCol, gCol, bCol, 1.0f };
			} else {
				// ── 周囲に円状に広がる粒 ──
				// ちょっと広めの半径＆Zにもバラつきを持たせる
				std::uniform_real_distribution<float> radiusDist(0.5f, 3.0f);
				std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * std::numbers::pi_v<float>);
				std::uniform_real_distribution<float> zOffsetDist(-0.8f, 0.8f);

				float r = radiusDist(rng);
				float th = angleDist(rng);

				Vector3 offsetLocal{
					std::cos(th) * r,
					std::sin(th) * 0.7f,   // Yを少し強めて“湧き上がる”感じ
					zOffsetDist(rng)       // 手前/奥にも少し散らす
				};

				p.transform.translate = center + offsetLocal;

				// オフセット方向に外へ飛ばす
				Vector3 dir = (MyMath::Length(offsetLocal) > 0.001f)
					? MyMath::Normalize(offsetLocal)
					: Vector3{ 0.0f, 1.0f, 0.0f };

				std::uniform_real_distribution<float> spd(1.2f, 3.2f);
				p.velocity = dir * spd(rng);

				// 粒自体も少し大きめ
				float sc = std::uniform_real_distribution<float>(0.6f, 1.3f)(rng);
				p.transform.scale = { sc, sc, sc };

				// ちょい長めに残す
				p.lifeTime = std::uniform_real_distribution<float>(0.60f, 1.00f)(rng);
				p.currentTime = 0.0f;

				// 青〜シアン系で、中心フラッシュより少し落ち着いた色
				float t2 = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				float rCol = 0.05f + 0.10f * (1.0f - t2);
				float gCol = 0.80f + 0.15f * t2;
				float bCol = 1.00f;
				p.color = { rCol, gCol, bCol, 1.0f };
			}

		} else if (groupName == "enemyHit_flash") {
			// 中央のまぶしいフラッシュ（一瞬だけ）＋色は毎回ちょっと変える
			p.transform.translate = center; // 完全センター固定

			// 少し大きめにして「ドンッ」と光る感じ
			float sc = std::uniform_real_distribution<float>(1.3f, 1.8f)(rng);
			p.transform.scale = { sc, sc, sc };
			p.velocity = { 0.0f, 0.0f, 0.0f };

			p.lifeTime = std::uniform_real_distribution<float>(0.07f, 0.10f)(rng);
			p.currentTime = 0.0f;

			// --- 白ベース＋アクセントカラーをランダム ---
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 coreColor;
			if (t < 0.33f) {
				// ゴールド寄り
				coreColor = { 1.0f, 0.95f, 0.70f };
			} else if (t < 0.66f) {
				// シアン寄り
				coreColor = { 0.75f, 0.95f, 1.0f };
			} else {
				// マゼンタ寄り
				coreColor = { 1.0f, 0.75f, 1.0f };
			}
			p.color = { coreColor.x, coreColor.y, coreColor.z, 1.0f };

		} else if (groupName == "enemyHit_ring") {
			// 外側に広がるショックウェーブリング（カラフル）
			p.transform.translate = center; // ぴったり中心

			// ちょっと大きめ＆強め
			float sc = std::uniform_real_distribution<float>(1.6f, 2.3f)(rng);
			p.transform.scale = { sc, sc, sc };
			p.velocity = { 0.0f, 0.0f, 0.0f };

			p.lifeTime = std::uniform_real_distribution<float>(0.22f, 0.30f)(rng);
			p.currentTime = 0.0f;

			// ビビッドな色を何パターンかからランダム
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 ringColor;
			if (t < 0.25f) {
				// ゴールド
				ringColor = { 1.0f, 0.9f, 0.4f };
			} else if (t < 0.5f) {
				// シアン
				ringColor = { 0.4f, 0.95f, 1.0f };
			} else if (t < 0.75f) {
				// ピンク
				ringColor = { 1.0f, 0.55f, 0.8f };
			} else {
				// ライム
				ringColor = { 0.6f, 1.0f, 0.6f };
			}
			p.color = { ringColor.x, ringColor.y, ringColor.z, 1.0f };

		} else if (groupName == "enemyHit_rays") {
			// 放射状の細長いレイ（光の筋）
			p.transform.translate = center;

			// 画面上の回転角
			float angle = std::uniform_real_distribution<float>(0.0f, 2.0f * std::numbers::pi_v<float>)(rng);
			p.transform.rotate = { 0.0f, 0.0f, angle };

			float len = std::uniform_real_distribution<float>(0.4f, 0.8f)(rng);
			float thin = std::uniform_real_distribution<float>(0.1f, 0.18f)(rng);
			p.transform.scale = { thin, len, 1.0f }; // ← XとYを逆転させる

			// 少しだけ外側に膨らむように動かす
			Vector3 dir = { std::cos(angle), 0.0f, std::sin(angle) };
			dir = (MyMath::Length(dir) > 0.001f) ? MyMath::Normalize(dir) : Vector3{ 1,0,0 };
			float spd = std::uniform_real_distribution<float>(3.0f, 6.0f)(rng);
			p.velocity = dir * spd;

			p.lifeTime = std::uniform_real_distribution<float>(0.18f, 0.26f)(rng);
			p.currentTime = 0.0f;

			// レイも派手色で
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 rayColor;
			if (t < 0.25f) {
				rayColor = { 1.0f, 0.85f, 0.45f };   // ゴールド
			} else if (t < 0.5f) {
				rayColor = { 0.5f, 0.95f, 1.0f };    // シアン
			} else if (t < 0.75f) {
				rayColor = { 1.0f, 0.6f, 0.9f };     // ピンク
			} else {
				rayColor = { 0.7f, 1.0f, 0.6f };     // ライム
			}
			p.color = { rayColor.x, rayColor.y, rayColor.z, 1.0f };

		} else if (groupName == "enemyHit_spark") {
			// 周りに飛び散る小さな火花（スピード＆色増し）
			p.transform.translate = center;

			// ランダム方向（XZメイン、少しだけY）
			float a = std::uniform_real_distribution<float>(0.0f, 2.0f * std::numbers::pi_v<float>)(rng);
			float up = std::uniform_real_distribution<float>(-0.25f, 0.55f)(rng);
			Vector3 dir = MyMath::Normalize(Vector3{ std::cos(a), up, std::sin(a) });

			// スピード強め
			float spd = std::uniform_real_distribution<float>(8.0f, 16.0f)(rng);
			p.velocity = dir * spd;

			float sc = std::uniform_real_distribution<float>(0.22f, 0.40f)(rng);
			p.transform.scale = { sc, sc, sc };

			p.lifeTime = std::uniform_real_distribution<float>(0.32f, 0.52f)(rng);
			p.currentTime = 0.0f;

			// 暖色・寒色・マゼンタをミックス
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 spColor;
			if (t < 0.33f) {
				// オレンジ
				spColor = { 1.0f, 0.65f, 0.25f };
			} else if (t < 0.66f) {
				// シアン
				spColor = { 0.4f, 0.9f, 1.0f };
			} else {
				// マゼンタ
				spColor = { 1.0f, 0.45f, 0.9f };
			}
			p.color = { spColor.x, spColor.y, spColor.z, 1.0f };
		} else if (groupName == "lt_nova_core") {
			// 爆心コア：画面を埋めるくらいのまぶしいエネルギー球
			p.transform.translate = center;

			// サイズ大幅アップ（敵が完全に飲み込まれるレベル）
			float sc = std::uniform_real_distribution<float>(4.0f, 6.0f)(rng);
			p.transform.scale = { sc, sc, sc };
			p.velocity = { 0.0f, 0.0f, 0.0f };

			// 残光長め（ドーンと光が残る）
			p.lifeTime = std::uniform_real_distribution<float>(0.45f, 0.65f)(rng);
			p.currentTime = 0.0f;

			// 中心は白＋黄金（太陽みたいな爆心）
			p.color = { 1.0f, 0.96f, 0.70f, 1.0f };


		} else if (groupName == "lt_nova_wave") {
			// 球状ショックウェーブ（外側のエネルギー殻。コアよりさらに大きい）
			p.transform.translate = center;

			// 半径かなり拡大（画面を貫く衝撃波）
			float sc = std::uniform_real_distribution<float>(5.0f, 7.5f)(rng);
			p.transform.scale = { sc, sc, sc };
			p.velocity = { 0.0f, 0.0f, 0.0f };

			// コアより少し長く残して「爆風の壁」感
			p.lifeTime = std::uniform_real_distribution<float>(0.50f, 0.80f)(rng);
			p.currentTime = 0.0f;

			// 内側が黄〜外側オレンジに見えるような暖色
			p.color = { 1.0f, 0.78f, 0.32f, 1.0f };

		} else if (groupName == "lt_nova_burst") {
			// シンプルな光の爆発粒子
			p.transform.translate = center;

			// 飛び散り方向ランダム
			Vector3 dir = {
				std::uniform_real_distribution<float>(-1,1)(rng),
				std::uniform_real_distribution<float>(-1,1)(rng),
				std::uniform_real_distribution<float>(-1,1)(rng)
			};
			if (MyMath::Length(dir) < 0.001f) dir = { 0,1,0 };
			dir = MyMath::Normalize(dir);

			// スピード（ランダム）
			float spd = std::uniform_real_distribution<float>(0.3f, 1.2f)(rng);
			p.velocity = dir * spd;

			// 大きさ（小さめの点）
			float sc = std::uniform_real_distribution<float>(0.2f, 0.6f)(rng);
			p.transform.scale = { sc, sc, sc };

			// 色（白ベース）
			p.color = { 1,1,1,1 };

			p.lifeTime = std::uniform_real_distribution<float>(0.20f, 0.40f)(rng);
		} else if (groupName == "lt_nova_debris") {
			// 破片：暗い塊が高速で四方八方に飛ぶ
			p.transform.translate = center;

			// バラバラの方向に飛ばす
			float ang1 = std::uniform_real_distribution<float>(0, 2 * std::numbers::pi_v<float>)(rng);
			float ang2 = std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng);

			Vector3 dir = {
				std::cos(ang1),
				ang2,
				std::sin(ang1)
			};
			dir = MyMath::Normalize(dir);

			float speed = std::uniform_real_distribution<float>(4.0f, 9.0f)(rng);
			p.velocity = dir * speed;

			// 小さめの塊＋ランダム
			float sc = std::uniform_real_distribution<float>(0.2f, 0.45f)(rng);
			p.transform.scale = { sc, sc, sc };

			// 暗い破片 → 茶色〜黒
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			float c = MyMath::Lerp(0.05f, 0.20f, t);
			p.color = { c, c * 0.9f, c * 0.8f, 1.0f };

			p.lifeTime = std::uniform_real_distribution<float>(0.4f, 0.8f)(rng);
			p.currentTime = 0.0f;
		} else if (groupName == "lt_nova_crack") {
			// 亀裂：空間を裂くような細長いスパーク
			p.transform.translate = center;

			// ランダム方向へ細く長いひび
			float ang = std::uniform_real_distribution<float>(0, 2 * std::numbers::pi_v<float>)(rng);
			Vector3 dir = { std::cos(ang), 0.0f, std::sin(ang) };

			float len = std::uniform_real_distribution<float>(1.5f, 3.0f)(rng);
			float thin = std::uniform_real_distribution<float>(0.05f, 0.12f)(rng);
			p.transform.scale = { len, thin, 1.0f };

			// ほぼ動かないが少しだけ散る
			p.velocity = dir * std::uniform_real_distribution<float>(0.5f, 1.5f)(rng);

			// 黒〜赤黒いひび
			float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 col = {
				MyMath::Lerp(0.05f, 0.3f, t),
				MyMath::Lerp(0.0f, 0.05f, t),
				MyMath::Lerp(0.0f, 0.05f, t)
			};
			p.color = { col.x, col.y, col.z, 1.0f };

			p.lifeTime = std::uniform_real_distribution<float>(0.35f, 0.5f)(rng);
			p.currentTime = 0.0f;
		} else if (groupName == "enemyDeath_core") {
			// 敵が消える瞬間、中心にフッと出る小さな光

			// 位置は完全にセンター
			p.transform.translate = center;

			// 少しだけ大きめだけど、そこまでド派手じゃない
			float sc = std::uniform_real_distribution<float>(0.9f, 1.4f)(rng);
			p.transform.scale = { sc, sc, sc };

			// 動かない（その場で光って消える）
			p.velocity = { 0.0f, 0.0f, 0.0f };

			// 寿命はかなり短いパッと光る感じ
			p.lifeTime = std::uniform_real_distribution<float>(0.12f, 0.20f)(rng);
			p.currentTime = 0.0f;

			// 少し黄味がかった白い光
			p.color = { 1.0f, 0.96f, 0.86f, 1.0f };

		} else if (groupName == "enemyDeath_shard") {
			// バラバラに飛び散る光の破片

			// 中心からごく小さなオフセット
			std::uniform_real_distribution<float> offSmall(-0.15f, 0.15f);
			Vector3 localOffset{
				offSmall(rng),
				offSmall(rng),
				offSmall(rng)
			};
			p.transform.translate = center + localOffset;

			// ランダム方向（やや上＋後ろに飛ぶ、ふわっと散るイメージ）
			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			Vector3 dir{
				frand(-0.6f, 0.6f),
				frand(0.0f, 0.9f),    // 上方向に少しバイアス
				frand(-1.0f, 0.2f)    // 画面奥〜少し手前
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float spd = frand(0.6f, 1.6f);
			p.velocity = dir * spd;

			// 小さい光の破片
			float sc = frand(0.25f, 0.55f);
			p.transform.scale = { sc, sc, sc };

			// ちょっとだけ残る
			p.lifeTime = frand(0.45f, 0.85f);
			p.currentTime = 0.0f;

			// 色は少しだけカラフル（青〜シアン〜マゼンタの中間）
			float t = frand(0.0f, 1.0f);
			Vector3 col = {
				0.6f + 0.3f * t,      // R : 0.6〜0.9
				0.7f + 0.2f * (1 - t),// G : 0.7〜0.9
				1.0f                  // B : 1.0（青白い感じ）
			};
			p.color = { col.x, col.y, col.z, 1.0f };

		} else if (groupName == "enemyDeath_smoke") {
			// ふわっと残る煙（あまり主張しない）

			// 位置はほぼセンター
			p.transform.translate = center;

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 少しだけゆっくり上昇
			p.velocity = {
				frand(-0.15f, 0.15f),
				frand(0.15f, 0.35f),
				frand(-0.15f, 0.15f)
			};

			// 丸くて少し大きめ
			float sc = frand(0.9f, 1.8f);
			p.transform.scale = { sc, sc, sc };

			// わりと長めに残って、消えたあとも余韻がある
			p.lifeTime = frand(0.9f, 1.5f);
			p.currentTime = 0.0f;

			// 薄いグレー〜少し青み
			Vector3 col3 = {
				frand(0.70f, 0.85f),
				frand(0.72f, 0.88f),
				frand(0.80f, 0.95f)
			};
			p.color = { col3.x, col3.y, col3.z, 1.0f };
		} else if (groupName == "enemyPounceTrail") {
			// ─────────────────────────────
			// 敵の飛び掛かり軌道：コアレール（闇マゼンタ一本軸）
			// ─────────────────────────────
			p.transform.translate = center;

			float height = std::uniform_real_distribution<float>(1.6f, 2.4f)(rng); // 縦の長さ
			float width = std::uniform_real_distribution<float>(0.08f, 0.14f)(rng); // 横の太さ
			// スプライトの縦横を入れ替える
			p.transform.scale = { width, height, 1.0f };

			p.velocity = { 0.0f, 0.0f, 0.0f };
			p.lifeTime = std::uniform_real_distribution<float>(0.55f, 0.9f)(rng);
			p.currentTime = 0.0f;

			// ── ベースカラー：闇マゼンタ ──
			// あえて 1色＋ほんの少しだけバリエーションに絞る
			Vector3 base = { 0.70f, 0.05f, 0.85f }; // 赤強めの紫

			// 彩度・明るさを微妙に揺らす（全部ランダムにしない）
			float valueJitter = std::uniform_real_distribution<float>(0.75f, 0.95f)(rng);
			float hueJitter = std::uniform_real_distribution<float>(-0.05f, 0.05f)(rng);

			Vector3 col = {
				std::clamp(base.x + hueJitter, 0.0f, 1.0f),
				base.y,
				std::clamp(base.z - hueJitter, 0.0f, 1.0f)
			};

			// 全体を少し暗くして「白・黄」に寄らないように
			col.x *= valueJitter;
			col.y *= valueJitter;
			col.z *= valueJitter;

			float alpha = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);

			p.color = { col.x, col.y, col.z, alpha };
		} else if (groupName == "enemyPounceSpark") {
			// ─────────────────────────────
			// 軌道上から飛び散るスパーク（血・毒・火花）
			// ─────────────────────────────
			p.transform.translate = center;

			Vector3 dir = {
				std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng),
				std::uniform_real_distribution<float>(-0.3f, 0.9f)(rng),
				std::uniform_real_distribution<float>(-1.0f, 1.0f)(rng)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float spd = std::uniform_real_distribution<float>(2.0f, 4.0f)(rng);
			p.velocity = dir * spd;

			float baseScale = std::uniform_real_distribution<float>(0.20f, 0.35f)(rng);

			// 0.0–0.5  血スパーク（赤系）
			// 0.5–0.8  毒スパーク（緑系）※Gは高いけどRをかなり抑える
			// 0.8–1.0  火花スパーク（赤橙）※黄色に寄りすぎない
			float   r = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
			Vector3 spColor;
			float   alpha = 1.0f;

			if (r < 0.5f) {
				// 血
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				if (t < 0.5f) {
					spColor = { 0.95f, 0.15f, 0.25f }; // 鮮血
				} else {
					spColor = { 0.75f, 0.05f, 0.15f }; // どす黒い血
				}
				alpha = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);
				baseScale *= std::uniform_real_distribution<float>(1.0f, 1.3f)(rng);

			} else if (r < 0.8f) {
				// 毒
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				if (t < 0.5f) {
					spColor = { 0.10f, 0.80f, 0.25f }; // 毒緑
				} else {
					spColor = { 0.05f, 0.60f, 0.20f }; // 少し暗い毒緑
				}
				alpha = std::uniform_real_distribution<float>(0.8f, 0.95f)(rng);
				baseScale *= std::uniform_real_distribution<float>(0.9f, 1.1f)(rng);

			} else {
				// 火花（赤橙）※黄色まで行かない
				float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
				if (t < 0.5f) {
					spColor = { 0.95f, 0.35f, 0.15f }; // 赤寄りオレンジ
				} else {
					spColor = { 0.85f, 0.25f, 0.15f }; // ちょい暗め
				}
				alpha = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);
				baseScale *= std::uniform_real_distribution<float>(0.8f, 1.0f)(rng);
			}

			// ここでも「白・黄に寄せない」ために少し暗くする
			float darken = 0.85f;
			spColor.x *= darken;
			spColor.y *= darken;
			spColor.z *= darken;

			p.transform.scale = { baseScale, baseScale, baseScale };
			p.color = { spColor.x, spColor.y, spColor.z, alpha };

			p.lifeTime = std::uniform_real_distribution<float>(0.20f, 0.40f)(rng);
			p.currentTime = 0.0f;
		} else if (groupName == "core_charge_shell") {
			p.transform.translate = center;
			float startScale = 1.6f;
			p.transform.scale = { startScale, startScale, startScale };
			p.velocity = { 0.0f, 0.0f, 0.0f };
			// 邪悪な黒紫
			p.color = { 0.35f, 0.0f, 0.5f, 0.35f };
			p.lifeTime = 0.45f;
			p.currentTime = 0.0f;
		} else if (groupName == "core_charge_inward") {

			std::uniform_real_distribution<float> dirDist(-1.0f, 1.0f);
			Vector3 dirRand = { dirDist(rng), dirDist(rng), dirDist(rng) };

			if (MyMath::Length(dirRand) < 0.001f) dirRand = { 0,1,0 };
			dirRand = MyMath::Normalize(dirRand);

			float radius = std::uniform_real_distribution<float>(2.0f, 5.0f)(rng);
			Vector3 offset = dirRand * radius;

			p.transform.translate = center + offset;

			Vector3 dirToCenter = MyMath::Normalize(-offset);

			float speed = std::uniform_real_distribution<float>(2.0f, 5.0f)(rng);
			p.velocity = dirToCenter * speed;

			p.transform.scale = { 0.15f, 0.15f, 0.15f };

			// 怪しい血のような禍々しく濃い赤
			p.color = { 0.8f, 0.05f, 0.1f, 0.9f };

			p.lifeTime = 0.7f;
			p.currentTime = 0.0f;
		} else if (groupName == "core_charge_ribbon") {

			float height = std::uniform_real_distribution<float>(-1.2f, 1.2f)(rng);
			p.transform.translate.y += height;

			// もっと長く・太くして視認性アップ
			p.transform.scale = { 3.2f, 0.14f, 1.0f };

			// 明るめの邪悪紫に変更（青成分足すと光って見える）
			p.color = { 0.85f, 0.2f, 1.0f, 0.95f };

			p.velocity = { 0.0f, 0.0f, 0.0f };

			// 寿命もちょい伸ばすと“渦巻き”感が出る
			p.lifeTime = 0.55f;
		} else if (groupName == "core_charge_flash") {

			float s = std::uniform_real_distribution<float>(0.25f, 0.55f)(rng);

			// 完全に正面に出るように Z も s にする
			p.transform.scale = { s, s, s };

			// 発光強めの深紅（alpha も上げる）
			p.color = { 1.0f, 0.05f, 0.15f, 1.0f };

			// 寿命をほんの少しだけ伸ばすと見える
			p.lifeTime = 0.18f;

			p.velocity = { 0.0f, 0.0f, 0.0f };
		} else if (groupName == "bossDeath_bomb") {
			// モンストっぽい「丸い爆発」がボンボン出るやつ

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// ボスの周囲にランダム配置（ちょっと縦長）
			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(-0.6f, 1.0f),
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float radius = frand(1.5f, 5.0f);               // 中心からの距離
			p.transform.translate = center + dir * radius;

			// 外向きにそこそこ速く飛ぶ（画面全体に広がる感じ）
			float speed = frand(2.5f, 6.0f);
			p.velocity = dir * speed;
			// 爆発本体をかなり大きく
			float sc = frand(3.0f, 6.0f);
			p.transform.scale = { sc, sc, sc };
			// 少しだけ長く見せる
			p.lifeTime = frand(0.35f, 0.65f);
			p.currentTime = 0.0f;

			// オレンジ〜黄色の炎色
			Vector3 col{
				1.0f,
				frand(0.45f, 0.9f),
				frand(0.0f, 0.25f)
			};
			p.color = { col.x, col.y, col.z, 1.0f };

		} else if (groupName == "bossDeath_ring") {
			// 画面を埋めるくらいの衝撃波リング

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 中心固定
			p.transform.translate = center;

			// 大きなリング
			float sc = frand(6.0f, 10.0f);
			p.transform.scale = { sc, sc, sc };

			p.velocity = { 0.0f, 0.0f, 0.0f };

			// 少し長めに残る
			p.lifeTime = frand(0.5f, 0.9f);
			p.currentTime = 0.0f;

			// 黄白っぽい衝撃波カラー
			p.color = { 1.0f, 0.88f, 0.55f, 1.0f };

		} else if (groupName == "bossDeath_smoke") {
			// 爆発のあとの大きな煙

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// ボスの周囲に大きく散らす
			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(0.0f, 1.0f),
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			// ボスの周囲にさらに広く散らす
			float radius = frand(6.0f, 14.0f);
			p.transform.translate = center + dir * radius;
			// 上方向にもう少し強めに流れる
			float speed = frand(0.8f, 1.6f);
			// 大きめの煙
			float sc = frand(3.0f, 5.5f);
			p.transform.scale = { sc, sc, sc };
			// かなり長く残して「余韻」を出す
			p.lifeTime = frand(2.0f, 3.0f);
			p.currentTime = 0.0f;

			// 少し青みがかった白煙〜灰
			Vector3 col3{
				frand(0.70f, 0.90f),
				frand(0.72f, 0.92f),
				frand(0.78f, 0.96f)
			};
			p.color = { col3.x, col3.y, col3.z, 1.0f };
		} else if (groupName == "bossClear_core") {
			// ボス撃破後の「クリア決定打」になる爆心コア

			p.transform.translate = center;

			// かなりデカい光球（ボスを飲み込むレベル）
			float sc = std::uniform_real_distribution<float>(5.0f, 8.0f)(rng);
			p.transform.scale = { sc, sc, sc };

			// 動かない・その場でドーンと光る
			p.velocity = { 0.0f, 0.0f, 0.0f };

			// 少し長めに残して余韻を出す
			p.lifeTime = std::uniform_real_distribution<float>(0.6f, 0.9f)(rng);
			p.currentTime = 0.0f;

			// 白〜黄金色のまぶしい爆心
			p.color = { 1.0f, 0.96f, 0.80f, 1.0f };

		} else if (groupName == "bossClear_ring") {
			// ボス撃破後の超デカいショックウェーブ

			p.transform.translate = center;

			// 半径かなり大きめ
			float sc = std::uniform_real_distribution<float>(7.0f, 11.0f)(rng);
			p.transform.scale = { sc, sc, sc };

			// ほぼ動かない（サイズで見せる）
			p.velocity = { 0.0f, 0.0f, 0.0f };

			// 少し残る
			p.lifeTime = std::uniform_real_distribution<float>(0.45f, 0.7f)(rng);
			p.currentTime = 0.0f;

			// 外周が少しオレンジがかった白
			p.color = { 1.0f, 0.9f, 0.65f, 1.0f };

		} else if (groupName == "bossClear_spark") {
			// 爆発で四方八方に飛ぶ光の破片

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			// 中心からランダム方向へ
			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(-1.0f, 1.0f),
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			// 少しだけ中心から離して出す
			float radius = frand(0.5f, 4.0f);
			p.transform.translate = center + dir * radius;

			// かなり速く飛ばす
			float speed = frand(8.0f, 18.0f);
			p.velocity = dir * speed;

			// 細長い破片っぽく
			float len = frand(0.6f, 1.4f);
			float thin = frand(0.12f, 0.25f);
			p.transform.scale = { len, thin, 1.0f };

			// 短命〜中くらい
			p.lifeTime = frand(0.25f, 0.5f);
			p.currentTime = 0.0f;

			// 黄〜白〜オレンジ寄りの光
			Vector3 col{
				1.0f,
				frand(0.7f, 0.95f),
				frand(0.3f, 0.6f)
			};
			p.color = { col.x, col.y, col.z, 1.0f };

		} else if (groupName == "bossClear_debris") {
			// 重めに飛んでいく破片（ゲームクリア感の余韻）

			auto frand = [&](float a, float b) {
				return std::uniform_real_distribution<float>(a, b)(rng);
				};

			Vector3 dir{
				frand(-1.0f, 1.0f),
				frand(0.2f, 1.0f),      // ちょい上向き寄り
				frand(-1.0f, 1.0f)
			};
			if (MyMath::Length(dir) < 0.001f) {
				dir = { 0.0f, 1.0f, 0.0f };
			}
			dir = MyMath::Normalize(dir);

			float radius = frand(1.0f, 5.0f);
			p.transform.translate = center + dir * radius;

			// スパークより遅め・重たい感じ
			float speed = frand(3.0f, 8.0f);
			p.velocity = dir * speed;

			// ゴツい破片
			float sc = frand(0.4f, 0.9f);
			p.transform.scale = { sc, sc, sc };

			// 長めに残る
			p.lifeTime = frand(0.7f, 1.4f);
			p.currentTime = 0.0f;

			// 暗めのオレンジ〜焦げ茶っぽい色
			Vector3 col{
				frand(0.4f, 0.8f),
				frand(0.2f, 0.4f),
				frand(0.05f, 0.15f)
			};
			p.color = { col.x, col.y, col.z, 1.0f };
		} else if (groupName == "trail_lt_path") {
			// 発生位置：完全に中心だけ
			p.transform.translate = center;

			// 速度なし（軌道は弾そのものが描く）
			p.velocity = { 0.0f, 0.0f, 0.0f };

			// ★ 完全等方スケール（横長禁止）
			std::uniform_real_distribution<float> scl(0.08f, 0.14f);
			float sc = scl(rng);
			p.transform.scale = { sc, sc, sc };

			// ★ 短命（線にならない）
			p.lifeTime = std::uniform_real_distribution<float>(0.08f, 0.15f)(rng);
			p.currentTime = 0.0f;

			// 色：必殺技らしく白〜薄青
			float c = std::uniform_real_distribution<float>(0.85f, 1.0f)(rng);
			p.color = { 0.9f * c, 0.95f * c, 1.0f, 1.0f };
		} else { // 上記意外
			// ── 既存：ヒット/汎用（上にふわっと・暖色系） ──
			std::uniform_real_distribution<float> velX(-0.15f, 0.15f);
			std::uniform_real_distribution<float> velY(0.10f, 0.30f);
			p.velocity = { velX(rng), velY(rng), velX(rng) };

			float sc = std::uniform_real_distribution<float>(1.0f, 2.0f)(rng);
			p.transform.scale = { sc, sc, sc };

			float life = std::uniform_real_distribution<float>(0.6f, 1.2f)(rng);
			p.lifeTime = life; p.currentTime = 0.0f;

			float base = std::uniform_real_distribution<float>(0.8f, 1.0f)(rng);
			p.color = { base, base * 0.5f, base * 0.2f, 1.0f };
		}

		p.transform.rotate = { 0,0,0 }; // 使ってなければ0で
		return p;
	}

	void ParticleManager::CreateRingVertices() {
		for (uint32_t index = 0; index < kRingDivide; ++index) { // 分割数分ループ
			float theta = index * radianPerDivide; // 現在の角度
			float nextTheta = (index + 1) * radianPerDivide; // 次の角度

			// 現在と次のサイン・コサインを計算
			float sin = std::sin(theta);
			float cos = std::cos(theta);
			float sinNext = std::sin(nextTheta);
			float cosNext = std::cos(nextTheta);

			// U座標を計算
			float u = float(index) / float(kRingDivide);
			float uNext = float(index + 1) / float(kRingDivide);

			// 頂点の位置を計算
			Vector4 outerCurr = { -sin * kOuterRadius, cos * kOuterRadius, 0.0f, 1.0f };
			Vector4 outerNext = { -sinNext * kOuterRadius, cosNext * kOuterRadius, 0.0f, 1.0f };
			Vector4 innerCurr = { -sin * kInnerRadius, cos * kInnerRadius, 0.0f, 1.0f };
			Vector4 innerNext = { -sinNext * kInnerRadius, cosNext * kInnerRadius, 0.0f, 1.0f };

			// 1枚目の三角形
			ringModelData.vertices.push_back({ outerCurr, {u, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData.vertices.push_back({ outerNext, {uNext, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData.vertices.push_back({ innerCurr, {u, 1.0f}, {0.0f, 0.0f, 1.0f} });

			// 2枚目の三角形
			ringModelData.vertices.push_back({ innerCurr, {u, 1.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData.vertices.push_back({ outerNext, {uNext, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData.vertices.push_back({ innerNext, {uNext, 1.0f}, {0.0f, 0.0f, 1.0f} });
		}

	}

	void ParticleManager::CreateCylinderVertices() {
		const uint32_t kHeightDivide = 8; // 縦方向の分割数（お好みで）
		const float height = 2.0f;        // 円柱の高さ
		const float halfHeight = height / 2.0f;

		for (uint32_t h = 0; h < kHeightDivide; ++h) { // 縦方向の分割ループ
			// 現在と次のY座標、V座標を計算
			float y0 = -halfHeight + height * (float(h) / kHeightDivide);
			float y1 = -halfHeight + height * (float(h + 1) / kHeightDivide);
			float v0 = float(h) / kHeightDivide;
			float v1 = float(h + 1) / kHeightDivide;

			for (uint32_t i = 0; i < kRingDivide; ++i) { // 横方向の分割ループ
				// 現在と次の角度を計算
				float theta0 = i * radianPerDivide;
				float theta1 = (i + 1) * radianPerDivide;
				// 現在と次のサイン・コサインを計算
				float sin0 = std::sin(theta0);
				float cos0 = std::cos(theta0);
				float sin1 = std::sin(theta1);
				float cos1 = std::cos(theta1);
				// 頂点の位置を計算
				float x0 = cos0 * kOuterRadius;
				float z0 = -sin0 * kOuterRadius;
				float x1 = cos1 * kOuterRadius;
				float z1 = -sin1 * kOuterRadius;
				// U座標を計算
				float u0 = float(i) / kRingDivide;
				float u1 = float(i + 1) / kRingDivide;
				// 法線ベクトルを計算
				Vector3 normal0 = { cos0, 0.0f, -sin0 };
				Vector3 normal1 = { cos1, 0.0f, -sin1 };

				// 1枚目の三角形
				cylinderModelData.vertices.push_back({ {x0, y0, z0, 1.0f}, {u0, v0}, normal0 });
				cylinderModelData.vertices.push_back({ {x1, y0, z1, 1.0f}, {u1, v0}, normal1 });
				cylinderModelData.vertices.push_back({ {x0, y1, z0, 1.0f}, {u0, v1}, normal0 });

				// 2枚目の三角形
				cylinderModelData.vertices.push_back({ {x0, y1, z0, 1.0f}, {u0, v1}, normal0 });
				cylinderModelData.vertices.push_back({ {x1, y0, z1, 1.0f}, {u1, v0}, normal1 });
				cylinderModelData.vertices.push_back({ {x1, y1, z1, 1.0f}, {u1, v1}, normal1 });
			}
		}
	}

	//void ParticleManager::CreateRibbonVertices() {
	//	// 横長リボン（幅：2.0、高さ：0.3）みたいな比率で作る
	//	const float halfW = 1.0f;   // X 方向
	//	const float halfH = 0.15f;  // Y 方向（細い）
	//
	//	// 三角形2つ分（通常クアッド）
	//	ribbonModelData.vertices.push_back({
	//		.position = { halfW,  halfH, 0.0f, 1.0f},
	//		.texcoord = {0.0f, 0.0f},
	//		.normal = {0.0f, 0.0f, 1.0f}
	//		});
	//	ribbonModelData.vertices.push_back({
	//		.position = {-halfW,  halfH, 0.0f, 1.0f},
	//		.texcoord = {1.0f, 0.0f},
	//		.normal = {0.0f, 0.0f, 1.0f}
	//		});
	//	ribbonModelData.vertices.push_back({
	//		.position = { halfW, -halfH, 0.0f, 1.0f},
	//		.texcoord = {0.0f, 1.0f},
	//		.normal = {0.0f, 0.0f, 1.0f}
	//		});
	//
	//	ribbonModelData.vertices.push_back({
	//		.position = { halfW, -halfH, 0.0f, 1.0f},
	//		.texcoord = {0.0f, 1.0f},
	//		.normal = {0.0f, 0.0f, 1.0f}
	//		});
	//	ribbonModelData.vertices.push_back({
	//		.position = {-halfW,  halfH, 0.0f, 1.0f},
	//		.texcoord = {1.0f, 0.0f},
	//		.normal = {0.0f, 0.0f, 1.0f}
	//		});
	//	ribbonModelData.vertices.push_back({
	//		.position = {-halfW, -halfH, 0.0f, 1.0f},
	//		.texcoord = {1.0f, 1.0f},
	//		.normal = {0.0f, 0.0f, 1.0f}
	//		});
	//}
}