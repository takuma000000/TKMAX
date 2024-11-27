#include "ImGuiManager.h"
#include "imgui/imgui.h"
#include <imgui/imgui_impl_win32.h>

void ImGuiManager::Initialize(WindowsAPI* winApp, DirectXCommon* dxCommon)
{
	//ImGuoのコンテキストを生成
	ImGui::CreateContext();
	//ImGuiのスタイルを設定
	ImGui::StyleColorsDark();

	//Win32用の初期化
	ImGui_ImplWin32_Init(winApp->GetHwnd());

}
