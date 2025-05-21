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
	dxCommon_->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
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
		dxCommon_->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), particleGroupIterator->second.kNumInstance, 0, 0);

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
	resterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
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
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//0～1の範囲外をリピート
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
}

void ParticleManager::CreateVR()
{
	//頂点リソースを作る
	vertexResource = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
}

void ParticleManager::CreateVB()
{
	//頂点バッファビューを作成する
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);
}

void ParticleManager::WriteResource()
{
	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	//書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void ParticleManager::CreateParticleGroup(const std::string name, const std::string textureFilePath)
{
	// 登録済みの名前かチェックしてassert
	assert(particleGroups.find(name) == particleGroups.end());

	// 新たな空のパーティクルグループを作成し、コンテナに登録
	ParticleGroup newGroup;

	newGroup.materialData.textureFilePath = textureFilePath; // マテリアルデータにテクスチャファイルパスを設定

	// テクスチャを読み込む
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

	// テクスチャのSRVインデックスを取得
	uint32_t srvIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	newGroup.srvIndex = srvIndex;

	// ✅ これを追加！
	newGroup.materialData.textureIndex = srvIndex;

	// インスタンシング用リソースの作成
	newGroup.kNumInstance = 100; // 必要なインスタンス数（仮で100）。用途に応じて調整
	size_t bufferSize = sizeof(ParticleForGPU) * newGroup.kNumInstance;

	// GPUリソースを作成
	newGroup.instancingResource = dxCommon_->CreateBufferResource(bufferSize);

	newGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.instancingData));

	// SRVインデックスを確保
	uint32_t instanceSrvIndex = srvManager_->Allocate();

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

ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& randomEngine, const Vector3& center)
{
	std::uniform_real_distribution<float> distAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distSpeed(0.8f, 2.0f);
	std::uniform_real_distribution<float> distScale(0.05f, 0.15f);
	std::uniform_real_distribution<float> distLifetime(0.15f, 0.3f);
	std::uniform_real_distribution<float> distOffset(-0.1f, 0.1f); // ★ 新規：中心からのずれ範囲

	std::uniform_int_distribution<int> colorTypeDist(0, 3);

	const Vector4 colorList[] = {
		{1.0f, 1.0f, 1.0f, 1.0f},  // 真っ白
		{0.0f, 0.0f, 1.0f, 1.0f},  // 青
		{1.0f, 0.0f, 0.0f, 1.0f},  // 赤
		{1.0f, 1.0f, 0.0f, 1.0f},  // 黄
		{1.0f, 0.5f, 0.5f, 1.0f},  // ピンク
		{0.5f, 1.0f, 0.5f, 1.0f},  // 緑
		{0.5f, 0.5f, 1.0f, 1.0f},  // 水色
		{1.0f, 0.5f, 1.0f, 1.0f},   // 紫
		{1.0f, 0.84f, 0.0f, 1.0f}  // 金色
	};

	Particle particle;

	// ★ 発生位置にランダムオフセットを加える
	Vector3 offset = {
		distOffset(randomEngine),
		distOffset(randomEngine),
		distOffset(randomEngine)
	};
	particle.transform.translate = center + offset;

	// ランダム放射方向
	float theta = distAngle(randomEngine);
	float phi = distAngle(randomEngine);

	Vector3 dir = {
		std::cos(theta) * std::sin(phi),
		std::cos(phi),
		std::sin(theta) * std::sin(phi)
	};

	particle.velocity = dir * distSpeed(randomEngine);
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };

	float scale = distScale(randomEngine);
	particle.transform.scale = { scale, scale, scale };

	particle.color = colorList[colorTypeDist(randomEngine)];

	particle.lifeTime = distLifetime(randomEngine);
	particle.currentTime = 0.0f;

	return particle;
}










