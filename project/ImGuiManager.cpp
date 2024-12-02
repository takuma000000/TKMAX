#include "ImGuiManager.h"
#include "imgui/imgui.h"
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

void ImGuiManager::Initialize(WindowsAPI* winApp, DirectXCommon* dxCommon)
{
	HRESULT hr;

	dxCommon_ = dxCommon;
	winApp_ = winApp;

	//ImGuoのコンテキストを生成
	ImGui::CreateContext();
	//ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	//Win32用の初期化
	ImGui_ImplWin32_Init(winApp_->GetHwnd());

	//デスクリプタ―ヒープ設定
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//デスクリプタ―ヒープ
	hr = dxCommon_->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srvHeap_));
	assert(SUCCEEDED(hr));

	// ImGuiのDirectX12初期化関数を呼び出す
	ImGui_ImplDX12_Init(
		dxCommon_->GetDevice(), // DirectX12デバイス
		static_cast<int>(dxCommon_->GetBackBufferCount()), // バックバッファの数（型をintにキャスト）
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, // バックバッファのフォーマット（RTVと一致させる必要あり）
		srvHeap_.Get(), // SRVヒープ（Shader Resource View用のディスクリプタヒープ）
		srvHeap_->GetCPUDescriptorHandleForHeapStart(), // SRVのCPU側のハンドル
		srvHeap_->GetGPUDescriptorHandleForHeapStart() // SRVのGPU側のハンドル
	);

}

void ImGuiManager::Finalize()
{
	// ImGuiのDirectX12用の終了処理
	ImGui_ImplDX12_Shutdown();
	// ImGuiのWin32用の終了処理
	ImGui_ImplWin32_Shutdown();
	// ImGuiのコンテキストを破棄
	ImGui::DestroyContext();

	// デスクリプタヒープを解放
	srvHeap_.Reset();
}

void ImGuiManager::Begin()
{
	//ImGuiフレーム開始
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::End()
{
	ImGui::Render();
}

void ImGuiManager::Draw()
{
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	//デスクリプタヒープの配列をセットするコマンド
	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap_.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	//描画コマンドを発行
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}
