#pragma once
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace TKM {
	class DirectXCommon;
}

namespace TKM {

	//=============================================================
	// SrvManagerクラス
	// SRVヒープの管理を行うクラス
	//=============================================================
	class SrvManager {
	private:
		//=============================================================
		// 共通参照
		//=============================================================

		TKM::DirectXCommon* directXCommon_ = nullptr; // DirectX共通管理

		//=============================================================
		// SRVヒープ
		//=============================================================

		uint32_t descriptorSize_ = 0; // デスクリプタサイズ
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_; // デスクリプタヒープ

		uint32_t useIndex_ = 0; // 次に使用するSRVインデックス

	public:
		//=============================================================
		// 生成・破棄
		//=============================================================

		~SrvManager();

		//=============================================================
		// 初期化・描画準備
		//=============================================================

		/// <summary>
		/// SRVマネージャを初期化します。
		/// </summary>
		void Initialize(TKM::DirectXCommon* directXCommon);

		/// <summary>
		/// SRVデスクリプタヒープをセットします。
		/// </summary>
		void PreDraw();

		/// <summary>
		/// SRVデスクリプタテーブルをセットします。
		/// </summary>
		void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex);

		//=============================================================
		// SRV割り当て
		//=============================================================

		/// <summary>
		/// SRVを1つ割り当てます。
		/// </summary>
		uint32_t Allocate();

		/// <summary>
		/// SRVを確保可能かを返します。
		/// </summary>
		bool Available() const;

		/// <summary>
		/// 最大SRV数を表します。
		/// </summary>
		static const uint32_t kMaxSRVCount;

		//=============================================================
		// ディスクリプタ取得
		//=============================================================

		/// <summary>
		/// 指定番号のCPUディスクリプタハンドルを取得します。
		/// </summary>
		static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleSUB(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

		/// <summary>
		/// 指定番号のGPUディスクリプタハンドルを取得します。
		/// </summary>
		static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleSUB(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

		/// <summary>
		/// SRVのCPUディスクリプタハンドルを取得します。
		/// </summary>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);

		/// <summary>
		/// SRVのGPUディスクリプタハンドルを取得します。
		/// </summary>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

		//=============================================================
		// SRV生成
		//=============================================================

		/// <summary>
		/// テクスチャ2D用SRVを生成します。
		/// </summary>
		void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

		/// <summary>
		/// 構造化バッファ用SRVを生成します。
		/// </summary>
		void CreateSRVforStructureBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride);

		//=============================================================
		// Getter

		/// <summary>
		/// SRVデスクリプタヒープを取得します。
		/// </summary>
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSrvDescriptorHeap() const { return descriptorHeap_; }

		/// <summary>
		/// SRVデスクリプタサイズを取得します。
		/// </summary>
		uint32_t GetDescriptorSizeSRV() { return descriptorSize_; }

		//=============================================================
	};
}