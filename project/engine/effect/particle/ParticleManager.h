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
// パーティクルの生成・更新・描画を管理するクラス
//=============================================================
namespace TKM {
	class ParticleManager {
	public:
		//=============================================================
		// 列挙型
		//=============================================================

		enum class ParticleType {
			NORMAL,   // 通常パーティクル
			RING,     // リングパーティクル
			CYLINDER, // シリンダーパーティクル
			RIBBON,   // リボンパーティクル
		};

		enum class LoadLevel {
			Low,      // 負荷低
			Medium,   // 負荷中
			High,     // 負荷高
			Critical, // 負荷非常に高
		};

		//=============================================================
		// 型エイリアス
		//=============================================================

		using Transform = TKM::Transform; // Transformクラスのエイリアス

		//=============================================================
		// 構造体
		//=============================================================

		// 軸合わせ用AABB構造体
		struct AABB {
			Vector3 min_; // 最小点
			Vector3 max_; // 最大点
		};

		// 加速度構造体
		struct Acc {
			Vector3 acc_; // 加速度
			AABB area_;   // 範囲
		};

		// GPU用パーティクル構造体
		struct ParticleForGPU {
			Matrix4x4 wvp_;
			Matrix4x4 World_;
			Vector4 color_;
		};

		// パーティクル構造体
		struct Particle {
			Transform transform_;
			Vector3 velocity_;
			Vector4 color_;
			float lifeTime_;
			float currentTime_;
		};

		// パーティクルグループ構造体
		struct ParticleGroup {
			MaterialData materialData_;
			std::list<Particle> particles_;
			uint32_t srvIndex_;
			Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource_;
			uint32_t kNumInstance_;
			ParticleForGPU* instancingData_;
			ParticleType type_;
		};

		//=============================================================
		// 取得
		//=============================================================

		/// <summary>
		/// ParticleManagerのインスタンスを取得します。
		/// </summary>
		static ParticleManager* GetInstance();

		//=============================================================
		// 初期化・更新・描画
		//=============================================================

		/// <summary>
		/// ParticleManagerを初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理</param>
		/// <param name="srvManager">SRV管理</param>
		/// <param name="camera">使用カメラ</param>
		void Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager, TKM::Camera* camera);

		/// <summary>
		/// ParticleManagerを更新します。
		/// </summary>
		/// <param name="dt">経過時間</param>
		void Update(float dt);

		/// <summary>
		/// ParticleManagerを描画します。
		/// </summary>
		void Draw();

		//=============================================================
		// パイプライン・描画準備
		//=============================================================

		/// <summary>
		/// グラフィックスパイプラインを生成します。
		/// </summary>
		void CreatePipeline();

		/// <summary>
		/// ルートシグネチャを生成します。
		/// </summary>
		void CreateRootSignature();

		/// <summary>
		/// 頂点データを初期化します。
		/// </summary>
		void InitializeVD();

		/// <summary>
		/// 頂点リソースを生成します。
		/// </summary>
		void CreateVR();

		/// <summary>
		/// 頂点バッファビューを生成します。
		/// </summary>
		void CreateVB();

		/// <summary>
		/// パーティクルリソースを書き込みます。
		/// </summary>
		void WriteResource();

		//=============================================================
		// グループ管理
		//=============================================================

		/// <summary>
		/// すべてのパーティクルグループの生存粒子を消します。
		/// </summary>
		void ClearAllGroups();

		/// <summary>
		/// パーティクルグループを作成します。
		/// </summary>
		/// <param name="name">グループ名</param>
		/// <param name="textureFilePath">テクスチャパス</param>
		/// <param name="type">パーティクル種別</param>
		void CreateParticleGroup(const std::string& name, const std::string& textureFilePath, ParticleType type);

		/// <summary>
		/// パーティクルグループをクリアします。
		/// </summary>
		/// <param name="name">グループ名</param>
		void ClearGroup(const std::string& name);

		/// <summary>
		/// パーティクルグループを取得します。
		/// </summary>
		/// <returns>パーティクルグループ一覧</returns>
		std::unordered_map<std::string, ParticleGroup> GetParticleGroups() { return particleGroups_; }

		//=============================================================
		// 行列生成
		//=============================================================

		/// <summary>
		/// ビルボード行列を作成します。
		/// </summary>
		void MakeBillboardMatrix();

		//=============================================================
		// 放出・生成
		//=============================================================

		/// <summary>
		/// パーティクルを放出します。
		/// </summary>
		/// <param name="name">グループ名</param>
		/// <param name="pos">放出位置</param>
		/// <param name="count">放出数</param>
		void Emit(const std::string name, const Vector3& pos, uint32_t count);

		/// <summary>
		/// パーティクルを放出します。
		/// </summary>
		/// <param name="name">グループ名</param>
		/// <param name="tr">Transform</param>
		/// <param name="color">色</param>
		/// <param name="count">放出数</param>
		void EmitWithTransform(const std::string& name, const Transform& tr, const Vector4& color, uint32_t count);

		/// <summary>
		/// 新しいパーティクルを作成します。
		/// </summary>
		/// <param name="randomEngine">乱数生成器</param>
		/// <param name="groupName">グループ名</param>
		/// <param name="translate">生成位置</param>
		/// <returns>生成したパーティクル</returns>
		Particle MakeNewParticle(std::mt19937& randomEngine, const std::string& groupName, const Vector3& translate);

		//=============================================================
		// 形状頂点生成
		//=============================================================

