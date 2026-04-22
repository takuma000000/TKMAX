#pragma once
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wrl.h>
#include <Xinput.h>
#include "WindowsAPI.h"

//=============================================================
// Inputクラス
// キーボードとゲームパッドの入力を管理するクラス
//=============================================================
namespace TKM {
	class Input {
	public:
		template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

		//=============================================================
		// 取得・生成
		//=============================================================

		/// <summary>
		/// インスタンスを取得します。
		/// </summary>
		static Input* GetInstance();

		Input() = default;
		~Input() = default;
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		//=============================================================
		// 初期化・終了・更新
		//=============================================================

		/// <summary>
		/// Inputを初期化します。
		/// </summary>
		/// <param name="winApp">Windows管理</param>
		void Initialize(TKM::WindowsAPI* winApp);

		/// <summary>
		/// Inputを終了します。
		/// </summary>
		void Finalize();

		/// <summary>
		/// Inputを更新します。
		/// </summary>
		void Update();

		//=============================================================
		// キーボード・パッド入力
		//=============================================================

		/// <summary>
		/// 指定したキーが押されているかを返します。
		/// </summary>
		/// <param name="keyNumber">キー番号</param>
		/// <returns>押されていたらtrue</returns>
		bool PushKey(BYTE keyNumber);

		/// <summary>
		/// 指定したキーが押された瞬間かを返します。
		/// </summary>
		/// <param name="keyNumber">キー番号</param>
		/// <returns>押された瞬間ならtrue</returns>
		bool TriggerKey(BYTE keyNumber);

		/// <summary>
		/// 指定したボタンが押されているかを返します。
		/// </summary>
		/// <param name="button">ボタン</param>
		/// <returns>押されていたらtrue</returns>
		bool PushButton(WORD button);

		/// <summary>
		/// 指定したボタンが押された瞬間かを返します。
		/// </summary>
		/// <param name="button">ボタン</param>
		/// <returns>押された瞬間ならtrue</returns>
		bool TriggerButton(WORD button);

		/// <summary>
		/// ゲームパッドが接続されているかを返します。
		/// </summary>
		/// <returns>接続されていたらtrue</returns>
		bool IsGamepadConnected() const;

		//=============================================================
		// Getter
		//=============================================================

		/// <summary>
		/// 左スティックのX軸値を取得します。
		/// </summary>
		/// <returns>X軸値</returns>
		SHORT GetLeftStickX();

		/// <summary>
		/// 左スティックのY軸値を取得します。
		/// </summary>
		/// <returns>Y軸値</returns>
		SHORT GetLeftStickY();

		/// <summary>
		/// 右スティックのX軸値を取得します。
		/// </summary>
		/// <returns>X軸値</returns>
		SHORT GetRightStickX();

		/// <summary>
		/// 右スティックのY軸値を取得します。
		/// </summary>
		/// <returns>Y軸値</returns>
		SHORT GetRightStickY();

		/// <summary>
		/// 右トリガー値を取得します。
		/// </summary>
		/// <returns>右トリガー値</returns>
		BYTE GetRightTrigger();

		/// <summary>
		/// 左トリガー値を取得します。
		/// </summary>
		/// <returns>左トリガー値</returns>
		BYTE GetLeftTrigger();

		/// <summary>
		/// マウスホイール回転量を取得します。
		/// </summary>
		/// <returns>ホイール回転量</returns>
		int GetWheel() const { return wheel_; }

		//=============================================================
		// Setter
		//=============================================================

		/// <summary>
		/// コントローラーの振動を設定します。
		/// </summary>
		/// <param name="leftMotor">左モーターの強さ</param>
		/// <param name="rightMotor">右モーターの強さ</param>
		void SetVibration(WORD leftMotor, WORD rightMotor);

		/// <summary>
		/// マウスホイール回転量を設定します。
		/// </summary>
		/// <param name="delta">ホイール回転量</param>
		void SetWheel(int delta) { wheel_ = delta; }

	private:
		//=============================================================
		// DirectInput
		//=============================================================

		ComPtr<IDirectInputDevice8> keyboard_; // キーボードデバイス
		BYTE key_[256] = {};                   // 現在キー状態
		BYTE keyPre_[256] = {};                // 前フレームキー状態
		ComPtr<IDirectInput8> directInput_;    // DirectInput本体
		TKM::WindowsAPI* winApp_ = nullptr;    // Windows管理

		//=============================================================
		// XInput
		//=============================================================

		XINPUT_STATE controllerState_ = {};     // 現在コントローラー状態
		XINPUT_STATE prevControllerState_ = {}; // 前フレームコントローラー状態

		//=============================================================
		// マウス
		//=============================================================

		int wheel_ = 0; // マウスホイール量
	};
}