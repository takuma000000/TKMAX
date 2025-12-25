#include "TextureManager.h"
#include "DirectXCommon.h"
#include "StringUtility.h"
#include "SrvManager.h"

using namespace StringUtility;

namespace TKM {
	TextureManager* TextureManager::instance = nullptr;
	//ImGuiで0番を使用するため、1番から使用
	uint32_t TextureManager::kSRVIndexTop = 1;

	void TextureManager::Initialize(TKM::DirectXCommon* dxCommon, TKM::SrvManager* srvManager) {
		dxCommon_ = dxCommon;
		srvManager_ = srvManager;

		//SRVの数と同数
		textureDatas.reserve(TKM::SrvManager::kMaxSRVCount);
	}

	void TextureManager::LoadTexture(const std::string& filePath) {

		// 既に読み込み済みならスキップ
		if (textureDatas.contains(filePath)) {
			return;
		}

		// テクスチャ上限チェック
		assert(srvManager_->Available());

		// 画像読み込み
		DirectX::ScratchImage image{};
		std::wstring filePathW = ConvertString(filePath);
		HRESULT hr = S_FALSE;

		// 拡張子がDDSかで分岐
		if (filePath.ends_with(".dds")) { // DDSファイル
			hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
		} else { // WIC対応ファイル
			hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
		}
		assert(SUCCEEDED(hr));

		// MipMap生成用
		DirectX::ScratchImage mipImages{};

		// 圧縮フォーマットなら一旦解凍
		if (DirectX::IsCompressed(image.GetMetadata().format)) { // 圧縮フォーマット
			DirectX::ScratchImage decompressed{}; // 解凍後画像

			// 解凍
			hr = DirectX::Decompress(
				image.GetImages(),
				image.GetImageCount(),
				image.GetMetadata(),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				decompressed);
			assert(SUCCEEDED(hr));

			// MipMap生成
			hr = DirectX::GenerateMipMaps(
				decompressed.GetImages(),
				decompressed.GetImageCount(),
				decompressed.GetMetadata(),
				DirectX::TEX_FILTER_SRGB,
				0,
				mipImages);
			assert(SUCCEEDED(hr));
		} else { // 非圧縮フォーマット
			// 非圧縮ならそのまま
			hr = DirectX::GenerateMipMaps(
				image.GetImages(),
				image.GetImageCount(),
				image.GetMetadata(),
				DirectX::TEX_FILTER_SRGB,
				0,
				mipImages);
			assert(SUCCEEDED(hr));
		}

		// テクスチャ情報作成
		TextureData textureData{};

		// メタデータ取得
		textureData.metadata = mipImages.GetMetadata();
		textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
		textureData.srvIndex = srvManager_->Allocate();
		textureData.srvHnadleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
		textureData.srvHnadleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

		// SRV設定
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = textureData.metadata.format;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (textureData.metadata.IsCubemap()) { // キューブマップ
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.MipLevels = UINT_MAX;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		} else { // 2Dテクスチャ
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);
		}

		// SRV生成
		dxCommon_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHnadleCPU);
		// テクスチャデータ転送
		textureData.intermediateResource = dxCommon_->UploadTextureData(textureData.resource.Get(), mipImages);

		// 登録
		textureDatas.emplace(filePath, std::move(textureData));
	}

	uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath) {
		//読み込み済みテクスチャを検索
		if (textureDatas.contains(filePath)) {
			//読み込み済みなら要素番号を返す
			uint32_t textureIndex = textureDatas[filePath].srvIndex;
			return textureIndex;
		}

		assert(0);
		return 0;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath) {
		//テクスチャ枚数上限チェック
		assert(srvManager_->Available());

		TextureData& textureData = textureDatas[filePath]; //テクスチャデータの参照を取得
		return textureData.srvHnadleGPU;
	}

	const DirectX::TexMetadata& TextureManager::GetMetadata(const std::string& filePath) {
		//テクスチャ枚数上限チェック
		assert(srvManager_->Available());

		//テクスチャデータの参照を取得
		TextureData& textureData = textureDatas[filePath];
		return textureData.metadata;
	}

	TextureManager* TextureManager::GetInstance() {
		static TextureManager instance_;
		return &instance_;
	}

	void TextureManager::Finalize() {

	}
}