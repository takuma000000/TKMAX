#include "ImGuiManager.h"
#include "externals/imgui/imgui.h"
#include <externals/imgui/imgui_impl_win32.h>
#include <externals/imgui/imgui_impl_dx12.h>

void ImGuiManager::Initialize(WindowsAPI* winApp, DirectXCommon* dxCommon)
{
	HRESULT hr;

	dxCommon_ = dxCommon;
	winApp_ = winApp;

	//ImGuoのコンテキストを生成
	ImGui::CreateContext();

	ImGuiStyle& style = ImGui::GetStyle();

	//========================================
	// スタイルの設定
	style.WindowRounding = 5.0f; // ウィンドウの角を丸くする
	style.FrameRounding = 4.0f;  // フレームの角を丸くする

	// カスタムスタイルの設定
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);      // テキスト色
	colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 0.3f, 0.0f, 0.7f);      // ウィンドウ背景（透過）
	colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);      // 枠線
	colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.2f, 0.0f, 0.4f);      // フレーム背景
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.7f, 0.0f, 0.4f);      // フレーム背景（ホバー時）
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.9f, 0.0f, 0.4f);      // フレーム背景（アクティブ時）
	colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.4f, 0.0f, 0.4f);      // タイトル背景
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.6f, 0.0f, 0.4f);      // タイトル背景（アクティブ時）
	colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.9f, 0.0f, 1.0f);      // チェックマーク
	colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);      // スライダー
	colors[ImGuiCol_Button] = ImVec4(0.0f, 0.4f, 0.0f, 0.4f);      // ボタン
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.7f, 0.0f, 0.4f);      // ボタン（ホバー時）
	colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.9f, 0.0f, 0.4f);      // ボタン（アクティブ時）
	colors[ImGuiCol_Header] = ImVec4(0.0f, 0.4f, 0.0f, 0.4f);      // ヘッダー
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.7f, 0.0f, 0.4f);      // ヘッダー（ホバー時）
	colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.9f, 0.0f, 0.4f);      // ヘッダー（アクティブ時）
	colors[ImGuiCol_Separator] = ImVec4(0.0f, 0.9f, 0.0f, 0.4f);      // セパレーター
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.4f, 0.0f, 0.4f);      // リサイズグリップ
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.0f, 0.7f, 0.0f, 0.4f);      // リサイズグリップ（ホバー時）
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.0f, 0.9f, 0.0f, 0.4f);      // リサイズグリップ（アクティブ時）
	colors[ImGuiCol_Tab] = ImVec4(0.0f, 0.4f, 0.0f, 0.4f);      // タブ
	colors[ImGuiCol_TabHovered] = ImVec4(0.0f, 0.7f, 0.0f, 0.4f);      // タブ（ホバー時）
	colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.9f, 0.0f, 0.4f);      // タブ（アクティブ時）
	colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);      // ポップアップ背景（透過）

	//ImGuiのスタイルを設定
	//ImGui::StyleColorsDark();

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

	//ImGuiドッキング
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	//ImGui::StyleColorsDark();  // ダークテーマ (デフォルト)
	//ImGui::StyleColorsLight(); // ライトテーマ
	//ImGui::StyleColorsClassic(); // クラシックテーマ

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
