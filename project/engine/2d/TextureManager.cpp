#include "TextureManager.h"
#include "DirectXCommon.h"
#include "StringUtility.h"
#include "SrvManager.h"

using namespace StringUtility;

TextureManager* TextureManager::instance = nullptr;
//ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

void TextureManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager) {
	dxCommon_ = dxCommon;
	srvManager_ = srvManager;

	//SRVの数と同数
	textureDatas.reserve(SrvManager::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath) {

	// 既に読み込み済みならスキップ
	if (textureDatas.contains(filePath)) {
		return;
	}

	// テクスチャ上限チェック
	assert(srvManager_->Available());

	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = S_FALSE;

	// 拡張子がDDSかで分岐
	if (filePath.ends_with(".dds")) {
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	}
	else {
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	assert(SUCCEEDED(hr));

	// MipMap生成用
	DirectX::ScratchImage mipImages{};

	// 圧縮フォーマットなら一旦解凍
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		DirectX::ScratchImage decompressed{};
		hr = DirectX::Decompress(
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			DXGI_FORMAT_R8G8B8A8_UNORM,
			decompressed);
		assert(SUCCEEDED(hr));

		hr = DirectX::GenerateMipMaps(
			decompressed.GetImages(),
			decompressed.GetImageCount(),
			decompressed.GetMetadata(),
			DirectX::TEX_FILTER_SRGB,
			0,
			mipImages);
		assert(SUCCEEDED(hr));
	}
	else {
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
	

	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

	textureData.srvIndex = srvManager_->Allocate();
	textureData.srvHnadleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHnadleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

	// SRV設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (textureData.metadata.IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);
	}

	// SRV生成
	dxCommon_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHnadleCPU);

	textureData.intermediateResource = dxCommon_->UploadTextureData(textureData.resource.Get(), mipImages);

	// 登録
	textureDatas.emplace(filePath, std::move(textureData));
}



//Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata)
//{
//	//metadataを基にResourceの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = UINT(metadata.width);//Textureの幅
//	resourceDesc.Height = UINT(metadata.height);//Textureの高さ
//	resourceDesc.MipLevels = UINT(metadata.mipLevels);//mipmapの数
//	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);//奥行き or 配列Textureの配列数
//	resourceDesc.Format = metadata.format;//TextureのFormat
//	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);//Textureの次元数。
//
//	//利用するHeapの作成。非常に特殊な運用。
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;//細かい設定を行う
//	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//WriteBackポリシーでCPUアクセス可能
//	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//プロセッサの近くに配置
//
//	//Resourceの生成
//	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
//	HRESULT hr = dxCommon_->GetDevice()->CreateCommittedResource(
//		&heapProperties,//Heapの設定
//		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
//		&resourceDesc,//Resouceの設定
//		D3D12_RESOURCE_STATE_GENERIC_READ,//初回のResourceState。	Textureは基本読むだけ
//		nullptr,//Clear最適値。使わないのでnullptr
//		IID_PPV_ARGS(&resource)//作成するResourceポインタへのポインタ
//	);
//	assert(SUCCEEDED(hr));
//	return resource;
//}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{

	//読み込み済みテクスチャを検索
	if (textureDatas.contains(filePath)) {
		//読み込み済みなら要素番号を返す
		uint32_t textureIndex = textureDatas[filePath].srvIndex;
		return textureIndex;
	}

	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath)
{
	//テクスチャ枚数上限チェック
	assert(srvManager_->Available());

	TextureData& textureData = textureDatas[filePath]; //テクスチャデータの参照を取得
	return textureData.srvHnadleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetadata(const std::string& filePath)
{
	//テクスチャ枚数上限チェック
	assert(srvManager_->Available());

	//テクスチャデータの参照を取得
	TextureData& textureData = textureDatas[filePath];
	return textureData.metadata;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t index)
{
	for (const auto& [key, tex] : textureDatas) {
		if (tex.srvIndex == index) {
			return tex.srvHnadleGPU;
		}
	}

	assert(0 && "SRV index not found in textureDatas.");
	D3D12_GPU_DESCRIPTOR_HANDLE invalid{};
	invalid.ptr = 0;
	return invalid;
}

TextureManager* TextureManager::GetInstance() {
	static TextureManager instance_;
	return &instance_;
}

void TextureManager::Finalize() {
	//delete instance;
	//instance = nullptr;

}
