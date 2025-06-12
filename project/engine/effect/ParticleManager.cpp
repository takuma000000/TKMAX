#include "ParticleManager.h"
#include "TextureManager.h"
#include "MyMath.h"
#include <numbers>


ParticleManager* ParticleManager::instance = nullptr;

ParticleManager* ParticleManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new ParticleManager();
	}

	return instance;
}

void ParticleManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, Camera* camera)
{
	//引数で受け取る
	dxCommon_ = dxCommon;
	srvManager_ = srvManager;
	camera_ = camera;

	acc.acc = { 0.0f,0.0f,0.0f };
	acc.area.min = { -1.0f,-1.00f,-1.0f };
	acc.area.max = { 1.0f,1.0f,1.0f };

	//ランダムエンジンの初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	CreatePipeline();
	InitializeVD();
	CreateVR();
	CreateVB();
	WriteResource();

}

void ParticleManager::Update()
{
	MakeBillboardMatrix();

	camera_->GetViewMatrix();
	camera_->GetProjectionMatrix();

	for (std::unordered_map<std::string, ParticleGroup>::iterator particleGroupIterator = particleGroups.begin(); particleGroupIterator != particleGroups.end();) {

		ParticleGroup* particleGroup = &(particleGroupIterator->second);
		particleGroupIterator->second.kNumInstance = 0;


		for (std::list<Particle>::iterator particleIterator = particleGroup->particles.begin(); particleIterator != particleGroup->particles.end();) {
			if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {//生存期間を過ぎていたら更新せず描画対象にしない
				particleIterator = particleGroup->particles.erase(particleIterator);
				continue;
			}
			Matrix4x4 scaleMatrix = MyMath::MakeScaleMatrix((*particleIterator).transform.scale);
			Matrix4x4 translateMatrix = MyMath::MakeTranslateMatrix((*particleIterator).transform.translate);
			Matrix4x4 rotateMatrix = MyMath::MakeRotateMatrix((*particleIterator).transform.rotate);
			Matrix4x4 worldMatrix = scaleMatrix * rotateMatrix * billboardMatrix * translateMatrix;
			Matrix4x4 cameraMatrix = MyMath::MakeAffineMatrix(camera_->GetScale(), camera_->GetRotate(), camera_->GetTranslate());
			Matrix4x4 viewMatrix = MyMath::Inverse4x4(cameraMatrix);
			Matrix4x4 projectionMatrix = MyMath::MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = MyMath::Multiply(worldMatrix, MyMath::Multiply(viewMatrix, projectionMatrix));
			if (particleGroupIterator->second.kNumInstance < kNumMaxInstance) {
				//フィールドの範囲内のParticleには加速度を適用する
				if (IsCollision(acc.area, (*particleIterator).transform.translate)) {
					(*particleIterator).velocity += acc.acc * kDeltaTime;
				}
				(*particleIterator).transform.translate += (*particleIterator).velocity * kDeltaTime;
				(*particleIterator).currentTime += kDeltaTime;//経過時間を足す
				particleGroup->instancingData[particleGroupIterator->second.kNumInstance].wvp = worldViewProjectionMatrix;
				particleGroup->instancingData[particleGroupIterator->second.kNumInstance].World = worldMatrix;
				particleGroup->instancingData[particleGroupIterator->second.kNumInstance].color = (*particleIterator).color;
				float alpha = 1.0f - ((*particleIterator).currentTime / (*particleIterator).lifeTime);
				particleGroup->instancingData[particleGroupIterator->second.kNumInstance].color.w = alpha;
				++particleGroupIterator->second.kNumInstance;//生きているParticleの数を1つカウントする
			}

			++particleIterator;

		}
		++particleGroupIterator;
	}
}

