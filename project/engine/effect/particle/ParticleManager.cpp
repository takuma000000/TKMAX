#define NOMINMAX
#include "ParticleManager.h"
#include "TextureManager.h"
#include "ParticleSpawner.h"
#include "MyMath.h"
#include <numbers>
#include <algorithm>

namespace TKM {
	ParticleManager* ParticleManager::instance_ = nullptr;

	ParticleManager* ParticleManager::GetInstance() {
		if (instance_ == nullptr) {
			instance_ = new ParticleManager();
		}

		return instance_;
	}

	void ParticleManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager, TKM::Camera* camera) {
		//引数で受け取る
		dxCommon_ = dxCommon;
		srvManager_ = srvManager;
		camera_ = camera;

		// --- 加速度フィールド初期化 ---
		acc.acc_ = { 0.0f,0.0f,0.0f };
		acc.area_.min_ = { -1.0f,-1.00f,-1.0f };
		acc.area_.max_ = { 1.0f,1.0f,1.0f };

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

		for (std::unordered_map<std::string, ParticleGroup>::iterator particleGroupIterator = particleGroups_.begin(); particleGroupIterator != particleGroups_.end();) { //各パーティクルグループの更新
			//パーティクルグループのポインタを取得
			ParticleGroup* particleGroup = &(particleGroupIterator->second);
			particleGroupIterator->second.kNumInstance_ = 0;

			for (std::list<Particle>::iterator particleIterator = particleGroup->particles_.begin(); particleIterator != particleGroup->particles_.end();) { //各パーティクルの更新
				if ((*particleIterator).lifeTime_ <= (*particleIterator).currentTime_) {//生存期間を過ぎていたら更新せず描画対象にしない
					particleIterator = particleGroup->particles_.erase(particleIterator);
					continue;
				}
				//ワールド行列計算
				Matrix4x4 scaleMatrix = MyMath::MakeScaleMatrix((*particleIterator).transform_.scale_);
				Matrix4x4 translateMatrix = MyMath::MakeTranslateMatrix((*particleIterator).transform_.translate_);
				Matrix4x4 rotateMatrix = MyMath::MakeRotateMatrix((*particleIterator).transform_.rotate_);
				Matrix4x4 worldMatrix{};
				if (particleGroupIterator->second.type_ == ParticleType::RIBBON) {
					// リボンは“弾道方向”が命。ビルボードしない
					worldMatrix = scaleMatrix * rotateMatrix * translateMatrix;
				} else {
					worldMatrix = scaleMatrix * rotateMatrix * billboardMatrix_ * translateMatrix;
				}
				Matrix4x4 cameraMatrix = MyMath::MakeAffineMatrix(camera_->GetScale(), camera_->GetRotate(), camera_->GetTranslate());
				Matrix4x4 viewMatrix = camera_->GetViewMatrix();
				Matrix4x4 projectionMatrix = camera_->GetProjectionMatrix();
				Matrix4x4 worldViewProjectionMatrix =
					MyMath::Multiply(worldMatrix, MyMath::Multiply(viewMatrix, projectionMatrix));
				if (particleGroupIterator->second.kNumInstance_ < kNumMaxInstance_) { //最大インスタンス数以下なら更新と描画対象にする
					//フィールドの範囲内のParticleには加速度を適用する
					if (IsCollision(acc.area_, (*particleIterator).transform_.translate_)) { //当たり判定
						(*particleIterator).velocity_ += acc.acc_ * kDeltaTime_;
					}
					(*particleIterator).transform_.translate_ += (*particleIterator).velocity_ * kDeltaTime_; //速度を元に位置を更新
					(*particleIterator).currentTime_ += kDeltaTime_;//経過時間を足す
					//インスタンスデータ更新
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].wvp_ = worldViewProjectionMatrix;
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].World_ = worldMatrix;
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_ = (*particleIterator).color_;
					float alpha = 1.0f - ((*particleIterator).currentTime_ / (*particleIterator).lifeTime_); //アルファ値計算(0~1)
					const std::string& g = particleGroupIterator->first;
					if (g == "trail_lt_ribbon") {
						alpha = 1.0f; // 繋がって見せる（パンツァ寄せ）
					}
					particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = alpha;
					float t = (*particleIterator).currentTime_ / (*particleIterator).lifeTime_;
					t = std::clamp(t, 0.0f, 1.0f);

					// 広がり（リング・コア）
					if (g == "titleExplode_ring") {
						float grow = 1.0f + 12.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "titleExplode_core") {
						float grow = 1.0f + 18.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "titleBeamClash_ring") {
						float grow = 1.0f + 22.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}
					if (g == "titleBeamClash_core") {
						float grow = 1.0f + 10.0f * kDeltaTime_;
						(*particleIterator).transform_.scale_.x *= grow;
						(*particleIterator).transform_.scale_.y *= grow;
						(*particleIterator).transform_.scale_.z *= grow;
					}

					// アルファカーブ（“パァン”を作る）
					float a = 1.0f - t;
					if (g == "titleExplode_core") { a = a * a * a * a; }          // 速く消える白飛び
					else if (g == "titleExplode_rays") { a = a * a; }             // 光線はキレ
					else if (g == "titleExplode_ring") { a = std::pow(a, 1.2f); } // 少し残す
					else if (g == "titleExplode_debris") { a = std::pow(a, 1.6f); }

					if (g == "titleBeam_player" || g == "titleBeam_boss") {
						// ビームは短命＆キレよく
						a = a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "titleBeamClash_core") {
						// コアは白飛び気味に素早く消える
						a = a * a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "titleBeamClash_rays") {
						// スパークは一瞬だけ残す
						a = a * a;
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}
					if (g == "titleBeamClash_ring") {
						// リングは少し残して「衝撃波」を見せる
						a = std::pow(a, 1.1f);
						particleGroup->instancingData_[particleGroupIterator->second.kNumInstance_].color_.w = a;
					}

					++particleGroupIterator->second.kNumInstance_;//生きているParticleの数を1つカウントする
				}
				++particleIterator; //次のパーティクルへ
			}
			++particleGroupIterator; //次のパーティクルグループへ
		}
	}

	void ParticleManager::Draw() {
		auto* cmd = dxCommon_->GetCommandList(); // コマンドリスト取得

		// 共通セット
		cmd->SetGraphicsRootSignature(rootSignature_.Get());
		cmd->SetPipelineState(graphicsPipelineState_.Get());
		cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 頂点数取得
		const UINT vtxCountNormal = static_cast<UINT>(modelData_.vertices_.size());
		const UINT vtxCountRing = static_cast<UINT>(ringModelData_.vertices_.size());
		const UINT vtxCountCylinder = static_cast<UINT>(cylinderModelData_.vertices_.size());
		const UINT vtxCountRibbon = static_cast<UINT>(ribbonModelData_.vertices_.size());

		for (auto it = particleGroups_.begin(); it != particleGroups_.end(); ++it) { //各パーティクルグループの描画
			ParticleGroup& group = it->second;

			// ① インスタンス0なら描かない
			if (group.kNumInstance_ == 0) {
				continue;
			}

			// ② モデル頂点数0も弾く（型ごと）
			if (group.type_ == ParticleType::NORMAL && vtxCountNormal == 0) continue;
			if (group.type_ == ParticleType::RING && vtxCountRing == 0) continue;
			if (group.type_ == ParticleType::CYLINDER && vtxCountCylinder == 0) continue;
			if (group.type_ == ParticleType::RIBBON && vtxCountRibbon == 0) continue;

			// ③ 永続CBに値を書くだけ（Create/Releaseしない）
			//    ※ Initialize() で materialCB_ を UploadHeap で作って materialCPU_ を永続Map済み
			materialCPU_->color_ = Vector4(1, 1, 1, 1);
			materialCPU_->enableLighting_ = false;
			materialCPU_->uvTransform_ = MyMath::MakeIdentity4x4();

			// ④ ルートバインド
			cmd->SetGraphicsRootConstantBufferView(0, materialCB_->GetGPUVirtualAddress());
			cmd->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUDescriptorHandle(group.srvIndex_));                     // 粒子個別のSRV（頂点/インスタンス用など）
			cmd->SetGraphicsRootDescriptorTable(2, srvManager_->GetGPUDescriptorHandle(group.materialData_.textureIndex_));    // テクスチャ

			// ⑤ VB切替 & DrawInstanced
			if (group.type_ == ParticleType::NORMAL) {
				cmd->IASetVertexBuffers(0, 1, &vertexBufferView_);
				cmd->DrawInstanced(vtxCountNormal, group.kNumInstance_, 0, 0);
			} else if (group.type_ == ParticleType::RING) {
				cmd->IASetVertexBuffers(0, 1, &ringVertexBufferView_);
				cmd->DrawInstanced(vtxCountRing, group.kNumInstance_, 0, 0);
			} else if (group.type_ == ParticleType::CYLINDER) {
				cmd->IASetVertexBuffers(0, 1, &cylinderVertexBufferView_);
				cmd->DrawInstanced(vtxCountCylinder, group.kNumInstance_, 0, 0);
			} else if (group.type_ == ParticleType::RIBBON) {
				cmd->IASetVertexBuffers(0, 1, &ribbonVertexBufferView_);
				cmd->DrawInstanced(vtxCountRibbon, group.kNumInstance_, 0, 0);
			}
		}
	}

	void ParticleManager::CreatePipeline() {
		HRESULT hr;

		//呼び出し
		CreateRootSignature();

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
		graphicPipelineStateDesc.pRootSignature = rootSignature_.Get();
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
		hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
		assert(SUCCEEDED(hr));
	}

	void ParticleManager::CreateRootSignature() {
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
		rootSignature_ = nullptr;
		hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlog->GetBufferPointer(), signatureBlog->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)); //生成
		assert(SUCCEEDED(hr));
	}

	void ParticleManager::InitializeVD() {
		//四角形の頂点データ
		modelData_.vertices_.push_back({ .position_ = {1.0f,1.0f,0.0f,1.0f},.texcoord_ = {0.0f,0.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {-1.0f,1.0f,0.0f,1.0f},.texcoord_ = {1.0f,0.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {1.0f,-1.0f,0.0f,1.0f},.texcoord_ = {0.0f,1.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {1.0f,-1.0f,0.0f,1.0f},.texcoord_ = {0.0f,1.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {-1.0f,1.0f,0.0f,1.0f},.texcoord_ = {1.0f,0.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.vertices_.push_back({ .position_ = {-1.0f,-1.0f,0.0f,1.0f},.texcoord_ = {1.0f,1.0f},.normal_ = {0.0f,0.0f,1.0f} });
		modelData_.material_.textureFilePath_ = "./resources/texture/circle.png"; //テクスチャパス

		CreateRingVertices(); //リング頂点データ作成
		ringModelData_.material_.textureFilePath_ = "./resources/texture/gradationLine.png"; //テクスチャパス

		CreateCylinderVertices(); //シリンダー頂点データ作成
		cylinderModelData_.material_.textureFilePath_ = "./resources/texture/gradationLine.png"; //テクスチャパス

		CreateRibbonVertices(); // リボン（細長い板）
		ribbonModelData_.material_.textureFilePath_ = "./resources/texture/circle.png";
	}

	void ParticleManager::CreateVR() {
		//頂点リソースを作る
		vertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * modelData_.vertices_.size());
		//リングの頂点リソースを作る
		ringVertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * ringModelData_.vertices_.size());
		//cylinderの頂点リソースを作る
		cylinderVertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * cylinderModelData_.vertices_.size());
		// リボン
		ribbonVertexResource_ = dxCommon_->CreateBufferResource(sizeof(VertexData) * ribbonModelData_.vertices_.size());
	}

	void ParticleManager::CreateVB() {
		//頂点バッファビューを作成する
		vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
		vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData_.vertices_.size());
		vertexBufferView_.StrideInBytes = sizeof(VertexData);

		//リングの頂点リソースを作成する
		ringVertexBufferView_.BufferLocation = ringVertexResource_->GetGPUVirtualAddress();
		ringVertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * ringModelData_.vertices_.size());
		ringVertexBufferView_.StrideInBytes = sizeof(VertexData);

		//cylinderの頂点リソースを作成する
		cylinderVertexBufferView_.BufferLocation = cylinderVertexResource_->GetGPUVirtualAddress();
		cylinderVertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * cylinderModelData_.vertices_.size());
		cylinderVertexBufferView_.StrideInBytes = sizeof(VertexData);

		// RIBBON
		ribbonVertexBufferView_.BufferLocation = ribbonVertexResource_->GetGPUVirtualAddress();
		ribbonVertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * ribbonModelData_.vertices_.size());
		ribbonVertexBufferView_.StrideInBytes = sizeof(VertexData);
	}

	void ParticleManager::WriteResource() {
		//頂点リソースにデータを書き込む
		VertexData* vertexData = nullptr;
		//書き込むためのアドレスを取得
		vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
		std::memcpy(vertexData, modelData_.vertices_.data(), sizeof(VertexData) * modelData_.vertices_.size());

		//リングの頂点リソースを作成する
		VertexData* ringVertexData = nullptr;
		//書き込むためのアドレスを取得
		ringVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&ringVertexData));
		std::memcpy(ringVertexData, ringModelData_.vertices_.data(), sizeof(VertexData) * ringModelData_.vertices_.size());

		//cylinderの頂点リソースを作成する
		VertexData* cylinderVertexData = nullptr;
		//書き込むためのアドレスを取得
		cylinderVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&cylinderVertexData));
		std::memcpy(cylinderVertexData, cylinderModelData_.vertices_.data(), sizeof(VertexData) * cylinderModelData_.vertices_.size());

		// RIBBON
		VertexData* ribbonVertexData = nullptr;
		ribbonVertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&ribbonVertexData));
		std::memcpy(ribbonVertexData, ribbonModelData_.vertices_.data(), sizeof(VertexData) * ribbonModelData_.vertices_.size());

	}

	void ParticleManager::CreateParticleGroup(const std::string& name, const std::string& textureFilePath, ParticleType type) {
		// すでに存在するなら何もしない（安全な再呼び出し対応）
		if (particleGroups_.find(name) != particleGroups_.end()) {
			return;
		}

		// 新規作成
		ParticleGroup newGroup;
		newGroup.materialData_.textureFilePath_ = textureFilePath;
		newGroup.type_ = type;

		// テクスチャ読み込み＆SRV取得
		TKM::TextureManager::GetInstance()->LoadTexture(textureFilePath);
		uint32_t srvIndex = TKM::TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
		newGroup.materialData_.textureIndex_ = srvIndex;
		// インスタンシング用バッファ作成
		newGroup.kNumInstance_ = kNumMaxInstance_;
		size_t bufferSize = sizeof(ParticleForGPU) * newGroup.kNumInstance_;
		newGroup.instancingResource_ = dxCommon_->CreateBufferResource(bufferSize);
		newGroup.instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&newGroup.instancingData_));

		// インスタンシング用SRV作成
		uint32_t instanceSrvIndex = srvManager_->Allocate();
		srvManager_->CreateSRVforStructureBuffer(instanceSrvIndex, newGroup.instancingResource_.Get(), newGroup.kNumInstance_, sizeof(ParticleForGPU));
		newGroup.srvIndex_ = instanceSrvIndex;

		particleGroups_[name] = newGroup; // 登録
	}

	void ParticleManager::MakeBillboardMatrix() {
		//カメラの向きに回転するビルボード行列を作成
		Matrix4x4 backToFrontMatrix = MyMath::MakeRotateYMatrix(std::numbers::pi_v<float>);
		//ビルボード行列 = カメラのワールド行列 × Z180度回転行列
		billboardMatrix_ = MyMath::Multiply(backToFrontMatrix, camera_->GetWorldMatrix());

		billboardMatrix_.m[3][0] = 0.0f; //平行移動成分はいらない
		billboardMatrix_.m[3][1] = 0.0f; //平行移動成分はいらない
		billboardMatrix_.m[3][2] = 0.0f; //平行移動成分はいらない

	}

	void ParticleManager::Emit(const std::string name, const Vector3& pos, uint32_t count) {
		assert(particleGroups_.find(name) != particleGroups_.end());
		ParticleGroup& group = particleGroups_[name]; // パーティクルグループの参照を取得

		const size_t kHardCap = std::max<size_t>(group.kNumInstance_, 200); // 下限200
		for (uint32_t i = 0; i < count; ++i) {
			// 超過してたら古い順に削除（重さ対策）
			while (group.particles_.size() >= kHardCap) {
				group.particles_.pop_front();
			}
			Particle newParticle = MakeNewParticle(randomEngine_, name, pos);
			group.particles_.push_back(newParticle);
		}
	}

	void ParticleManager::EmitWithTransform(const std::string& name, const Transform& tr, const Vector4& color, uint32_t count) {
		auto it = particleGroups_.find(name);
		if (it == particleGroups_.end()) { return; }

		ParticleGroup& group = it->second;

		size_t kHardCap = std::max<size_t>(group.kNumInstance_, 200);
		if (name == "trail_lt_ribbon") {
			kHardCap = 1; // ← “レーザー本体” は常に最新1本だけ残す
		}
		for (uint32_t i = 0; i < count; ++i) {
			while (group.particles_.size() >= kHardCap) {
				group.particles_.pop_front();
			}

			Particle p{};
			p.transform_ = tr;
			p.velocity_ = { 0.0f, 0.0f, 0.0f };
			p.color_ = color;
			if (name == "trail_lt_ribbon") {
				p.lifeTime_ = 0.05f; // すぐ消える（毎フレ出すので見た目は常に繋がる）
			} else {
				p.lifeTime_ = 0.25f;
			}
			p.currentTime_ = 0.0f;

			group.particles_.push_back(p);
		}
	}

	ParticleManager::Particle ParticleManager::MakeNewParticle(std::mt19937& rng, const std::string& groupName, const Vector3& center) {
		return ParticleSpawner::MakeNewParticle(rng, groupName, center); // グループ名と中心位置を渡して、ParticleSpawnerに新しいParticleを作ってもらう
	}

	void ParticleManager::CreateRingVertices() {
		for (uint32_t index = 0; index < kRingDivide_; ++index) { // 分割数分ループ
			float theta = index * radianPerDivide_; // 現在の角度
			float nextTheta = (index + 1) * radianPerDivide_; // 次の角度

			// 現在と次のサイン・コサインを計算
			float sin = std::sin(theta);
			float cos = std::cos(theta);
			float sinNext = std::sin(nextTheta);
			float cosNext = std::cos(nextTheta);

			// U座標を計算
			float u = float(index) / float(kRingDivide_);
			float uNext = float(index + 1) / float(kRingDivide_);

			// 頂点の位置を計算
			Vector4 outerCurr = { -sin * kOuterRadius_, cos * kOuterRadius_, 0.0f, 1.0f };
			Vector4 outerNext = { -sinNext * kOuterRadius_, cosNext * kOuterRadius_, 0.0f, 1.0f };
			Vector4 innerCurr = { -sin * kInnerRadius_, cos * kInnerRadius_, 0.0f, 1.0f };
			Vector4 innerNext = { -sinNext * kInnerRadius_, cosNext * kInnerRadius_, 0.0f, 1.0f };

			// 1枚目の三角形
			ringModelData_.vertices_.push_back({ outerCurr, {u, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ outerNext, {uNext, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ innerCurr, {u, 1.0f}, {0.0f, 0.0f, 1.0f} });

			// 2枚目の三角形
			ringModelData_.vertices_.push_back({ innerCurr, {u, 1.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ outerNext, {uNext, 0.0f}, {0.0f, 0.0f, 1.0f} });
			ringModelData_.vertices_.push_back({ innerNext, {uNext, 1.0f}, {0.0f, 0.0f, 1.0f} });
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

			for (uint32_t i = 0; i < kRingDivide_; ++i) { // 横方向の分割ループ
				// 現在と次の角度を計算
				float theta0 = i * radianPerDivide_;
				float theta1 = (i + 1) * radianPerDivide_;
				// 現在と次のサイン・コサインを計算
				float sin0 = std::sin(theta0);
				float cos0 = std::cos(theta0);
				float sin1 = std::sin(theta1);
				float cos1 = std::cos(theta1);
				// 頂点の位置を計算
				float x0 = cos0 * kOuterRadius_;
				float z0 = -sin0 * kOuterRadius_;
				float x1 = cos1 * kOuterRadius_;
				float z1 = -sin1 * kOuterRadius_;
				// U座標を計算
				float u0 = float(i) / kRingDivide_;
				float u1 = float(i + 1) / kRingDivide_;
				// 法線ベクトルを計算
				Vector3 normal0 = { cos0, 0.0f, -sin0 };
				Vector3 normal1 = { cos1, 0.0f, -sin1 };

				// 1枚目の三角形
				cylinderModelData_.vertices_.push_back({ {x0, y0, z0, 1.0f}, {u0, v0}, normal0 });
				cylinderModelData_.vertices_.push_back({ {x1, y0, z1, 1.0f}, {u1, v0}, normal1 });
				cylinderModelData_.vertices_.push_back({ {x0, y1, z0, 1.0f}, {u0, v1}, normal0 });

				// 2枚目の三角形
				cylinderModelData_.vertices_.push_back({ {x0, y1, z0, 1.0f}, {u0, v1}, normal0 });
				cylinderModelData_.vertices_.push_back({ {x1, y0, z1, 1.0f}, {u1, v0}, normal1 });
				cylinderModelData_.vertices_.push_back({ {x1, y1, z1, 1.0f}, {u1, v1}, normal1 });
			}
		}
	}

	void ParticleManager::CreateRibbonVertices() {
		// 長さ：Z方向（-1..+1）、太さ：Y方向（-0.15..+0.15）
		// ※ X は 0 固定（板をYZ平面に置く）
		const float halfL = 1.0f;   // Z方向（基準長さ=2.0）
		const float halfH = 0.15f;  // Y方向（細い）

		ribbonModelData_.vertices_.clear();
		ribbonModelData_.vertices_.reserve(6);

		// normal は +X（YZ平面の表面）
		const Vector3 n = { 1.0f, 0.0f, 0.0f };

		// 三角形2枚
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f,  halfH,  halfL, 1.0f}, .texcoord_ = {0.0f, 0.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f,  halfH, -halfL, 1.0f}, .texcoord_ = {1.0f, 0.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f, -halfH,  halfL, 1.0f}, .texcoord_ = {0.0f, 1.0f}, .normal_ = n });

		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f, -halfH,  halfL, 1.0f}, .texcoord_ = {0.0f, 1.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f,  halfH, -halfL, 1.0f}, .texcoord_ = {1.0f, 0.0f}, .normal_ = n });
		ribbonModelData_.vertices_.push_back({ .position_ = {0.0f, -halfH, -halfL, 1.0f}, .texcoord_ = {1.0f, 1.0f}, .normal_ = n });
	}
}