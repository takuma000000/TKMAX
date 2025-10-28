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

void ParticleManager::Update()
{
	MakeBillboardMatrix(); //ビルボードマトリクス作成

	//カメラの各種行列を取得
	camera_->GetViewMatrix();
	camera_->GetProjectionMatrix();

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
			Matrix4x4 viewMatrix = MyMath::Inverse4x4(cameraMatrix);
			Matrix4x4 projectionMatrix = MyMath::MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = MyMath::Multiply(worldMatrix, MyMath::Multiply(viewMatrix, projectionMatrix));
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

void ParticleManager::Draw()
{
	auto* cmd = dxCommon_->GetCommandList(); // コマンドリスト取得

	// 共通セット
	cmd->SetGraphicsRootSignature(rootSignature.Get());
	cmd->SetPipelineState(graphicsPipelineState.Get());
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 頂点数取得
	const UINT vtxCountNormal = static_cast<UINT>(modelData.vertices.size());
	const UINT vtxCountRing = static_cast<UINT>(ringModelData.vertices.size());
	const UINT vtxCountCylinder = static_cast<UINT>(cylinderModelData.vertices.size());

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

void ParticleManager::CreatePipeline()
{
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

void ParticleManager::CreateRootSigunature()
{
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

void ParticleManager::InitializeVD()
{
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
	// すでに存在するなら何もしない（安全な再呼び出し対応）
	if (particleGroups.find(name) != particleGroups.end()) {
		return;
	}

	// 新規作成
	ParticleGroup newGroup;
	newGroup.materialData.textureFilePath = textureFilePath;
	newGroup.type = type;

	// テクスチャ読み込み＆SRV取得
	TextureManager::GetInstance()->LoadTexture(textureFilePath);
	uint32_t srvIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	newGroup.materialData.textureIndex = srvIndex;
	// インスタンシング用バッファ作成
	newGroup.kNumInstance = 100;
	size_t bufferSize = sizeof(ParticleForGPU) * newGroup.kNumInstance;
	newGroup.instancingResource = dxCommon_->CreateBufferResource(bufferSize);
	newGroup.instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.instancingData));

	// インスタンシング用SRV作成
	uint32_t instanceSrvIndex = srvManager_->Allocate();
	srvManager_->CreateSRVforStructureBuffer(instanceSrvIndex, newGroup.instancingResource.Get(), newGroup.kNumInstance, sizeof(ParticleForGPU));
	newGroup.srvIndex = instanceSrvIndex;

	particleGroups[name] = newGroup; // 登録
}

void ParticleManager::MakeBillboardMatrix()
{
	//カメラの向きに回転するビルボード行列を作成
	Matrix4x4 backToFrontMatrix = MyMath::MakeRotateYMatrix(std::numbers::pi_v<float>);
	//ビルボード行列 = カメラのワールド行列 × Z180度回転行列
	billboardMatrix = MyMath::Multiply(backToFrontMatrix, camera_->GetWorldMatrix());

	billboardMatrix.m[3][0] = 0.0f; //平行移動成分はいらない
	billboardMatrix.m[3][1] = 0.0f; //平行移動成分はいらない
	billboardMatrix.m[3][2] = 0.0f; //平行移動成分はいらない

}

void ParticleManager::Emit(const std::string name, Vector3& pos, uint32_t count)
{
	assert(particleGroups.find(name) != particleGroups.end());
	ParticleGroup& group = particleGroups[name]; // パーティクルグループの参照を取得

	for (uint32_t i = 0; i < count; ++i) { // 指定数分パーティクル生成
		Particle newParticle = MakeNewParticle(randomEngine, name, pos); // ← name を渡す
		group.particles.push_back(newParticle);
	}
}


ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& rng, const std::string& groupName, const Vector3& center)
{
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
	} else if (groupName == "jetSmoke") { //── ジェット噴射煙 ──
		std::uniform_real_distribution<float> velX(-0.05f, 0.05f);
		std::uniform_real_distribution<float> velY(0.10f, 0.25f);
		std::uniform_real_distribution<float> velZ(-45.0f, -25.0f);
		p.velocity = { velX(rng), velY(rng), velZ(rng) };

		std::uniform_real_distribution<float> scl(0.5f, 1.0f);
		float sc = scl(rng);
		p.transform.scale = { sc, sc, sc };

		p.lifeTime = std::uniform_real_distribution<float>(1.5f, 2.5f)(rng);
		p.currentTime = 0.0f;

		// --- ランダムカラー煙：温～冷まで ---
		// 0.0 = 灰 (冷) ～ 1.0 = オレンジ白 (温)
		float hueType = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);

		Vector3 col3;
		if (hueType < 0.33f) {
			// 冷たい灰～青系
			col3 = { 0.75f, 0.78f, 0.85f };
		} else if (hueType < 0.66f) {
			// 標準的な白煙（少し黄味）
			col3 = { 0.88f, 0.86f, 0.80f };
		} else {
			// 暖かいオレンジ～クリーム系
			col3 = { 0.95f, 0.90f, 0.82f };
		}

		// 明度を少しランダムに
		float brightness = std::uniform_real_distribution<float>(0.8f, 1.0f)(rng);
		col3 = col3 * brightness;

		p.color = { col3.x, col3.y, col3.z, 1.0f }; // アルファは不透明スタート
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
		// LT：黄緑系（視認性が高く色弱でも区別しやすい）
		std::uniform_real_distribution<float> velX(-0.02f, 0.02f);
		std::uniform_real_distribution<float> velY(-0.02f, 0.02f);
		std::uniform_real_distribution<float> velZ(-2.5f, -1.0f);
		p.velocity = { velX(rng), velY(rng), velZ(rng) };

		float sc = std::uniform_real_distribution<float>(0.15f, 0.30f)(rng);
		p.transform.scale = { sc, sc, sc };
		p.lifeTime = std::uniform_real_distribution<float>(0.3f, 0.6f)(rng);
		p.currentTime = 0.0f;

		// 黄緑〜緑
		float t = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
		Vector3 col = { 0.4f + 0.3f * t, 1.0f, 0.3f + 0.3f * t };
		p.color = { 1.0f, 1.0f, 1.0f, 1.0f };  // 純白
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

void ParticleManager::CreateRingVertices()
{
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