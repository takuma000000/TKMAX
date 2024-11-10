#include "TextureManager.h"
#include "DirectXCommon.h"
#include "StringUtility.h"

using namespace StringUtility;

TextureManager* TextureManager::instance = nullptr;

void TextureManager::Initialize(){
	//SRVの数と同数
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath){
	//テクスチャファイルを読んでプログラムを扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	assert(SUCCEEDED(hr));

	//MipMapの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	assert(SUCCEEDED(hr));

	//テクスチャデータを追加
	textureDatas.resize(textureDatas.size() + 1);
	//追加したテクスチャデータの参照を取得
	TextureData& textureData = textureDatas.back();

	// 画像のファイルパス、メタデータ、リソース情報を設定
	textureData.filePath = filePath;  // ファイルパス
	textureData.metadata = mipImages.GetMetadata();  // メタデータを取得
	textureData.resource = CreateTextureResource(textureData.metadata);  // テクスチャリソースを生成

	//テクスチャデータ要素数番号をSRVのインデックスとする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size());


}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(const DirectX::TexMetadata& metadata)
{
	//metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);//Textureの幅
	resourceDesc.Height = UINT(metadata.height);//Textureの高さ
	resourceDesc.MipLevels = UINT(metadata.mipLevels);//mipmapの数
	resourceDesc.DepthOrArraySize = UINT(metadata.arraySize);//奥行き or 配列Textureの配列数
	resourceDesc.Format = metadata.format;//TextureのFormat
	resourceDesc.SampleDesc.Count = 1;//サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);//Textureの次元数。

	//利用するHeapの作成。非常に特殊な運用。
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;//細かい設定を行う
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;//WriteBackポリシーでCPUアクセス可能
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;//プロセッサの近くに配置

	//Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,//Heapの設定
		D3D12_HEAP_FLAG_NONE,//Heapの特殊な設定。特になし
		&resourceDesc,//Resouceの設定
		D3D12_RESOURCE_STATE_GENERIC_READ,//初回のResourceState。	Textureは基本読むだけ
		nullptr,//Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)//作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));
	return resource;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetSRVCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSRVGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, index);
}

void TextureManager::GenerateDescpitorHeap()
{
	//DescriptorSizeを取得しておく
	descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

#pragma region ディスクリプタヒープの生成

	//RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);

#pragma endregion
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
	ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heaptype, UINT numDescriptors, bool shaderVisible) {

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heaptype;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

TextureManager* TextureManager::GetInstance(){
	if (instance == nullptr) {
		instance = new TextureManager;
	}
	return instance;
}

void TextureManager::Finalize(){
	delete instance;
	instance = nullptr;
}
