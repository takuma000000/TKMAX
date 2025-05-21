#pragma once
#include <windows.h>
#include <stdint.h>

class WindowsAPI
{
public://getter
	HWND GetHwnd() const { return hwnd; }
	HINSTANCE GetHInstance() const { return wc.hInstance; }

public://静的メンバ関数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public://メンバ関数
	//初期化
	void Initialize();
	//更新
	void Update();
	//終了
	void Finalize();

	int GetWindowWidth() const { return windowWidth_; }
	int GetWindowHeight() const { return windowHeight_; }


public://定数
	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public:
	//メッセージの処理
	bool ProcessMessage();

private:
	//ウィンドウハンドル
	HWND hwnd = nullptr;
	//ウィンドウクラスの設定
	WNDCLASS wc{};
};

