#pragma comment(lib,"winmm.lib")

#include "WindowsAPI.h"
#include <cstdint>
#include <iostream>
#include "Framework.h"
#include "Input.h"

#ifdef USE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

namespace TKM {
	LRESULT CALLBACK WindowsAPI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

		// WM_NCCREATE で this を紐づける
		if (msg == WM_NCCREATE) {
			auto create = reinterpret_cast<CREATESTRUCT*>(lparam);
			auto self = reinterpret_cast<WindowsAPI*>(create->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		}

		// this を取得（以降のメッセージで使う）
		auto self = reinterpret_cast<WindowsAPI*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		// 先にホイール量だけ拾う（ImGuiより前）
		if (msg == WM_MOUSEWHEEL) {
			int delta = GET_WHEEL_DELTA_WPARAM(wparam);
			Input::GetInstance()->SetWheel(delta / WHEEL_DELTA);
		}

#ifdef USE_IMGUI
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
			return true;
		}
#endif

		switch (msg) {
		case WM_CLOSE:
			if (self && self->framework_) {
				self->framework_->SetEndRequest(true);
			}
			DestroyWindow(hwnd);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	void WindowsAPI::Initialize() {
		CoInitializeEx(0, COINIT_MULTITHREADED);
		timeBeginPeriod(1);

#pragma region Windowの生成
		wc_.lpfnWndProc = WindowProc;
		wc_.lpszClassName = L"CG2WindowClass";
		wc_.hInstance = GetModuleHandle(nullptr);
		wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);

		RegisterClass(&wc_);

		RECT wrc = { 0,0,kClientWidth_ ,kClientHeight_ };
		AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

		hwnd_ = CreateWindow(
			wc_.lpszClassName,
			L"TKMAX",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			wrc.right - wrc.left,
			wrc.bottom - wrc.top,
			nullptr,
			nullptr,
			wc_.hInstance,
			this // WindowProc に this を渡す
		);

		ShowWindow(hwnd_, SW_SHOW);
#pragma endregion
	}

	void WindowsAPI::Update() {
	}

	void WindowsAPI::Finalize() {
		CloseWindow(hwnd_);
		CoUninitialize();
	}

	bool WindowsAPI::ProcessMessage() {
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
}