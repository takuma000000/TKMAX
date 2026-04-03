#pragma once
#include "Logger.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "Camera.h"
#include <random>
#include <numbers>
#include "ModelTypes.h"
#include "Transform.h"

//=============================================================
// ParticleManagerクラス
// パーティクルの生成・更新・描画を管理するクラス。
//=============================================================
namespace TKM {
	class ParticleManager {
	public:
		enum class ParticleType {
			NORMAL, // 通常パーティクル
			RING, // リングパーティクル
			CYLINDER, // シリンダーパーティクル
			RIBBON, // リボンパーティクル
		};

		enum class LoadLevel {
			Low, // 負荷低
			Medium, // 負荷中
			High, // 負荷高
			Critical, // 負荷非常に高
		};

		// Transformクラスのエイリアス
		using Transform = TKM::Transform;

		//軸合わせ用AABB構造体
		struct AABB {
			Vector3 min_;//最小点
			Vector3 max_;//最大点
		};
		//加速度構造体
		struct Acc {
			Vector3 acc_;//加速度
			AABB area_;//範囲
		};
		//GPU用パーティクル構造体
		struct ParticleForGPU {
			Matrix4x4 wvp_;
			Matrix4x4 World_;
			Vector4 color_;
		};
		//パーティクル構造体
		struct Particle {
			Transform transform_;
			Vector3 velocity_;
			Vector4 color_;
			float lifeTime_;
			float currentTime_;
		};
		//パーティクルグループ構造体
		struct ParticleGroup {
			MaterialData materialData_;
			std::list<Particle> particles_;
			uint32_t srvIndex_;
			Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
			uint32_t kNumInstance_;
			ParticleForGPU* instancingData_;
			ParticleType type_;
		};


		///<summary>ParticleManagerのインスタンスを取得します。</summary>
		static ParticleManager* GetInstance();

		/// <summary>
		/// <summary>ParticleManagerの初期化を行います。</summary>
		/// </summary>
		/// <param name="dxCommon"></param>
		/// <param name="srvManager"></param>
		/// <param name="camera"></param>
		void Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager, TKM::Camera* camera);
		/// <summary>
		/// <summary>ParticleManagerの終了処理を行います。</summary>
		/// </summary>
		void Update(float dt);
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
		void CreateRootSignature();
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
		/// すべてのパーティクルグループの生存粒子を消します。
		/// </summary>
		void ClearAllGroups();

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
		void Emit(const std::string name, const Vector3& pos, uint32_t count);

		/// <summary>
		/// <summary>パーティクルを放出します（Transform指定）。</summary>
		/// </summary>
		/// <param name="name"></param>
		/// <param name="tr"></param>
		/// <param name="color"></param>
		/// <param name="count"></param>
		void EmitWithTransform(const std::string& name, const Transform& tr, const Vector4& color, uint32_t count);
		/// <summary>
		/// <summary>パーティクルグループをクリアします。</summary>
		/// </summary>
		/// <param name="name"></param>
		void ClearGroup(const std::string& name);

		/// <summary>
		/// <summary>パーティクルグループを取得します。</summary>
		/// </summary>
		/// <returns></returns>
		std::unordered_map<std::string, ParticleGroup> GetParticleGroups() { return particleGroups_; }

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
		void SetCamera(TKM::Camera* cam) { camera_ = cam; }
		// =========================================
		// Getter===================================
		/// <summary>
		/// <summary>アクティブなパーティクルの数を取得します。</summary>
		/// </summary>
		/// <returns></returns>
		size_t GetActiveParticleCount() const;
		/// <summary>
		/// <summary>現在の負荷レベルを取得します。</summary>
		/// </summary>
		/// <returns></returns>
		LoadLevel GetLoadLevel() const;
		/// <summary>
		/// <summary>放出数を負荷レベルに応じてスケーリングして取得します。</summary>
		/// </summary>
		/// <param name="baseCount"></param>
		/// <param name="isPriorityEffect"></param>
		/// <returns></returns>
		uint32_t GetEmitCountScaled(uint32_t baseCount, bool isPriorityEffect = false) const;
		// =========================================

	private:

		ParticleManager() = default;
		~ParticleManager() = default;
		ParticleManager(ParticleManager&) = delete;
		ParticleManager& operator= (ParticleManager&) = delete;
		TKM::DirectXCommon* dxCommon_ = nullptr;
		TKM::SrvManager* srvManager_ = nullptr;
		TKM::Camera* camera_ = nullptr;

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

		ModelData modelData_;
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

		ModelData ringModelData_;
		Microsoft::WRL::ComPtr<ID3D12Resource> ringVertexResource_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW ringVertexBufferView_{};

		ModelData cylinderModelData_;
		Microsoft::WRL::ComPtr<ID3D12Resource> cylinderVertexResource_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW cylinderVertexBufferView_{};

		ModelData ribbonModelData_;
		Microsoft::WRL::ComPtr<ID3D12Resource> ribbonVertexResource_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW ribbonVertexBufferView_{};

		std::unordered_map<std::string, ParticleGroup> particleGroups_;

		const uint32_t kNumMaxInstance_ = 512;

		Matrix4x4 billboardMatrix_ = MyMath::MakeIdentity4x4();//単位行列

		Acc acc;

		//クライアント領域のサイズ
		const int32_t kClientWidth_ = 1280;
		const int32_t kClientHeight_ = 720;

		//Δtを定義
		const float kDeltaTime_ = 1.0f / 60.0f;

		uint32_t numInstance_ = 0;//描画すべきインスタンス数

		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

		std::mt19937 randomEngine_;

		// 負荷レベルの閾値
		size_t loadThresholdMedium_ = 400; // 負荷中の閾値
		size_t loadThresholdHigh_ = 550; // 負荷高の閾値
		size_t loadThresholdCritical_ = 650; // 負荷非常に高の閾値

		//Ring
		const uint32_t kRingDivide_ = 32;
		const float kOuterRadius_ = 1.0f;
		const float kInnerRadius_ = 0.2f;
		const float radianPerDivide_ = 2.0f * std::numbers::pi_v<float> / float(kRingDivide_);

		Microsoft::WRL::ComPtr<ID3D12Resource> materialCB_;  // 永続CB
		Material* materialCPU_ = nullptr;                    // マップしたポインタ

		/// <summary>
		/// <summary>AABBと点の当たり判定を行います。</summary>
		/// </summary>
		/// <param name="aabb"></param>
		/// <param name="point"></param>
		/// <returns></returns>
		bool IsCollision(const AABB& aabb, const Vector3& point) {
			return (point.x >= aabb.min_.x && point.x <= aabb.max_.x) &&
				(point.y >= aabb.min_.y && point.y <= aabb.max_.y) &&
				(point.z >= aabb.min_.z && point.z <= aabb.max_.z);
		}
	};
}