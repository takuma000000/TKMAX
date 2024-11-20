#pragma once
#include <string>
#include <d3d11.h>      // Direct3D 11の機能を使うためのヘッダ
#include <d3dcompiler.h> // シェーダーのコンパイル用
#include <DirectXMath.h> // 数学ライブラリ（DirectXMath）
#include "externals/DirectXTex/DirectXTex.h"
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

class DirectXCommon;
class SrvManager;

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

	//SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

	DirectXCommon* dxCommon_ = nullptr;
	SrvManager* srvManager_ = nullptr;

public:
	//シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	//終了
	void Finalize();

	//初期化
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

public: //テクスチャファイル読み込み関数
	//テクスチャファイルの読み込み
	void LoadTexture(const std::string& filePath);


public:
	//テクスチャ1枚分のデータ
	struct TextureData {
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHnadleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHnadleGPU;
	};

	//テクスチャデータ
	std::unordered_map<std::string, TextureData> textureDatas;

public:
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
	//テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	//メタデータを取得
	const DirectX::TexMetadata& GetMetadata(const std::string& filePath);

};

