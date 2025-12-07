#pragma once
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

class DirectXCommon;

//=============================================================
// SrvManagerクラス
// SRV（Shader Resource View）ヒープの管理と割り当てを行うクラス。
//=============================================================
class SrvManager {

private:
	DirectXCommon* directXCommon_ = nullptr;

	//SRV用のデスクリプタサイズ
	uint32_t descriptorSize;
	//SRVデスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	//次に使用するSRVインデックス
	uint32_t useIndex = 0;

public:
	//デストラクタ
	~SrvManager();

	//初期化
	/// <summary>SRVマネージャを初期化します。</summary>
	/// <param name="directXCommon">DirectX共通。</param>
	void Initialize(DirectXCommon* directXCommon);
	//ヒープセットコマンド
	/// <summary>SRVデスクリプタヒープをセットします。</summary>
	void PreDraw();
	//SRVセットコマンド
	/// <summary>SRVデスクリプタテーブルをセットします。</summary>
	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

	//
	/// <summary>SRVを1つ割り当てます。</summary>
	uint32_t Allocate();

	//確保可能チェック
	/// <summary>SRVを確保可能かチェックします。</summary>
	bool Available() const;

	//最大SRV数( 最大テクスチャ枚数 )
	static const uint32_t kMaxSRVCount;

public: //色々な関数
	//指定番号のCPUディスクリプタハンドルを取得する
	/// <summary>指定番号のCPUディスクリプタハンドルを取得します。</summary>
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleSUB(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	//指定番号のGPUディスクリプタハンドルを取得する
	/// <summary>指定番号のGPUディスクリプタハンドルを取得します。</summary>
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleSUB(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);
	//SRVの指定番号のCPUディスクリプタハンドルを取得する
	/// <summary>SRVの指定番号のCPUディスクリプタハンドルを取得します。</summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
	//SRVの指定番号のGPUディスクリプタハンドルを取得する
	/// <summary>SRVの指定番号のGPUディスクリプタハンドルを取得します。</summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);
	//SRV生成関数( テクスチャ用 )
	///	<summary>テクスチャ2D用のSRVを生成します。</summary>
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);
	//SRV生成
	///	<summary>構造化バッファ用のSRVを生成します。</summary>
	void CreateSRVforStructureBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

	//getter
	/// <summary>SRVデスクリプタヒープのゲッター。</summary>
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSrvDescriptorHeap() const { return descriptorHeap; }
	///<summary>SRVデスクリプタサイズのゲッター。</summary>
	uint32_t GetDescriptorSizeSRV() { return descriptorSize; }
};