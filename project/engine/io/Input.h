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
namespace TKM {
	class Input {
	public:
		template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
		static Input* GetInstance();

		Input() = default;
		~Input() = default;
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		/// <summary>
		/// オブジェクトやモジュールの初期化を行う。
		/// </summary>
		/// <param name="winApp"></param>
		void Initialize(TKM::WindowsAPI* winApp);
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
		/// <summary>
		/// <para>指定したボタンが押されているかを返します。</para>
		/// </summary>
		/// <param name="button"></param>
		/// <returns></returns>
		bool PushButton(WORD button);
		/// <summary>
		/// <para>指定したボタンが押された瞬間かを返します。</para>
		/// </summary>
		/// <param name="button"></param>
		/// <returns></returns>
		bool TriggerButton(WORD button);
		/// <summary>
		/// <para>ゲームパッドが接続されているかを返します。</para>
		/// </summary>
		/// <returns></returns>
		bool IsGamepadConnected() const;

		// Getter========================================
		/// <summary>
		/// <para>左スティックのX軸の値を取得します。</para>
		/// </summary>
		/// <returns></returns>
		SHORT GetLeftStickX();
		/// <summary>
		/// <para>左スティックのY軸の値を取得します。</para>
		/// </summary>
		/// <returns></returns>
		SHORT GetLeftStickY();
		/// <summary>
		/// <para>右スティックのX軸の値を取得します。</para>
		/// </summary>
		/// <returns></returns>
		SHORT GetRightStickX();
		/// <summary>
		///	<para>右スティックのY軸の値を取得します。</para>
		/// </summary>
		/// <returns></returns>
		SHORT GetRightStickY();
		/// <summary>
		/// <para>右トリガーの取得</para>
		/// </summary>
		/// <returns></returns>
		BYTE GetRightTrigger();
		/// <summary>
		/// <para>左トリガーの取得</para>
		/// </summary>
		/// <returns></returns>
		BYTE GetLeftTrigger();
		/// <summary>
		///	<para>マウスホイールの回転量を取得します。</para>
		/// </summary>
		/// <returns></returns>
		int  GetWheel() const { return wheel_; }
		// ==============================================
		// Setter========================================
		/// <summary>
		/// <para>コントローラーの振動を設定します。</para>
		/// </summary>
		/// <param name="leftMotor"></param>
		/// <param name="rightMotor"></param>
		void SetVibration(WORD leftMotor, WORD rightMotor);
		/// <summary>
		/// <para>マウスホイールの回転量を設定します。</para>
		/// </summary>
		/// <param name="delta"></param>
		void SetWheel(int delta) { wheel_ = delta; }
		// ==============================================
	private:

		ComPtr<IDirectInputDevice8> keyboard_;
		BYTE key_[256] = {};
		BYTE keyPre_[256] = {};
		ComPtr<IDirectInput8> directInput_;
		TKM::WindowsAPI* winApp_ = nullptr;

		// XInput 用のメンバ変数
		XINPUT_STATE controllerState_ = {};
		XINPUT_STATE prevControllerState_ = {};

		// マウスホイール量（フレーム単位でリセットされる）
		int wheel_ = 0;
	};
}