void ParticleManager::Draw()
{
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (std::unordered_map<std::string, ParticleGroup>::iterator particleGroupIterator = particleGroups.begin(); particleGroupIterator != particleGroups.end();) {

		//マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
		materialResource = dxCommon_->CreateBufferResource(sizeof(Material));
		//マテリアルにデータを書き込む
		Material* materialData = nullptr;
		//書き込むためのアドレスを取得
		materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
		//今回は白を書き込んでみる
		materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		materialData->enableLighting = true;
		materialData->uvTransform = MyMath::MakeIdentity4x4();

		dxCommon_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource.Get()->GetGPUVirtualAddress());
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(particleGroupIterator->second.srvIndex));
		dxCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(2, srvManager_->GetGPUDescriptorHandle(particleGroupIterator->second.materialData.textureIndex));

		ParticleGroup& group = particleGroupIterator->second;//グループの取得
		// 頂点バッファ切り替え
		if (group.type == ParticleType::NORMAL) {
			dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
			dxCommon_->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), group.kNumInstance, 0, 0);
		} else if (group.type == ParticleType::RING) {
			dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &ringVertexBufferView);
			dxCommon_->GetCommandList()->DrawInstanced(UINT(ringModelData.vertices.size()), group.kNumInstance, 0, 0);
		} else if (group.type == ParticleType::CYLINDER) {
			dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &cylinderVertexBufferView);
			dxCommon_->GetCommandList()->DrawInstanced(UINT(cylinderModelData.vertices.size()), group.kNumInstance, 0, 0);
		}

		++particleGroupIterator;
	}
}

void ParticleManager::CreatePipeline()
{
	HRESULT hr;

	//呼び出し
	CreateRootSigunature();

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

	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	D3D12_RASTERIZER_DESC resterizerDesc{};
	resterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	resterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompileShader(L"Particle.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);
	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompileShader(L"Particle.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;
	//書き込みします
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	//比較関数はLessEqual。つまり、近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

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

	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
}

void ParticleManager::CreateRootSigunature()
{
	HRESULT hr;

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0;//0から始まる
	descriptorRange[0].NumDescriptors = 1;//数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//Offsetを自動計算

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
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
		assert(false);
	}

	//バイナリを元に生成
	rootSignature = nullptr;
	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlog->GetBufferPointer(), signatureBlog->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
}

void ParticleManager::InitializeVD()
{
	modelData.vertices.push_back({ .position = {1.0f,1.0f,0.0f,1.0f},.texcoord = {0.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {1.0f,-1.0f,0.0f,1.0f},.texcoord = {0.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,1.0f,0.0f,1.0f},.texcoord = {1.0f,0.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.vertices.push_back({ .position = {-1.0f,-1.0f,0.0f,1.0f},.texcoord = {1.0f,1.0f},.normal = {0.0f,0.0f,1.0f} });
	modelData.material.textureFilePath = "./resources/circle.png";

	CreateRingVertices();
	ringModelData.material.textureFilePath = "./resources/gradationLine.png";

	CreateCylinderVertices();
	cylinderModelData.material.textureFilePath = "./resources/gradationLine.png";
}

void ParticleManager::CreateVR()
{
	//頂点リソースを作る
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//リングの頂点リソースを作る
	ringVertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * ringModelData.vertices.size());
	//cylinderの頂点リソースを作る
	cylinderVertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * cylinderModelData.vertices.size());
}

void ParticleManager::CreateVB()
{
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
}

void ParticleManager::WriteResource()
{
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

}

void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& textureFilePath, ParticleType type)
{
	assert(particleGroups.find(name) == particleGroups.end());

	ParticleGroup newGroup;
	newGroup.materialData.textureFilePath = textureFilePath;
	newGroup.type = type;

	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	uint32_t srvIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	newGroup.materialData.textureIndex = srvIndex;

	// SRVインデックスを確保
	uint32_t instanceSrvIndex = srvManager_->Allocate();

	// インスタンシング用リソースの作成
	newGroup.kNumInstance = 100; // 必要なインスタンス数（仮で100）。用途に応じて調整
	size_t bufferSize = sizeof(ParticleForGPU) * newGroup.kNumInstance;
	newGroup.instancingResource = dxCommon_->CreateBufferResource(bufferSize);
	newGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.instancingData));

	// 構造化バッファ用のSRVを生成
	srvManager_->CreateSRVforStructureBuffer(
		instanceSrvIndex,
		newGroup.instancingResource.Get(),
		newGroup.kNumInstance,       // インスタンスの数
		sizeof(ParticleForGPU)       // 各インスタンスの構造体サイズ
	);

	// SRVインデックスを記録
	newGroup.srvIndex = instanceSrvIndex;

	particleGroups[name] = newGroup;
}

