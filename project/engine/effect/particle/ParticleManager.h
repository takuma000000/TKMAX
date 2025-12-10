#pragma once
#include "Logger.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "engine/3d/camera/Camera.h"
#include <random>
#include <numbers>
#include "ModelTypes.h" 

//=============================================================
// ParticleManagerクラス
// パーティクルの生成・更新・描画を管理するクラス。
//=============================================================
class ParticleManager{
public:

	enum class ParticleType {
		NORMAL, // 通常パーティクル
		RING, // リングパーティクル
		CYLINDER, // シリンダーパーティクル
		RIBBON, // リボンパーティクル
	};

	//座標変換情報
	struct Transform{
		Vector3 scale;
		Vector3 rotate;
		Vector3 translate;
	};
	//軸合わせ用AABB構造体
	struct AABB {
		Vector3 min;//最小点
		Vector3 max;//最大点
	};
	//加速度構造体
	struct Acc {
		Vector3 acc;//加速度
		AABB area;//範囲
	};

	/// <summary>
	/// <summary>AABBと点の当たり判定を行います。</summary>
	/// </summary>
	/// <param name="aabb"></param>
	/// <param name="point"></param>
	/// <returns></returns>
	bool IsCollision(const AABB& aabb, const Vector3& point) {
		return (point.x >= aabb.min.x && point.x <= aabb.max.x) &&
			(point.y >= aabb.min.y && point.y <= aabb.max.y) &&
			(point.z >= aabb.min.z && point.z <= aabb.max.z);
	}
	//GPU用パーティクル構造体
	struct ParticleForGPU {
		Matrix4x4 wvp;
		Matrix4x4 World;
		Vector4 color;
	};
	//パーティクル構造体
	struct Particle {
		Transform transform;
		Vector3 velocity;
		Vector4 color;

		float lifeTime;
		float currentTime;
	};
	//パーティクルグループ構造体
	struct ParticleGroup {
		MaterialData materialData;
		std::list<Particle> particles;
		uint32_t srvIndex;
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
		uint32_t kNumInstance;
		ParticleForGPU* instancingData;
		ParticleType type;
	};


	///<summary>ParticleManagerのインスタンスを取得します。</summary>
	static ParticleManager* GetInstance();

	/// <summary>
	/// <summary>ParticleManagerの初期化を行います。</summary>
	/// </summary>
	/// <param name="dxCommon"></param>
	/// <param name="srvManager"></param>
	/// <param name="camera"></param>
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager, Camera* camera);
	/// <summary>
	/// <summary>ParticleManagerの終了処理を行います。</summary>
	/// </summary>
	void Update();
	/// <summary>
	/// <summary>ParticleManagerの描画を行います。</summary>
	/// </summary>
	void Draw();

	/// <summary>
	/// <summary>グラフィックスパイプラインを生成します。</summary>
	/// </summary>
	void CreatePipeline();
	/// <summary>
	/// <summary>ルートシグネチャを生成します。</summary>
	/// </summary>
	void CreateRootSigunature();
	/// <summary>
	/// <summary>頂点データを初期化します。</summary>
	/// </summary>
	void InitializeVD();
	/// <summary>
	/// <summary>頂点リソースを生成します。</summary>
	/// </summary>
	void CreateVR();
	/// <summary>
	/// <summary>頂点バッファビューを生成します。</summary>
	/// </summary>
	void CreateVB();
	/// <summary>
	/// <summary>パーティクルリソースを書き込みます。</summary>
	/// </summary>
	void WriteResource();

	/// <summary>
	/// <summary>パーティクルグループを作成します。</summary>
	/// </summary>
	/// <param name="name"></param>
	/// <param name="textureFilePath"></param>
	/// <param name="type"></param>
	void CreateParticleGroup(const std::string& name, const std::string& textureFilePath, ParticleType type);

	/// <summary>
	/// <summary>ビルボード行列を作成します。</summary>
	/// </summary>
	void MakeBillboardMatrix();

	/// <summary>
	/// <summary>パーティクルを放出します。</summary>
	/// </summary>
	/// <param name="name"></param>
	/// <param name="pos"></param>
	/// <param name="count"></param>
	void Emit(const std::string name, Vector3& pos, uint32_t count);

	/// <summary>
	/// <summary>パーティクルグループを取得します。</summary>
	/// </summary>
	/// <returns></returns>
	std::unordered_map<std::string, ParticleGroup> GetParticleGroups() { return particleGroups; }

	/// <summary>
	/// <summary>新しいパーティクルを作成します。</summary>
	/// </summary>
	/// <param name="randomEngine"></param>
	/// <param name="groupName"></param>
	/// <param name="translate"></param>
	/// <returns></returns>
	Particle MakeNewParticle(std::mt19937& randomEngine, const std::string& groupName, const Vector3& translate);

	/// <summary>
	/// <summary>リング頂点を作成します。</summary>
	/// </summary>
	void CreateRingVertices();
	/// <summary>
	/// <summary>シリンダー頂点を作成します。</summary>
	/// </summary>
	void CreateCylinderVertices();
	/// <summary>
	/// <summary>リボン頂点を作成します。</summary>
	/// </summary>
	void CreateRibbonVertices();

	// Setter===================================
	/// <summary>
	/// <summary>カメラをセットします。</summary>
	/// </summary>
	/// <param name="cam"></param>
	void SetCamera(Camera* cam) { camera_ = cam; }
	// =========================================

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

	ModelData ringModelData;
	Microsoft::WRL::ComPtr<ID3D12Resource> ringVertexResource = nullptr;
	D3D12_VERTEX_BUFFER_VIEW ringVertexBufferView{};

	ModelData cylinderModelData;
	Microsoft::WRL::ComPtr<ID3D12Resource> cylinderVertexResource = nullptr;
	D3D12_VERTEX_BUFFER_VIEW cylinderVertexBufferView{};

	ModelData ribbonModelData;
	Microsoft::WRL::ComPtr<ID3D12Resource> ribbonVertexResource = nullptr;
	D3D12_VERTEX_BUFFER_VIEW ribbonVertexBufferView{};

	std::unordered_map<std::string, ParticleGroup> particleGroups;

	const uint32_t kNumMaxInstance = 512;

	Matrix4x4 billboardMatrix = MyMath::MakeIdentity4x4();//単位行列

	Acc acc;

	//クライアント領域のサイズ
	const int32_t kClientWidth = 1280;
	const int32_t kClientHeight = 720;

	//Δtを定義
	const float kDeltaTime = 1.0f / 60.0f;

	uint32_t numInstance = 0;//描画すべきインスタンス数

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;

	std::mt19937 randomEngine;

	//Ring
	const uint32_t kRingDivide = 32;
	const float kOuterRadius = 1.0f;
	const float kInnerRadius = 0.2f;
	const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);

	Microsoft::WRL::ComPtr<ID3D12Resource> materialCB_;  // 永続CB
	Material* materialCPU_ = nullptr;                    // マップしたポインタ
};