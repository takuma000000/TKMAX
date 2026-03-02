#pragma once
#include <vector>
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "MyMath.h"
#include "DirectXCommon.h"
#include "CameraManager.h"

namespace TKM {

	//=============================================================
	// TrailRibbonRendererクラス
	// 弾の軌跡などに使うリボン描画クラス。
	//=============================================================
	class TrailRibbonRenderer {
	public:
		// Singletonのインスタンスを取得します。
		static TrailRibbonRenderer* GetInstance() {
			static TrailRibbonRenderer inst;
			return &inst;
		}

		/// <summary>
		/// TrailRibbonRendererの初期化を行います。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// TrailRibbonRendererの更新処理を行います。
		/// </summary>
		/// <param name="dt">前フレームからの経過時間（秒）</param>
		void Update(float dt);
		/// <summary>
		/// リボンを描画します。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		/// <param name="camera">カメラ</param>
		/// <param name="points">リボンの中心点のリスト。古い順に並べること。</param>
		/// <param name="headWidth">リボンの先端の幅</param>
		/// <param name="tailWidth">リボンの末端の幅</param>
		/// <param name="intensity">リボンの明るさ</param>
		/// <param name="color">リボンの色</param>
		/// <param name="uvTiling">UVのタイル数（リボンの長さに対するテクスチャの繰り返し回数）</param>
		/// <param name="uvScroll">UVのスクロール量（時間経過で変化させると流れるようになる）</param>
		void DrawRibbon(
			DirectXCommon* dxCommon,
			const Camera& camera,
			const std::vector<Vector3>& points,
			float headWidth,
			float tailWidth,
			float intensity,
			const Vector3& color,
			float uvTiling,
			float uvScroll
		);

	private:
		TrailRibbonRenderer() = default;
		~TrailRibbonRenderer() = default;
		TrailRibbonRenderer(const TrailRibbonRenderer&) = delete;
		TrailRibbonRenderer& operator=(const TrailRibbonRenderer&) = delete;

		// 頂点構造体
		struct Vertex {
			Vector3 pos; // 頂点の位置
			Vector2 uv; // 頂点のUV
			Vector4 color; // 頂点の色
			float age01; // 頂点の寿命割合（0=生成直後、1=寿命末期）
		};
		// 定数バッファ構造体
		struct CB {
			Matrix4x4 viewProj; // ビュー射影行列
			float time; // 経過時間
			float uvScroll; // UVのスクロール量
			float intensity; // リボンの明るさ
			float pad0; // 16byteアライメントのためのパディング
		};

		/// <summary>
		/// リボン描画のパイプラインステートとルートシグネチャを作成します。
		/// </summary>
		/// <param name="dxCommon">DirectX 共通管理クラス</param>
		void CreatePipeline_(DirectXCommon* dxCommon);
		/// <summary>
		/// リボン描画に必要な頂点バッファとインデックスバッファを、指定した最大頂点数・最大インデックス数で作成します。
		/// </summary>
		/// <param name="device">D3D12デバイス</param>
		/// <param name="maxVerts">最大頂点数</param>
		/// <param name="maxIndices">最大インデックス数</param>
		void EnsureBuffers_(ID3D12Device* device, uint32_t maxVerts, uint32_t maxIndices);
		/// <summary>
		/// リボンの中心点のリストから、カメラに対して常に面が向くようなリボンの頂点とインデックスを生成します。
		/// </summary>
		/// <param name="camera">カメラ</param>
		/// <param name="points">リボンの中心点のリスト。古い順に並べること。</param>
		/// <param name="headWidth">リボンの先端の幅</param>
		/// <param name="tailWidth">リボンの末端の幅</param>
		/// <param name="color">リボンの色</param>
		/// <param name="uvTiling">UVのタイル数（リボンの長さに対するテクスチャの繰り返し回数）</param>
		/// <param name="outVerts">生成された頂点を格納する出力パラメータ</param>
		/// <param name="outIndices">生成されたインデックスを格納する出力パラメータ</param>
		void BuildRibbonMesh_(
			const Camera& camera,
			const std::vector<Vector3>& points,
			float headWidth,
			float tailWidth,
			const Vector3& color,
			float uvTiling,
			std::vector<Vertex>& outVerts,
			std::vector<uint16_t>& outIndices
		);

		//==============================================================
		// メンバ変数
		//==============================================================
		// D3D12リソース
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;
		Microsoft::WRL::ComPtr<ID3D12Resource> vb_;
		Microsoft::WRL::ComPtr<ID3D12Resource> ib_;
		D3D12_VERTEX_BUFFER_VIEW vbView_{};
		D3D12_INDEX_BUFFER_VIEW ibView_{};
		uint32_t vbCapacity_ = 0;
		uint32_t ibCapacity_ = 0;
		// CBV（Upload）
		Microsoft::WRL::ComPtr<ID3D12Resource> cb_;
		CB* cbMapped_ = nullptr;

		float time_ = 0.0f;

		// 一時生成
		std::vector<Vertex> tmpVerts_;
		std::vector<uint16_t> tmpIndices_;
	};

} // namespace TKM