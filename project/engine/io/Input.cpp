#include "Input.h"
#include <cassert>
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"Xinput.lib")  // XInputのライブラリ追加

Input* Input::instance = nullptr;

Input* Input::GetInstance()
{
	if (instance == nullptr) {
		instance = new Input;
	}
	return instance;
}

void Input::Initialize(WindowsAPI* windowsAPI)
{
	HRESULT result;

	this->winApp = windowsAPI;

	result = DirectInput8Create(winApp->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));
	result = keyboard->SetCooperativeLevel(winApp->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));
}

void Input::Finalize()
{
	delete instance;
	instance = nullptr;
}

void Input::Update()
{
	// キーボードの状態を更新
	memcpy(keyPre, key, sizeof(key));
	keyboard->Acquire();
	keyboard->GetDeviceState(sizeof(key), key);

	// ゲームパッドの状態を更新
	prevControllerState = controllerState;
	ZeroMemory(&controllerState, sizeof(XINPUT_STATE));
	XInputGetState(0, &controllerState);
}

// キーボード入力判定
bool Input::PushKey(BYTE keyNumber)
{
	return (key[keyNumber] & 0x80) != 0;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	return !(keyPre[keyNumber] & 0x80) && (key[keyNumber] & 0x80);
}

// ゲームパッドのボタン判定
bool Input::PushButton(WORD button)
{
	return (controllerState.Gamepad.wButtons & button) != 0;
}

bool Input::TriggerButton(WORD button)
{
	return !(prevControllerState.Gamepad.wButtons & button) &&
		(controllerState.Gamepad.wButtons & button);
}

// 左スティックの取得
SHORT Input::GetLeftStickX()
{
	return controllerState.Gamepad.sThumbLX;
}

SHORT Input::GetLeftStickY()
{
	return controllerState.Gamepad.sThumbLY;
}

// 右スティックの取得
SHORT Input::GetRightStickX()
{
	return controllerState.Gamepad.sThumbRX;
}

SHORT Input::GetRightStickY()
{
	return controllerState.Gamepad.sThumbRY;
}

BYTE Input::GetRightTrigger()
{
	return controllerState.Gamepad.bRightTrigger;
}

BYTE Input::GetLeftTrigger()
{
	return controllerState.Gamepad.bLeftTrigger;
}

//// トリガー入力
//BYTE Input::GetLeftTrigger()
//{
//	return controllerState.Gamepad.bLeftTrigger;
//}
//
//BYTE Input::GetRightTrigger()
//{
//	return controllerState.Gamepad.bRightTrigger;
//}

// コントローラーの振動
void Input::SetVibration(WORD leftMotor, WORD rightMotor)
{
	XINPUT_VIBRATION vibration;
	ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
	vibration.wLeftMotorSpeed = leftMotor;
	vibration.wRightMotorSpeed = rightMotor;
	XInputSetState(0, &vibration);
}
