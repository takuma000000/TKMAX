#pragma once
#include "DirectXCommon.h"

class Camera;

//=============================================================
// Object3dCommonクラス
// 3Dオブジェクト描画の共通設定（パイプライン等）を管理するクラス。
//=============================================================
class Object3dCommon{
public://メンバ関数
	static Object3dCommon* instance;
	//シングルトンインスタンスの取得
	static Object3dCommon* GetInstance();

	/// <summary>
	/// 3Dオブジェクト共通機能を初期化します。
	/// </summary>
	/// <param name="dxCommon"></param>
	void Initialize(DirectXCommon* dxCommon);
	/// <summary>
	/// 3Dオブジェクト共通機能を終了処理します。
	/// </summary>
	void Finalize();	
	/// <summary>
	/// 3Dオブジェクト描画の共通設定を行います。
	/// </summary>
	void DrawSetCommon();

	// Getter===================================
	/// <summary>
	/// DirectXCommonのゲッター。
	/// </summary>
	/// <returns></returns>
	DirectXCommon* GetDxCommon() const { return dxCommon_; }
	/// <summary>
	/// デフォルトカメラのゲッター。
	/// </summary>
	/// <returns></returns>
	Camera* GetDefaultCamera() const { return defaultCamera_; }
	// =========================================
	// Setter===================================
	/// <summary>
	/// デフォルトカメラのセッター。
	/// </summary>
	/// <param name="camera"></param>
	void SetDefaultCamera(Camera* camera) { this->defaultCamera_ = camera; }
	// =========================================
private://メンバ変数
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

	//デフォルトカメラ
	Camera* defaultCamera_ = nullptr;

	/// <summary>
	/// ルートシグネチャを生成します。
	/// </summary>
	void GenerateRootSignature();
	/// <summary>
	/// グラフィックスパイプラインを生成します。
	/// </summary>
	void GenerateGraficsPipeline();

	///シングルトン-----------------------------------------------
	//コンストラクタ、デストラクタの隠蔽
	Object3dCommon() = default;
	~Object3dCommon() = default;
	//コピーインストラクタの封印
	Object3dCommon(Object3dCommon&) = delete;
	//コピー代入演算子の封印
	Object3dCommon& operator=(Object3dCommon&) = delete;
	///---------------------------------------------------------
};