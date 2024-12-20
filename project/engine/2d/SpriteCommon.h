#pragma once
#include "DirectXCommon.h"

class SpriteCommon
{

public://メンバ関数...初期化

	static SpriteCommon* instance;
	//シングルトンインスタンスの取得
	static SpriteCommon* GetInstance();

	void Initialize(DirectXCommon* dxCommon);

	void Finalize();

	//共通描画設定
	void DrawSetCommon();

private://メンバ関数
	//ルートシグネチャの生成
	void GenerateRootSignature();
	//グラフィックスパイプラインの生成
	void GenerateGraficsPipeline();

public://getter
	DirectXCommon* GetDxCommon() const { return dxCommon_; }

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

