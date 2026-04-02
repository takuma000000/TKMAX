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

		// デバッグ用パラメータ構造体
		struct DebugParams {
			bool enable = true;

			// 太さ
			float headWidth = 2.326f;
			float tailWidth = 1.94f;

			// 明るさ
			float intensity = 30.0f;

			// 色(真紫)
			Vector3 color = { 0.01f, 1.0f, 0.07f }; // RGB(0,4,255)

			// UV
			float uvTiling = 0.0f;   // リボン長さ方向の繰り返し
			float uvScroll = 1.421f;    // 時間で流す速度（VSで gTime*uvScroll）
		};

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

		/// <summary>
		/// ImGuiデバッグ表示。DebugParamsの編集と、描画に必要な頂点数・インデックス数の表示を行います。
		/// </summary>
		void ImGuiDebug();

		// Getter========================================
		/// <summary>
		/// デバッグ用パラメータの取得。
		/// </summary>
		/// <returns>デバッグ用パラメータ</returns>
		DebugParams& GetDebugParams() { return debug_; }
		/// <summary>
		/// デバッグ用パラメータの取得（const）。リボン描画処理内で参照されることを想定。
		/// </summary>
		/// <returns>デバッグ用パラメータ</returns>
		const DebugParams& GetDebugParams() const { return debug_; }
		// ==============================================

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

		//==============================================
		// D3D12リソース
		//==============================================
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_; // ルートシグネチャ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_; // パイプラインステートオブジェクト
		//==============================================
		// タイマー
		//==============================================
		float time_ = 0.0f; // 経過時間
		//==============================================
		// 一時生成データ
		//==============================================
		std::vector<Vertex> tmpVerts_; // 頂点生成のための一時バッファ
		std::vector<uint16_t> tmpIndices_; // インデックス生成のための一時バッファ
		//==============================================
		// フレームリング
		//==============================================
		static constexpr int kFrameRing_ = 3; // フレームリングの数。これだけバッファを用意しておけば、GPUが最大2フレーム遅れている状況でも安全に描画できる。
		int frameIndex_ = 0; // 現在のフレームリングのインデックス。0～(kFrameRing_-1)の範囲で回る。
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> drawVB_[kFrameRing_]; // 描画用頂点バッファのフレームリング。描画ごとにframeIndex_を進めていき、GPUがまだ使用中のバッファを上書きしないようにする。
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> drawIB_[kFrameRing_]; // 描画用インデックスバッファのフレームリング。描画ごとにframeIndex_を進めていき、GPUがまだ使用中のバッファを上書きしないようにする。
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> drawCB_[kFrameRing_]; // 描画用定数バッファのフレームリング。描画ごとにframeIndex_を進めていき、GPUがまだ使用中のバッファを上書きしないようにする。
		uint32_t drawCount_ = 0; // 今フレームで何本目のリボン描画か

		std::vector<Vertex*> drawVBMapped_[kFrameRing_];   // 各スロットのVBマップ先
		std::vector<uint16_t*> drawIBMapped_[kFrameRing_]; // 各スロットのIBマップ先
		std::vector<CB*> drawCBMapped_[kFrameRing_];       // 各スロットのCBマップ先

		std::vector<uint32_t> drawVBCapacity_[kFrameRing_]; // 各スロットのVB頂点容量
		std::vector<uint32_t> drawIBCapacity_[kFrameRing_]; // 各スロットのIB index容量
		//==============================================
		// デバッグ
		//==============================================
		DebugParams debug_{}; // デバッグ用パラメータ
	};
} // namespace TKM