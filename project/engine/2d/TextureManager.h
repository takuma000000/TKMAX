#pragma once
#include <string>
#include <d3d11.h>      // Direct3D 11の機能を使うためのヘッダ
#include <d3dcompiler.h> // シェーダーのコンパイル用
#include <DirectXMath.h> // 数学ライブラリ（DirectXMath）
#include "DirectXTex.h"
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

class DirectXCommon;
class SrvManager;

//=============================================================
// TextureManagerクラス
// テクスチャの読み込みと管理を行うクラス。
//=============================================================

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
	/// <summary>シングルトンインスタンスを取得します。</summary>
	static TextureManager* GetInstance();
	//終了
	/// <summary>テクスチャマネージャを終了します。</summary>
	void Finalize();

	//初期化
	/// <summary>テクスチャマネージャを初期化します。</summary>
	/// <param name="dxCommon">DirectX共通。</param>
	/// <param name="srvManager">SRVマネージャ。</param>
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

public: //テクスチャファイル読み込み関数
	//テクスチャファイルの読み込み
	/// <summary>テクスチャファイルを読み込みます。</summary>
	void LoadTexture(const std::string& filePath);


public:
	//テクスチャ1枚分のデータ
	struct TextureData {
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource;
		uint32_t srvIndex = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHnadleCPU{};
		D3D12_GPU_DESCRIPTOR_HANDLE srvHnadleGPU{};

		// デフォルトコンストラクタ（手動で定義）
		TextureData() = default;

		// ムーブコンストラクタ
		TextureData(TextureData&& other) noexcept
			: metadata(std::move(other.metadata)),
			resource(std::move(other.resource)),
			intermediateResource(std::move(other.intermediateResource)),
			srvIndex(other.srvIndex),
			srvHnadleCPU(other.srvHnadleCPU),
			srvHnadleGPU(other.srvHnadleGPU) {
			other.srvIndex = 0;
		}

		// ムーブ代入演算子
		TextureData& operator=(TextureData&& other) noexcept {
			if (this != &other) {
				metadata = std::move(other.metadata);
				resource = std::move(other.resource);
				intermediateResource = std::move(other.intermediateResource);
				srvIndex = other.srvIndex;
				srvHnadleCPU = other.srvHnadleCPU;
				srvHnadleGPU = other.srvHnadleGPU;
				other.srvIndex = 0;
			}
			return *this;
		}

	};


	//テクスチャデータ
	std::unordered_map<std::string, TextureData> textureDatas;

public:
	/// <summary>ファイルパスからSRVインデックスを取得します。</summary>
	/// <param name="filePath">テクスチャのファイルパス。</param>
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
	//テクスチャ番号からGPUハンドルを取得
	/// <summary>ファイルパスからGPUハンドルを取得します。</summary>
	/// <param name="filePath">テクスチャのファイルパス。</param>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	//メタデータを取得
	/// <summary>ファイルパスからメタデータを取得します。</summary>
	/// <param name="filePath">テクスチャのファイルパス。</param>
	const DirectX::TexMetadata& GetMetadata(const std::string& filePath);

};

