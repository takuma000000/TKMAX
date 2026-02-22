#pragma once
#include <string>
#include <d3d11.h>      // Direct3D 11の機能を使うためのヘッダ
#include <d3dcompiler.h> // シェーダーのコンパイル用
#include <DirectXMath.h> // 数学ライブラリ（DirectXMath）
#include "DirectXTex.h"
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

namespace TKM {
	class DirectXCommon;
	class SrvManager;
}

//=============================================================
// TextureManagerクラス
// テクスチャの読み込みと管理を行うクラス。
//=============================================================
namespace TKM {
	class TextureManager {

	public:
		/// <summary>
		/// シングルトンインスタンスの取得。
		/// </summary>
		/// <returns>TextureManagerのシングルトンインスタンス</returns>
		static TextureManager* GetInstance();
		/// <summary>
		/// シングルトンインスタンスの破棄。
		/// </summary>
		void Finalize();
		/// <summary>
		/// テクスチャマネージャを初期化します。
		/// </summary>
		/// <param name="dxCommon">DirectX共通管理クラスのインスタンス</param>
		/// <param name="srvManager">SRVマネージャのインスタンス</param>
		void Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager);
		/// <summary>
		/// テクスチャを読み込みます。
		/// </summary>
		/// <param name="filePath">テクスチャファイルのパス</param>
		void LoadTexture(const std::string& filePath);

		//テクスチャ1枚分のデータ
		struct TextureData {
			DirectX::TexMetadata metadata_; // テクスチャのメタデータ
			Microsoft::WRL::ComPtr<ID3D12Resource> resource_; // テクスチャリソース
			Microsoft::WRL::ComPtr <ID3D12Resource> intermediateResource_; // アップロード用の中間リソース
			uint32_t srvIndex_ = 0; // SRVのインデックス
			D3D12_CPU_DESCRIPTOR_HANDLE srvHnadleCPU_{}; // SRVのCPUハンドル
			D3D12_GPU_DESCRIPTOR_HANDLE srvHnadleGPU_{}; // SRVのGPUハンドル

			// デフォルトコンストラクタ（手動で定義）
			TextureData() = default;

			// ムーブコンストラクタ
			TextureData(TextureData&& other) noexcept
				: metadata_(std::move(other.metadata_)),
				resource_(std::move(other.resource_)),
				intermediateResource_(std::move(other.intermediateResource_)),
				srvIndex_(other.srvIndex_),
				srvHnadleCPU_(other.srvHnadleCPU_),
				srvHnadleGPU_(other.srvHnadleGPU_) {
				other.srvIndex_ = 0;
			}

			// ムーブ代入演算子
			TextureData& operator=(TextureData&& other) noexcept {
				if (this != &other) { // 自己代入チェック
					metadata_ = std::move(other.metadata_); // メタデータはムーブできないため、コピーする
					resource_ = std::move(other.resource_); // ComPtrはムーブセマンティクスをサポートしているため、ムーブする
					intermediateResource_ = std::move(other.intermediateResource_); // ComPtrはムーブセマンティクスをサポートしているため、ムーブする
					srvIndex_ = other.srvIndex_; // SRVインデックスはムーブできないため、コピーする
					srvHnadleCPU_ = other.srvHnadleCPU_; // CPUハンドルはムーブできないため、コピーする
					srvHnadleGPU_ = other.srvHnadleGPU_; // GPUハンドルはムーブできないため、コピーする
					other.srvIndex_ = 0; // ムーブ元のSRVインデックスをリセット
				}
				return *this;
			}

		};

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

	private:
		static TextureManager* instance;

		///シングルトン-----------------------------------------------

		//コンストラクタ、デストラクタの隠蔽
		TextureManager() = default;
		~TextureManager() = default;
		//コピーインストラクタの封印
		TextureManager(TextureManager&) = delete;
		//コピー代入演算子の封印
		TextureManager& operator=(TextureManager&) = delete;

		///---------------------------------------------------------

		//======================================================================
		// SRV管理
		//======================================================================
		//SRVインデックスの開始番号
		static uint32_t kSRVIndexTop;
		//======================================================================
		// 外部参照
		//======================================================================
		TKM::DirectXCommon* dxCommon_ = nullptr;
		TKM::SrvManager* srvManager_ = nullptr;
		//======================================================================
		// テクスチャデータ管理
		//======================================================================
		//テクスチャデータ
		std::unordered_map<std::string, TextureData> textureDatas_;
	};
}