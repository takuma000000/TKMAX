#pragma once
#include "DirectXCommon.h"

namespace TKM {

	//=============================================================
	// BarrierCommonクラス
	// バリア描画の共通設定を管理するクラス
	//=============================================================
	class BarrierCommon {
	public:
		//=============================================================
		// 生成・取得
		//=============================================================

		/// <summary>
		/// インスタンスを取得します。
		/// </summary>
		static BarrierCommon* GetInstance();

		//=============================================================
		// 初期化・描画準備
		//=============================================================

		/// <summary>
		/// 共通設定を初期化します。
		/// </summary>
		void Initialize(DirectXCommon* dxCommon);

		/// <summary>
		/// 描画前の共通設定を行います。
		/// </summary>
		void DrawSetCommon();

	private:
		//=============================================================
		// DirectX共通
		//=============================================================

		DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理

		//=============================================================
		// パイプライン生成用設定
		//=============================================================

		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};    // ルートシグネチャ設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc_{}; // パイプライン設定
		D3D12_INPUT_ELEMENT_DESC inputElementDescs_[3] = {};      // 入力レイアウト

		//=============================================================
		// GPUリソース
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;     // ルートシグネチャ
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_ = nullptr;             // 頂点シェーダ
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_ = nullptr;              // ピクセルシェーダ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr; // パイプラインステート

		//=============================================================
		// 描画ステート
		//=============================================================

		D3D12_BLEND_DESC blendDesc_{};                 // ブレンド設定
		D3D12_RASTERIZER_DESC resterizerDesc_{};       // ラスタライザ設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{};  // 深度・ステンシル設定

		//=============================================================
		// 内部生成処理
		//=============================================================

		/// <summary>
		/// ルートシグネチャを生成します。
		/// </summary>
		void GenerateRootSignature();

		/// <summary>
		/// グラフィックスパイプラインを生成します。
		/// </summary>
		void GenerateGraphicsPipeline();

	private:
		//=============================================================
		// 禁止事項
		//=============================================================

		BarrierCommon() = default;
		~BarrierCommon() = default;
		BarrierCommon(BarrierCommon&) = delete;
		BarrierCommon& operator=(BarrierCommon&) = delete;
	};

}