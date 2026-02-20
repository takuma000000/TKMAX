#pragma once
#include "DirectXCommon.h"

//=============================================================
// SpriteCommonクラス
// スプライト描画の共通設定を管理するクラス。
//=============================================================

namespace TKM {
	class SpriteCommon {
	public://メンバ関数
		/// <summary>
		/// シングルトンインスタンスの取得。
		/// </summary>
		/// <returns></returns>
		static SpriteCommon* GetInstance();

		/// <summary>
		/// スプライト共通機能を初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectXCommonのインスタンス</param>
		void Initialize(DirectXCommon* dxCommon);
		/// <summary>
		/// スプライト共通機能を終了します。
		/// </summary>
		void Finalize();

		/// <summary>
		/// スプライト描画の共通設定を行います。
		/// </summary>
		void DrawSetCommon();

		// Getter=====================================
		/// <summary>
		/// DirectXCommonの取得。
		/// </summary>
		/// <returns></returns>
		DirectXCommon* GetDxCommon() { return dxCommon_; }
		/// <summary>
		/// DirectXCommonの取得（const版）。
		/// </summary>
		/// <returns></returns>
		const DirectXCommon* GetDxCommon() const { return dxCommon_; }
		// ===========================================

	private:
		/// <summary>
		/// ルートシグネチャを生成します。
		/// </summary>
		void GenerateRootSignature();
		/// <summary>
		/// グラフィックスパイプラインを生成します。
		/// </summary>
		void GenerateGraficsPipeline();

		//======================================================================
		// 外部参照
		//======================================================================
		DirectXCommon* dxCommon_ = nullptr;
		//======================================================================
		// RootSignature（ルートシグ）
		//======================================================================
		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{}; // ルートシグの説明構造体
		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr; // ルートシグのCOMポインタ
		//======================================================================
		// Shader（コンパイル済みBlob）
		//======================================================================
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_ = nullptr; // 頂点シェーダのBlob
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_ = nullptr; // ピクセルシェーダのBlob
		//======================================================================
		// InputLayout
		//======================================================================
		D3D12_INPUT_ELEMENT_DESC inputElementDescs_[3] = {}; // 入力レイアウトの説明構造体配列
		//======================================================================
		// Pipeline State（各種ステート）
		//======================================================================
		D3D12_BLEND_DESC blendDesc_{}; // ブレンドステートの説明構造体
		D3D12_RASTERIZER_DESC resterizerDesc_{}; // ラスタライザーステートの説明構造体
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{}; // デプスステンシルステートの説明構造体
		//======================================================================
		// PSO（Graphics Pipeline State）
		//======================================================================
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc_{}; // グラフィックスパイプラインステートの説明構造体
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr; // グラフィックスパイプラインステートのCOMポインタ

		///シングルトン-----------------------------------------------
		//コンストラクタ、デストラクタの隠蔽
		SpriteCommon() = default;
		~SpriteCommon() = default;
		//コピーインストラクタの封印
		SpriteCommon(SpriteCommon&) = delete;
		//コピー代入演算子の封印
		SpriteCommon& operator=(SpriteCommon&) = delete;
		///---------------------------------------------------------
	};
} //namespace TKM