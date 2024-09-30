#pragma once
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl.h>

//入力
class Input
{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public://メンバ関数
	//初期化
	void Initialize(HINSTANCE hInstance, HWND hwnd);
	//更新
	void Update();
	//キーの押下をチェック( 押されているか )
	bool PushKey(BYTE keyNumber);

	//キーのトリガーをチェック
	bool TriggerKey(BYTE keyNumber);

private://メンバ変数
	//キーボードのデバイス
	ComPtr<IDirectInputDevice8> keyboard;
	//全キーの状態
	BYTE key[256] = {};
	//前回の全キーの状態
	BYTE keyPre[256] = {};
	//DirectInputのインスタンス
	ComPtr<IDirectInput8> directInput;

};

