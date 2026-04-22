#pragma once
#define NOMINMAX
#include <vector>
#include <wrl.h>
#include <d3d12.h>

#include "DirectXCommon.h"
#include "MyMath.h"
#include "AABB.h"

namespace TKM {

	//=============================================================
	// LineRendererクラス
	// デバッグ用ライン描画を行うクラス
	//=============================================================
	class LineRenderer {
	public:
		//=============================================================
		// 色
		//=============================================================

		struct Color {
			float r, g, b, a;
		};

		//=============================================================
		// 取得・初期化
		//=============================================================

		/// <summary>
		/// インスタンスを取得します。
		/// </summary>
		static LineRenderer* GetInstance();

		/// <summary>
		/// ライン描画を初期化します。
		/// </summary>
		void Initialize(TKM::DirectXCommon* dxCommon, size_t maxLines = 1024);

		//=============================================================
		// フレーム制御
		//=============================================================

		/// <summary>
		/// フレーム開始処理を行います。
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// ラインを描画します。
		/// </summary>
		void Draw(const Matrix4x4& viewProj);

		//=============================================================
		// ライン追加
		//=============================================================

		/// <summary>
		/// ラインを追加します。
		/// </summary>
		void AddLine(const Vector3& a, const Vector3& b, const Color& c);

		/// <summary>
		/// AABBを追加します。
		/// </summary>
		void AddAABB(const Vector3& center, const Vector3& size, const Color& color);

		/// <summary>
		/// レイ交差時に色を切り替えるAABBを追加します。
		/// </summary>
		void AddAABBWithRayHighlight(
			const Vector3& center,
			const Vector3& size,
			const Vector3& rayOrigin,
			const Vector3& rayDir,
			const Color& normalColor,
			const Color& hitColor
		);

		/// <summary>
		/// 楕円体ワイヤーを追加します。
		/// </summary>
		void AddEllipsoid(
			const Vector3& center,
			const Vector3& radius,
			const Color& color,
			int segments = 32
		);

	private:
		//=============================================================
		// 生成・破棄
		//=============================================================

		LineRenderer() = default;
		~LineRenderer() = default;

		//=============================================================
		// 頂点構造体
		//=============================================================

		struct Vertex {
			Vector3 pos_;
			Color   col_;
		};

		//=============================================================
		// 共通参照
		//=============================================================

		TKM::DirectXCommon* dx_ = nullptr; // DirectX共通管理

		//=============================================================
		// 頂点データ
		//=============================================================

		std::vector<Vertex> vertices_; // 頂点配列
		size_t maxVertices_ = 0;       // 最大頂点数

		//=============================================================
		// GPUリソース
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_; // 頂点バッファ
		D3D12_VERTEX_BUFFER_VIEW vbView_{};                   // 頂点バッファビュー

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;     // パイプラインステート
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_; // ルートシグネチャ

		//=============================================================
		// 内部生成処理
		//=============================================================

		/// <summary>
		/// 頂点バッファを生成します。
		/// </summary>
		void CreateBuffer();

		/// <summary>
		/// パイプラインを生成します。
		/// </summary>
		void CreatePipeline();
	};
}