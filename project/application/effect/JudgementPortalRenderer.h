#pragma once
#include <wrl.h>
#include <d3d12.h>
#include "MyMath.h"

namespace TKM {
	class DirectXCommon;
	class Camera;

	// ==========================================================
	// ジャッジメントポータル描画クラス
	// 空間に浮かぶポータルのようなエフェクトを描画します。
	// ==========================================================
	class JudgementPortalRenderer {
	public:
		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="dxCommon">DirectX共通クラス</param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// 更新
		/// </summary>
		/// <param name="dt">デルタタイム</param>
		void Update(float dt);
		/// <summary>
		/// 描画
		/// </summary>
		/// <param name="dxCommon">DirectX共通クラス</param>
		/// <param name="camera">カメラ</param>
		/// <param name="center">ポータルの中心座標</param>	
		/// <param name="charge01">チャージ量(0.0～1.0)</param>
		/// <param name="index">ポータルのインデックス（0～5）</param>
		void Draw(
			DirectXCommon* dxCommon,
			const Camera& camera,
			const Vector3& center,
			float charge01,
			int index
		);

	private:
		// 頂点構造体
		struct Vertex {
			Vector3 pos; // 頂点座標
			Vector2 uv;  // テクスチャ座標
		};
		// 定数バッファ用データ構造体
		struct ConstBuffer {
			Matrix4x4 viewProj; // ビュー射影行列

			Vector3 centerWS; // ポータルの中心座標(ワールド座標)
			float time;       // 時間

			Vector3 camRight; // カメラの右方向ベクトル
			float size;       // ポータルのサイズ

			Vector3 camUp;  // カメラの上方向ベクトル
			float charge01; // チャージ量(0.0～1.0)

			Vector3 camFwd;  // カメラの前方向ベクトル
			float intensity; // ポータルの光の強さ
		};

		/// <summary>
		/// パイプラインとルートシグネチャの作成
		/// </summary>
		void CreatePipeline_();
		/// <summary>
		/// リソースの作成
		/// </summary>
		void CreateResources_();

		// ==========================================================
		// メンバ変数
		// ==========================================================
		DirectXCommon* dxCommon_ = nullptr;

		// ==========================================================
		// ポータル描画用パイプライン
		// ==========================================================
		float time_ = 0.0f;      // 時間
		float size_ = 9.0f;      // ポータルのサイズ
		float intensity_ = 1.0f; // ポータルの光の強さ

		// =========================================================
		// DirectX12用リソース
		// =========================================================
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_; // ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_; // パイプラインステート

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_; // 頂点バッファ
		D3D12_VERTEX_BUFFER_VIEW vbView_{};                   // 頂点バッファビュー


		static constexpr int kMaxPortal_ = 6;                             // 最大ポータル数
		Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_[kMaxPortal_]; // 定数バッファ
		ConstBuffer* constMap_[kMaxPortal_] = {};                         // 定数バッファマップ
	};
}