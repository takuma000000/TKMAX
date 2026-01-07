#pragma once
#include <string>
#include <d3d11.h>      // Direct3D 11の機能を使うためのヘッダ
#include <d3dcompiler.h> // シェーダーのコンパイル用
#include <DirectXMath.h> // 数学ライブラリ（DirectXMath）
#include "DirectXTex.h"
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

namespace TKM{
	class DirectXCommon;
	class SrvManager;
}

//=============================================================
// TextureManagerクラス
// テクスチャの読み込みと管理を行うクラス。
//=============================================================
namespace TKM {
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

		TKM::DirectXCommon* dxCommon_ = nullptr;
		TKM::SrvManager* srvManager_ = nullptr;

	public:
		/// <summary>
		/// シングルトンインスタンスの取得。
		/// </summary>
		/// <returns></returns>
		static TextureManager* GetInstance();
		/// <summary>
		/// シングルトンインスタンスの破棄。
		/// </summary>
		void Finalize();

		/// <summary>
		/// テクスチャマネージャを初期化します。
		/// </summary>
		/// <param name="dxCommon"></param>
		/// <param name="srvManager"></param>
		void Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager);

	public: //テクスチャファイル読み込み関数
		/// <summary>
		/// テクスチャを読み込みます。
		/// </summary>
		/// <param name="filePath"></param>
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
		// Getter=====================================
		/// <summary>
		/// ファイルパスからテクスチャ番号を取得します。
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		uint32_t GetTextureIndexByFilePath(const std::string& filePath);
		/// <summary>
		/// ファイルパスからSRVのCPUハンドルを取得します。
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);
		/// <summary>
		/// ファイルパスからテクスチャメタデータを取得します。
		/// </summary>
		/// <param name="filePath"></param>
		/// <returns></returns>
		const DirectX::TexMetadata& GetMetadata(const std::string& filePath);
		// ===========================================
	};
}