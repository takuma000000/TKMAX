#pragma once
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl.h>
#include <Xinput.h>
#include "WindowsAPI.h"

//=============================================================
// Inputクラス
// キーボードとゲームパッドの入力を管理するクラス。
//=============================================================
class Input{
public:
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	static Input* GetInstance();

	Input() = default;
	~Input() = default;
	Input(Input&) = delete;
	Input& operator=(Input&) = delete;

	/// <summary>
	/// <para>Inputの初期化を行います。</para>
	/// </summary>
	/// <param name="winApp"></param>
	void Initialize(WindowsAPI* winApp);
	/// <summary>
	/// オブジェクトやモジュールの終了処理（クリーンアップ）を行う。
	/// <para>Inputの終了処理を行います。</para>
	/// </summary>
	void Finalize();
	/// <summary>
	/// <para>Inputの更新を行います。</para>
	/// </summary>
	void Update();

	/// <summary>
	/// <para>指定したキーが押されているかを返します。</para>
	/// </summary>
	/// <param name="keyNumber"></param>
	/// <returns></returns>
	bool PushKey(BYTE keyNumber);
	/// <summary>
	/// <para>指定したキーが押された瞬間かを返します。</para>
	/// </summary>
	/// <param name="keyNumber"></param>
	/// <returns></returns>
	bool TriggerKey(BYTE keyNumber);

	// 追加: ゲームパッドのボタンチェック
	///<para>指定したボタンが押されているかを返します。</para>
	/// <param name="button"></param>
	bool PushButton(WORD button);
	/// <summary>
	/// <para>指定したボタンが押された瞬間かを返します。</para>
	/// </summary>
	/// <param name="button"></param>
	/// <returns></returns>
	bool TriggerButton(WORD button);

	// 追加: 左スティックの取得
	///<para>左スティックのX軸の値を取得します。</para>
	SHORT GetLeftStickX();
	///<para>左スティックのY軸の値を取得します。</para>
	SHORT GetLeftStickY();

	// 追加: 右スティックの取得
	////<para>右スティックのX軸の値を取得します。</para>
	SHORT GetRightStickX();
	/// <summary>
	///	<para>右スティックのY軸の値を取得します。</para>
	/// </summary>
	/// <returns></returns>
	SHORT GetRightStickY();

	// 左トリガーの取得
	/// <para>右トリガーの取得</para>
	BYTE GetRightTrigger();
	// 右トリガーの取得
	/// <para>左トリガーの取得</para>
	BYTE GetLeftTrigger();

	// 追加: 振動を設定
	///<para>コントローラーの振動を設定します。</para>
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