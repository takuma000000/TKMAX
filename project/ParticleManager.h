#pragma once
#include "Logger.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include <random>

class ParticleManager
{
public:

	struct ParticleForGPU {
		Matrix4x4 wvp;
		Matrix4x4 World;
		Vector4 color;
	};


	struct Particle {
		Transform transform;
		Vector3 velocity;
		Vector4 color;

		float lifeTime;
		float currentTime;
	};


	struct ParticleGroup {
		MaterialData materialData;
		std::list<Particle> particles;
		uint32_t srvIndex;
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
		uint32_t kNumInstance;
		ParticleForGPU* instancingData;
	};


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

	std::unordered_map<std::string, ParticleGroup> particleGroups;
};

