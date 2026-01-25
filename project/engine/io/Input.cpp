#include "Input.h"
#include <cassert>
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"Xinput.lib")  // XInputのライブラリ追加

namespace TKM {
	Input* Input::instance_ = nullptr;

	Input* Input::GetInstance() {
		if (instance_ == nullptr) {
			instance_ = new Input;
		}
		return instance_;
	}

	void Input::Initialize(TKM::WindowsAPI* windowsAPI) {
		HRESULT result;

		this->winApp_ = windowsAPI; // WindowsAPIのポインタを保存

		// DirectInputの初期化
		result = DirectInput8Create(winApp_->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput_, nullptr);
		assert(SUCCEEDED(result));
		// キーボードデバイスの生成
		result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
		assert(SUCCEEDED(result));
		// デバイスの設定
		result = keyboard_->SetDataFormat(&c_dfDIKeyboard);
		assert(SUCCEEDED(result));
		// 協調レベルの設定
		result = keyboard_->SetCooperativeLevel(winApp_->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
		assert(SUCCEEDED(result));
	}

	void Input::Finalize() {
		delete instance_;
		instance_ = nullptr;
	}

	void Input::Update() {
		// キーボードの状態を更新
		memcpy(keyPre_, key_, sizeof(key_));
		keyboard_->Acquire();
		keyboard_->GetDeviceState(sizeof(key_), key_);

		// ゲームパッドの状態を更新
		prevControllerState_ = controllerState_;
		ZeroMemory(&controllerState_, sizeof(XINPUT_STATE));
		XInputGetState(0, &controllerState_);
	}

	// キーボード入力判定
	bool Input::PushKey(BYTE keyNumber) {
		return (key_[keyNumber] & 0x80) != 0;
	}

	// キーボードのトリガー判定
	bool Input::TriggerKey(BYTE keyNumber) {
		return !(keyPre_[keyNumber] & 0x80) && (key_[keyNumber] & 0x80);
	}

	// ゲームパッドのボタン判定
	bool Input::PushButton(WORD button) {
		return (controllerState_.Gamepad.wButtons & button) != 0;
	}

	// ゲームパッドのトリガー判定
	bool Input::TriggerButton(WORD button) {
		return !(prevControllerState_.Gamepad.wButtons & button) &&
			(controllerState_.Gamepad.wButtons & button);
	}

	// 左スティックの取得
	SHORT Input::GetLeftStickX() {
		return controllerState_.Gamepad.sThumbLX;
	}

	// Y軸の取得
	SHORT Input::GetLeftStickY() {
		return controllerState_.Gamepad.sThumbLY;
	}

	// 右スティックの取得
	SHORT Input::GetRightStickX() {
		return controllerState_.Gamepad.sThumbRX;
	}

	// Y軸の取得
	SHORT Input::GetRightStickY() {
		return controllerState_.Gamepad.sThumbRY;
	}

	// トリガー入力
	BYTE Input::GetRightTrigger() {
		return controllerState_.Gamepad.bRightTrigger;
	}

	// トリガー入力
	BYTE Input::GetLeftTrigger() {
		return controllerState_.Gamepad.bLeftTrigger;
	}

	// コントローラーの振動
	void Input::SetVibration(WORD leftMotor, WORD rightMotor) {
		// 振動の設定
		XINPUT_VIBRATION vibration;
		ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
		vibration.wLeftMotorSpeed = leftMotor;
		vibration.wRightMotorSpeed = rightMotor;
		XInputSetState(0, &vibration);
	}
}