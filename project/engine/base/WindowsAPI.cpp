#pragma comment(lib,"winmm.lib")

#include "WindowsAPI.h"
#include <cstdint>
//#include "externals/imgui/imgui.h"
#include <iostream>
#include <imgui/imgui_impl_win32.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//ウィンドウプロシージャ
LRESULT CALLBACK WindowsAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
		return true;
	}

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		//ウィンドウが破壊された
	case WM_DESTROY:
		//OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WindowsAPI::Initialize()
{
	CoInitializeEx(0, COINIT_MULTITHREADED);
	//システムターマーの分解能を上げる
	timeBeginPeriod(1);

#pragma region Windowの生成
	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス名( なんでも良い )
	wc.lpszClassName = L"CG2WindowClass";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスを登録する
	RegisterClass(&wc);

	//ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWidth ,kClientHeight };

	//クライアント領域を元に実際のサイズに wrc を変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成
	hwnd = CreateWindow(
		wc.lpszClassName,		//利用するクラス名
		L"TkmENGINE",					//タイトルバーの文字( なんでも良い )
		WS_OVERLAPPEDWINDOW,	//ウィンドウスタイル
		CW_USEDEFAULT,			//表示X座標(Windowsに任せる)
		CW_USEDEFAULT,			//表示Y座標(WindowsOSに任せる)
		wrc.right - wrc.left,	//ウィンドウ横幅
		wrc.bottom - wrc.top,	//ウィンドウ縦幅
		nullptr,				//親ウィンドウハンドル
		nullptr,				//メニューハンドル
		wc.hInstance,			//インスタンスハンドル
		nullptr					//オプション
	);

	//ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

#pragma endregion
}

void WindowsAPI::Update()
{
}

void WindowsAPI::Finalize()
{
	CloseWindow(hwnd);
	CoUninitialize();
}

bool WindowsAPI::ProcessMessage()
{
	MSG msg{};

	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT) {
		return true;
	}

	return false;
}
