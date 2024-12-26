#include "Input.h"
#include <cassert>
#include <windowsx.h> // マウス関連のマクロ用
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

void Input::Initialize(WindowsAPI* windowsAPI)
{
	HRESULT result;

	//借りてきたWinAppのインスタンスを記録
	this->winApp = windowsAPI;

	//DirectInputのインスタンス作成
	result = DirectInput8Create(winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));
	//キーボードデバイス生成
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));
	//入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));
	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

void Input::Update()
{
	//前回のキー入力を保存
	memcpy(keyPre, key, sizeof(key));

	//キーボード情報の取得開始
	keyboard->Acquire();
	//全キーの入力情報を取得する
	keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::PushKey(BYTE keyNumber)
{
	//指定キーを押していればtrueを返す
	if (key[keyNumber]) {
		return true;
	}
	//そうでなければfalseで返す
	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	//指定キーを押していればtrueを返す
	if (keyPre[keyNumber]) {
		return true;
	}
	//そうでなければfalseで返す
	return false;
}

POINT Input::GetMousePosition()
{
	POINT point;
	GetCursorPos(&point); // デスクトップ座標系でマウス位置を取得
	ScreenToClient(winApp->GetHwnd(), &point); // ウィンドウ座標系に変換
	return point;
}
