#pragma once
#include "Logger.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include <random>

class ParticleManager
{
public:

	struct AABB {
		Vector3 min;//最小点
		Vector3 max;//最大点
	};

	struct Acc {
		Vector3 acc;//加速度
		AABB area;//範囲
	};

	bool IsCollision(const AABB& aabb, const Vector3& point) {
		return (point.x >= aabb.min.x && point.x <= aabb.max.x) &&
			(point.y >= aabb.min.y && point.y <= aabb.max.y) &&
			(point.z >= aabb.min.z && point.z <= aabb.max.z);
	}

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

	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, Camera* camera);

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

	//パーティクルグループの作成
	void CreateParticleGroup(const std::string name, const std::string textureFilePath);

	//billboardマトリクスの計算
	void MakeBillboardMatrix();

private:
	static ParticleManager* instance;

	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = delete;
	ParticleManager& operator= (ParticleManager&) = delete;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;
	Camera* camera_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	ModelData modelData;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	std::unordered_map<std::string, ParticleGroup> particleGroups;

	const uint32_t kNumMaxInstance = 100;

	Matrix4x4 billboardMatrix = MyMath::MakeIdentity4x4();//単位行列

	Acc acc;

	//クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//Δtを定義
	const float kDeltaTime = 1.0f / 60.0f;

	uint32_t numInstance = 0;//描画すべきインスタンス数
};