void ParticleManager::MakeBillboardMatrix()
{

	Matrix4x4 backToFrontMatrix = MyMath::MakeRotateYMatrix(std::numbers::pi_v<float>);

	billboardMatrix = MyMath::Multiply(backToFrontMatrix, camera_->GetWorldMatrix());

	billboardMatrix.m[3][0] = 0.0f;//平行移動成分はいらない
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

}

void ParticleManager::Emit(const std::string name, Vector3& pos, uint32_t count)
{
	// 登録済みのパーティクルグループかチェックしてassert
	assert(particleGroups.find(name) != particleGroups.end());

	// 指定されたパーティクルグループを取得
	ParticleGroup& group = particleGroups[name];

	// パーティクルを生成してグループに追加
	for (uint32_t i = 0; i < count; ++i) {
		Particle newParticle = MakeNewParticle(randomEngine, pos);
		group.particles.push_back(newParticle);
	}
}

ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate)
{
	std::uniform_real_distribution<float> distPos(-0.2f, 0.2f);     // ばらける範囲広め
	std::uniform_real_distribution<float> distVel(-0.6f, 0.6f);     // 弾ける強さUP
	std::uniform_real_distribution<float> distScale(0.5f, 3.0f);    // ← サイズ大きく
	std::uniform_real_distribution<float> distTime(0.8f, 1.5f);     // ← 寿命ちょい長め
	std::uniform_real_distribution<float> distColor(0.8f, 1.0f);    // 白～黄

	Particle particle;

	particle.transform.scale = {
		distScale(randomEngine),
		distScale(randomEngine),
		distScale(randomEngine)
	};
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.transform.translate = translate + Vector3(
		distPos(randomEngine),
		distPos(randomEngine),
		distPos(randomEngine)
	);

	// 上にも弾け飛ばす
	particle.velocity = Vector3(
		distVel(randomEngine),
		distVel(randomEngine) + 0.3f, // 上向き追加
		distVel(randomEngine)
	);

	// 黄色寄りの白（赤と緑強め）
	float r = 1.0f;
	float g = distColor(randomEngine);
	float b = distColor(randomEngine) * 0.3f;
	particle.color = { r, g, b, 1.0f };

	particle.lifeTime = distTime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}



void ParticleManager::CreateRingVertices()
{
	for (uint32_t index = 0; index < kRingDivide; ++index) {
		float theta = index * radianPerDivide;
		float nextTheta = (index + 1) * radianPerDivide;

		float sin = std::sin(theta);
		float cos = std::cos(theta);
		float sinNext = std::sin(nextTheta);
		float cosNext = std::cos(nextTheta);

		float u = float(index) / float(kRingDivide);
		float uNext = float(index + 1) / float(kRingDivide);

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

	for (uint32_t h = 0; h < kHeightDivide; ++h) {
		float y0 = -halfHeight + height * (float(h) / kHeightDivide);
		float y1 = -halfHeight + height * (float(h + 1) / kHeightDivide);
		float v0 = float(h) / kHeightDivide;
		float v1 = float(h + 1) / kHeightDivide;

		for (uint32_t i = 0; i < kRingDivide; ++i) {
			float theta0 = i * radianPerDivide;
			float theta1 = (i + 1) * radianPerDivide;

			float sin0 = std::sin(theta0);
			float cos0 = std::cos(theta0);
			float sin1 = std::sin(theta1);
			float cos1 = std::cos(theta1);

			float x0 = cos0 * kOuterRadius;
			float z0 = -sin0 * kOuterRadius;
			float x1 = cos1 * kOuterRadius;
			float z1 = -sin1 * kOuterRadius;

			float u0 = float(i) / kRingDivide;
			float u1 = float(i + 1) / kRingDivide;

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


