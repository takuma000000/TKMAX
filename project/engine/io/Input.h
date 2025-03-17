#pragma once
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl.h>
#include <Xinput.h>
#include "WindowsAPI.h"

class Input
{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	static Input* GetInstance();

	Input() = default;
	~Input() = default;
	Input(Input&) = delete;
	Input& operator=(Input&) = delete;

	void Initialize(WindowsAPI* winApp);
	void Finalize();
	void Update();

	bool PushKey(BYTE keyNumber);
	bool TriggerKey(BYTE keyNumber);

	// 追加: ゲームパッドのボタンチェック
	bool PushButton(WORD button);
	bool TriggerButton(WORD button);

	// 追加: 左スティックの取得
	SHORT GetLeftStickX();
	SHORT GetLeftStickY();

	// 追加: 右スティックの取得
	SHORT GetRightStickX();
	SHORT GetRightStickY();

	// 追加: 振動を設定
	void SetVibration(WORD leftMotor, WORD rightMotor);

private:
	static Input* instance;

	ComPtr<IDirectInputDevice8> keyboard;
	BYTE key[256] = {};
	BYTE keyPre[256] = {};
	ComPtr<IDirectInput8> directInput;
	WindowsAPI* winApp = nullptr;

	// 追加: XInput 用のメンバ変数
	XINPUT_STATE controllerState = {};
	XINPUT_STATE prevControllerState = {};
};
