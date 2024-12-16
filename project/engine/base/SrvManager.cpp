#include "SrvManager.h"
#include "DirectXCommon.h"

//最大テクスチャ枚数
const uint32_t SrvManager::kMaxSRVCount = 512;

void SrvManager::Initialize(DirectXCommon* directXCommon)
{
	//引数で受け取ってメンバ変数に記録する
	this->directXCommon_ = directXCommon;

	//SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	descriptorHeap = directXCommon_->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
	//デスクリプタ1個分のサイズを取得して記録
	descriptorSize = directXCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void SrvManager::PreDraw()
{
	//描画用のDescriptorHeapの設定
	ID3D12DescriptorHeap* descriptorHeaps[] = { descriptorHeap.Get()};
	directXCommon_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps);
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex)
{
	directXCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}

uint32_t SrvManager::Allocate()
{
	// 上限に達していないかチェック
	assert(useIndex < kMaxSRVCount);

	// returnする番号を一旦記録しておく
	uint32_t index = useIndex;
	// 次回のために番号を 1 進める
	useIndex++;
	// 上で記録した番号をreturn
	return index;
}

bool SrvManager::Available() const
{
	// 現在のインデックスが最大数未満であればtrueを返す
	return useIndex < kMaxSRVCount;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandleSUB(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandleSUB(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index)
{
	return GetCPUDescriptorHandleSUB(descriptorHeap.Get(), descriptorSize, index);
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index)
{
	return GetGPUDescriptorHandleSUB(descriptorHeap.Get(), descriptorSize, index);
}

void SrvManager::CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 標準のコンポーネントマッピング
	srvDesc.Format = Format; // テクスチャのフォーマット
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして扱う
	srvDesc.Texture2D.MipLevels = MipLevels; // Mipマップレベル数
	srvDesc.Texture2D.MostDetailedMip = 0; // 最も詳細なMipマップレベルのインデックス
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // リソースの最小レベルのクランプ値

	// 指定されたインデックスのCPUディスクリプタハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(srvIndex);

	// デバイスからSRVを作成
	directXCommon_->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, handle);
}

void SrvManager::CreateSRVforStructureBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride)
{
	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN; // 構造化バッファではフォーマットは未定義
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER; // バッファとして扱う
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 標準のコンポーネントマッピング
	srvDesc.Buffer.FirstElement = 0; // バッファの先頭要素
	srvDesc.Buffer.NumElements = numElements; // 要素数
	srvDesc.Buffer.StructureByteStride = structureByteStride; // 各構造体のサイズ
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE; // 特殊な設定はなし

	// 指定されたインデックスのCPUディスクリプタハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(srvIndex);

	// SRVを生成
	directXCommon_->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, handle);
}
