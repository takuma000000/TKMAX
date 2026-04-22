#pragma once
#include "DirectXCommon.h"

namespace TKM { class Camera; }

namespace TKM {

	//=============================================================
	// Object3dCommonクラス
	// 3Dオブジェクト描画の共通設定を管理するクラス
	//=============================================================
	class Object3dCommon {
	public:
		//=============================================================
		// 生成・取得
		//=============================================================

		/// <summary>
		/// インスタンスを取得します。
		/// </summary>
		static Object3dCommon* GetInstance();

		//=============================================================
		// 初期化・終了・描画準備
		//=============================================================

		/// <summary>
		/// 共通設定を初期化します。
		/// </summary>
		void Initialize(DirectXCommon* dxCommon);

		/// <summary>
		/// 共通設定を終了します。
		/// </summary>
		void Finalize();

		/// <summary>
		/// 描画前の共通設定を行います。
		/// </summary>
		void DrawSetCommon();

		//=============================================================
		// Getter

		/// <summary>
		/// DirectX共通管理を取得します。
		/// </summary>
		DirectXCommon* GetDxCommon() { return dxCommon_; }

		/// <summary>
		/// DirectX共通管理を取得します。
		/// </summary>
		const DirectXCommon* GetDxCommon() const { return dxCommon_; }

		/// <summary>
		/// デフォルトカメラを取得します。
		/// </summary>
		TKM::Camera* GetDefaultCamera() const { return defaultCamera_; }

		//=============================================================

		//=============================================================
		// Setter

		/// <summary>
		/// デフォルトカメラを設定します。
		/// </summary>
		void SetDefaultCamera(TKM::Camera* camera) { this->defaultCamera_ = camera; }

		//=============================================================

	private:
		//=============================================================
		// 共通参照
		//=============================================================

		DirectXCommon* dxCommon_ = nullptr; // DirectX共通管理

		//=============================================================
		// パイプライン設定
		//=============================================================

		D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature_{};         // ルートシグネチャ設定
		D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicPipelineStateDesc_{};// パイプライン設定
		D3D12_INPUT_ELEMENT_DESC inputElementDescs_[3] = {};           // 入力レイアウト

		D3D12_BLEND_DESC blendDesc_{};                // ブレンド設定
		D3D12_RASTERIZER_DESC resterizerDesc_{};      // ラスタライザ設定
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc_{}; // 深度・ステンシル設定

		//=============================================================
		// GPUリソース
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;      // ルートシグネチャ
		Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob_ = nullptr;              // 頂点シェーダ
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob_ = nullptr;               // ピクセルシェーダ
		Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr; // パイプラインステート

		//=============================================================
		// カメラ
		//=============================================================

		TKM::Camera* defaultCamera_ = nullptr; // デフォルトカメラ

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

		//=============================================================
		// 禁止事項
		//=============================================================

		Object3dCommon() = default;
		~Object3dCommon() = default;
		Object3dCommon(Object3dCommon&) = delete;
		Object3dCommon& operator=(Object3dCommon&) = delete;
	};
}