		/// <summary>
		/// リング頂点を作成します。
		/// </summary>
		void CreateRingVertices();

		/// <summary>
		/// シリンダー頂点を作成します。
		/// </summary>
		void CreateCylinderVertices();

		/// <summary>
		/// リボン頂点を作成します。
		/// </summary>
		void CreateRibbonVertices();

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// カメラを設定します。
		/// </summary>
		/// <param name="cam">使用カメラ</param>
		void SetCamera(TKM::Camera* cam) { camera_ = cam; }

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// アクティブなパーティクル数を取得します。
		/// </summary>
		/// <returns>アクティブな総パーティクル数</returns>
		size_t GetActiveParticleCount() const;

		/// <summary>
		/// 現在の負荷レベルを取得します。
		/// </summary>
		/// <returns>現在の負荷レベル</returns>
		LoadLevel GetLoadLevel() const;

		/// <summary>
		/// 放出数を負荷レベルに応じてスケーリングして取得します。
		/// </summary>
		/// <param name="baseCount">基準放出数</param>
		/// <param name="isPriorityEffect">優先演出かどうか</param>
		/// <returns>補正後の放出数</returns>
		uint32_t GetEmitCountScaled(uint32_t baseCount, bool isPriorityEffect = false) const;

	private:
		//=============================================================
		// 禁止事項
		//=============================================================

		ParticleManager() = default;
		~ParticleManager() = default;
		ParticleManager(ParticleManager&) = delete;
		ParticleManager& operator=(ParticleManager&) = delete;

		//=============================================================
		// 共通参照
		//=============================================================

		TKM::DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理
		TKM::SrvManager* srvManager_ = nullptr;  // SRV管理
		TKM::Camera* camera_ = nullptr;          // 使用カメラ

		//=============================================================
		// パイプライン
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;        // ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr; // パイプラインステート

		//=============================================================
		// モデル・頂点リソース
		//=============================================================

		ModelData modelData_;                                            // 通常モデルデータ
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;          // 通常頂点リソース
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};                    // 通常頂点バッファビュー

		ModelData ringModelData_;                                        // リングモデルデータ
		Microsoft::WRL::ComPtr<ID3D12Resource> ringVertexResource_ = nullptr; // リング頂点リソース
		D3D12_VERTEX_BUFFER_VIEW ringVertexBufferView_{};                // リング頂点バッファビュー

		ModelData cylinderModelData_;                                    // シリンダーモデルデータ
		Microsoft::WRL::ComPtr<ID3D12Resource> cylinderVertexResource_ = nullptr; // シリンダー頂点リソース
		D3D12_VERTEX_BUFFER_VIEW cylinderVertexBufferView_{};            // シリンダー頂点バッファビュー

		ModelData ribbonModelData_;                                      // リボンモデルデータ
		Microsoft::WRL::ComPtr<ID3D12Resource> ribbonVertexResource_ = nullptr; // リボン頂点リソース
		D3D12_VERTEX_BUFFER_VIEW ribbonVertexBufferView_{};              // リボン頂点バッファビュー

		//=============================================================
		// パーティクルデータ
		//=============================================================

		std::unordered_map<std::string, ParticleGroup> particleGroups_; // パーティクルグループ一覧

		const uint32_t kNumMaxInstance_ = 512; // 最大インスタンス数

		Matrix4x4 billboardMatrix_ = MyMath::MakeIdentity4x4(); // ビルボード行列

		Acc acc; // 加速度設定

		//=============================================================
		// 基本定数
		//=============================================================

		const int32_t kClientWidth_ = 1280;   // クライアント幅
		const int32_t kClientHeight_ = 720;   // クライアント高さ
		const float kDeltaTime_ = 1.0f / 60.0f; // Δt

		uint32_t numInstance_ = 0; // 描画するインスタンス数

		//=============================================================
		// マテリアル・乱数
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_; // マテリアルリソース
		std::mt19937 randomEngine_;                               // 乱数生成器

		//=============================================================
		// 負荷管理
		//=============================================================

		size_t loadThresholdMedium_ = 400;   // 負荷中の閾値
		size_t loadThresholdHigh_ = 550;     // 負荷高の閾値
		size_t loadThresholdCritical_ = 650; // 負荷非常に高の閾値

		//=============================================================
		// Ring設定
		//=============================================================

		const uint32_t kRingDivide_ = 32; // 分割数
		const float kOuterRadius_ = 1.0f; // 外側半径
		const float kInnerRadius_ = 0.2f; // 内側半径
		const float radianPerDivide_ = 2.0f * std::numbers::pi_v<float> / float(kRingDivide_); // 1分割あたりのラジアン

		//=============================================================
		// マテリアルCB
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12Resource> materialCB_; // 永続CB
		Material* materialCPU_ = nullptr;                   // マップ済みポインタ

		//=============================================================
		// 当たり判定
		//=============================================================

		/// <summary>
		/// AABBと点の当たり判定を行います。
		/// </summary>
		/// <param name="aabb">判定対象AABB</param>
		/// <param name="point">判定対象点</param>
		/// <returns>内包していたらtrue</returns>
		bool IsCollision(const AABB& aabb, const Vector3& point) {
			return (point.x >= aabb.min_.x && point.x <= aabb.max_.x) &&
				(point.y >= aabb.min_.y && point.y <= aabb.max_.y) &&
				(point.z >= aabb.min_.z && point.z <= aabb.max_.z);
		}
	};
}