#include "SpriteCommon.h"
#include <cassert>
#include <format>
#include "Logger.h"
#include <iostream>
#include <cmath>
using namespace Logger;

namespace TKM {
	SpriteCommon* SpriteCommon::GetInstance() {
		static SpriteCommon instance;
		return &instance;
	}

	void SpriteCommon::Initialize(DirectXCommon* dxCommon) {
		//引数で受け取ってメンバ変数に記録する
		dxCommon_ = dxCommon;

		GenerateGraficsPipeline(); //グラフィックスパイプライン生成
	}

	void SpriteCommon::Finalize() {}

	void SpriteCommon::GenerateRootSignature() {
		descriptionRootSignature_.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; //入力アセンブラで頂点レイアウトを使う

		D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {}; //DescriptorRange作成
		descriptorRange[0].BaseShaderRegister = 0;//0から始まる
		descriptorRange[0].NumDescriptors = 1;//数は1つ
		descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVを使う
		descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;//Offsetを自動計算

		//RootParameter作成。PixelShaderのMaterialとVertexShaderのTransform
		D3D12_ROOT_PARAMETER rootParameters[4] = {};
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
		rootParameters[0].Descriptor.ShaderRegister = 0;	//レジスタ番号0とバインド
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;	//VertexShaderで使う
		rootParameters[1].Descriptor.ShaderRegister = 0;	//レジスタ番号0とバインド
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//DescriptorTableを使う
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;//Tableの中身の配列を指定
		rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);//Tableで利用する数
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	//CBVを使う
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;	//PixelShaderで使う
		rootParameters[3].Descriptor.ShaderRegister = 1;//レジスタ番号1を使う
		descriptionRootSignature_.pParameters = rootParameters;	//ルートパラメータ配列へのポインタ
		descriptionRootSignature_.NumParameters = _countof(rootParameters);	//配列の長さ

		//Samplerの設定
		D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {}; //Samplerは1つだけ
		staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; //線形補間
		staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // ← WRAP→CLAMP
		staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // ← WRAP→CLAMP
		staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // ← WRAP→CLAMP
		staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//比較しない
		staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;//ありったけのMipMapを使う
		staticSamplers[0].ShaderRegister = 0;//レジスタ番号0を使う
		staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//PixelShaderで使う
		descriptionRootSignature_.pStaticSamplers = staticSamplers;
		descriptionRootSignature_.NumStaticSamplers = _countof(staticSamplers);


		HRESULT hr;
		//シリアライズしてバイナリにする
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlog = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errorBlog = nullptr;
		hr = D3D12SerializeRootSignature(&descriptionRootSignature_, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlog, &errorBlog);
		if (FAILED(hr)) {
			Log(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
			assert(false);
		}


		//バイナリを元に生成
		hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlog->GetBufferPointer(), signatureBlog->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
		assert(SUCCEEDED(hr));

		//入力レイアウトの設定
		inputElementDescs_[0].SemanticName = "POSITION";
		inputElementDescs_[0].SemanticIndex = 0;
		inputElementDescs_[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs_[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs_[1].SemanticName = "TEXCOORD";
		inputElementDescs_[1].SemanticIndex = 0;
		inputElementDescs_[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs_[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElementDescs_[2].SemanticName = "NORMAL";
		inputElementDescs_[2].SemanticIndex = 0;
		inputElementDescs_[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs_[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

		blendDesc_.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		resterizerDesc_.CullMode = D3D12_CULL_MODE_NONE; //カリングしない
		resterizerDesc_.FillMode = D3D12_FILL_MODE_SOLID; //塗りつぶし

		//DepthStencilStateの設定
		depthStencilDesc_.DepthEnable = FALSE;                         // ← true を false に
		depthStencilDesc_.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;   // ← ALL を ZERO に
		depthStencilDesc_.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;  // ← LessEqual を AlwaysでもOK

	}

	void SpriteCommon::GenerateGraficsPipeline() {

		GenerateRootSignature(); //ルートシグネチャ生成

		HRESULT hr;

		vertexShaderBlob_ = dxCommon_->CompileShader(L"resources/shaders/Sprite.VS.hlsl", L"vs_6_0"); // 頂点シェーダ生成
		pixelShaderBlob_ = dxCommon_->CompileShader(L"resources/shaders/Sprite.PS.hlsl", L"ps_6_0"); // ピクセルシェーダ生成

		// 入力レイアウトの設定
		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
		inputLayoutDesc.pInputElementDescs = inputElementDescs_;
		inputLayoutDesc.NumElements = _countof(inputElementDescs_);

		// ==============================
		// アルファブレンド設定を追加
		// ==============================
		D3D12_RENDER_TARGET_BLEND_DESC alphaBlendDesc{};
		alphaBlendDesc.BlendEnable = TRUE;
		alphaBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		alphaBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		alphaBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		alphaBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		alphaBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		alphaBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		alphaBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

		blendDesc_.AlphaToCoverageEnable = FALSE;
		blendDesc_.IndependentBlendEnable = FALSE;
		blendDesc_.RenderTarget[0] = alphaBlendDesc;

		// ==============================
		// 残りの設定
		// ==============================
		graphicPipelineStateDesc_.pRootSignature = rootSignature_.Get();
		graphicPipelineStateDesc_.InputLayout = inputLayoutDesc;
		graphicPipelineStateDesc_.VS = { vertexShaderBlob_->GetBufferPointer(), vertexShaderBlob_->GetBufferSize() };
		graphicPipelineStateDesc_.PS = { pixelShaderBlob_->GetBufferPointer(),  pixelShaderBlob_->GetBufferSize() };
		graphicPipelineStateDesc_.BlendState = blendDesc_;
		graphicPipelineStateDesc_.RasterizerState = resterizerDesc_;
		graphicPipelineStateDesc_.NumRenderTargets = 1;
		graphicPipelineStateDesc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		graphicPipelineStateDesc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		graphicPipelineStateDesc_.SampleDesc.Count = 1;
		graphicPipelineStateDesc_.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
		graphicPipelineStateDesc_.DepthStencilState = depthStencilDesc_;
		graphicPipelineStateDesc_.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicPipelineStateDesc_, IID_PPV_ARGS(&graphicsPipelineState_));
		assert(SUCCEEDED(hr));
	}

	void SpriteCommon::DrawSetCommon() {
		dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get()); //ルートシグネチャ設定
		dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState_.Get()); //パイプラインステート設定
		dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //プリミティブ形状設定（三角形リスト）
	}
}