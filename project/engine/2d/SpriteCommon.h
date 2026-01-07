#pragma once
#include "DirectXCommon.h"

//=============================================================
// SpriteCommonクラス
// スプライト描画の共通設定を管理するクラス。
//=============================================================

namespace TKM {
	class SpriteCommon {

	public://メンバ関数...初期化
		/// <summary>
		/// シングルトンインスタンス。
		/// </summary>
		static SpriteCommon* instance;
		/// <summary>
		/// シングルトンインスタンスの取得。
		/// </summary>
		/// <returns></returns>
		static SpriteCommon* GetInstance();

		/// <summary>
		/// スプライト共通機能を初期化します。
		/// </summary>
		/// <param name="dxCommon"></param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// スプライト共通機能を終了します。
		/// </summary>
		void Finalize();

		/// <summary>
		/// スプライト描画の共通設定を行います。
		/// </summary>
		void DrawSetCommon();

	private://メンバ関数
		/// <summary>
		/// ルートシグネチャを生成します。
		/// </summary>
		void GenerateRootSignature();
		/// <summary>
		/// グラフィックスパイプラインを生成します。
		/// </summary>
		void GenerateGraficsPipeline();

	public:
		
		// Getter=====================================
		/// <summary>
		/// DirectXCommonの取得。
		/// </summary>
		/// <returns></returns>
		DirectXCommon* GetDxCommon() const { return dxCommon_; }
		// ===========================================

	private:
		DirectXCommon* dxCommon_;
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc{};
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = nullptr;
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = nullptr;
		D3D12_BLEND_DESC blendDesc{};
		D3D12_RASTERIZER_DESC resterizerDesc{};
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;


		////シングルトン-----------------------------------------------

		//コンストラクタ、デストラクタの隠蔽
		SpriteCommon() = default;
		~SpriteCommon() = default;
		//コピーインストラクタの封印
		SpriteCommon(SpriteCommon&) = delete;
		//コピー代入演算子の封印
		SpriteCommon& operator=(SpriteCommon&) = delete;

		////---------------------------------------------------------
	};
} //namespace TKM