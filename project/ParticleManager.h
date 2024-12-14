#pragma once
#include "Logger.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include <random>

class ParticleManager
{
public:
	static ParticleManager* GetInstance();

	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

	void Update();

	void Draw();

	//パイプライン生成
	void CreatePipeline();
	//ルートシグネイチャー
	void CreateRootSigunature();
	//VertexData
	void InitializeVD();
	//VertexResource
	void CreateVR();
	//VertexBufferView
	void CreateVB();
	//Resource
	void WriteResource();

private:
	static ParticleManager* instance;

	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = delete;
	ParticleManager& operator= (ParticleManager&) = delete;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	ModelData modelData;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
};

