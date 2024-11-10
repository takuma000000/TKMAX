#pragma once
#include <string>
#include <d3d11.h>      // Direct3D 11の機能を使うためのヘッダ
#include <d3dcompiler.h> // シェーダーのコンパイル用
#include <DirectXMath.h> // 数学ライブラリ（DirectXMath）
#include "externals/DirectXTex/DirectXTex.h"
#include <wrl.h>
#include <d3d12.h>

class DirectXCommon;

class TextureManager {

private:
	static TextureManager* instance;

	////シングルトン-----------------------------------------------

	//コンストラクタ、デストラクタの隠蔽
	TextureManager() = default;
	~TextureManager() = default;
	//コピーインストラクタの封印
	TextureManager(TextureManager&) = delete;
	//コピー代入演算子の封印
	TextureManager& operator=(TextureManager&) = delete;

	////---------------------------------------------------------

	DirectXCommon* dxCommon_ = nullptr;

	//DirectX12デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;

public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	//終了
	void Finalize();

	//初期化
	void Initialize();

public: //テクスチャファイル読み込み関数
	//テクスチャファイルの読み込み
	void LoadTexture(const std::string& filePath);

	//テクスチャリソースの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

public:
	//テクスチャ1枚分のデータ
	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHnadleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHnadleGPU;
	};

	//テクスチャデータ
	std::vector<TextureData> textureDatas;

public:
	//SRVの指定番号のCPUディスクリプタハンドルを取得する
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);
	//SRVの指定番号のGPUディスクリプタハンドルを取得する
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

public: //色々な関数
	//指定番号のCPUディスクリプタハンドルを取得する
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	//指定番号のGPUディスクリプタハンドルを取得する
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

};

