#pragma once
#include <windows.h>
#include <stdint.h>

//前方宣言
namespace TKM {
	class Framework;
}

//=============================================================
// WindowsAPIクラス
// ウィンドウの生成・更新・終了処理を管理するクラス。
//=============================================================
namespace TKM {
	class WindowsAPI {
	public://getter
		/// <summary>
		/// <para>HWNDを取得します。</para>
		/// </summary>
		/// <returns></returns>
		HWND GetHwnd() const { return hwnd_; }
		/// <summary>
		/// <para>HINSTANCEを取得します。</para>
		/// </summary>
		/// <returns></returns>
		HINSTANCE GetHInstance() const { return wc_.hInstance; }

		// Setter========================================
		/// <summary>
		/// Frameworkへの参照を設定します（終了要求の通知に使用）。
		/// </summary>
		/// <param name="framework">所有するFramework</param>
		void SetFramework(Framework* framework) { framework_ = framework; }
		// ==============================================

		/// <summary>
		/// <para>ウィンドウプロシージャ</para>
		/// </summary>
		/// <param name="hwnd"></param>
		/// <param name="msg"></param>
		/// <param name="wparam"></param>
		/// <param name="lparam"></param>
		/// <returns></returns>
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		/// <summary>
		/// ウィンドウを初期化します。
		/// </summary>
		void Initialize();
		/// <summary>
		/// ウィンドウを更新します。
		/// </summary>
		void Update();
		/// <summary>
		/// ウィンドウを終了処理します。
		/// </summary>
		void Finalize();

		/// <summary>
		/// メッセージ処理を行います。
		/// </summary>
		/// <returns></returns>
		bool ProcessMessage();

		static const int32_t kClientWidth_ = 1280;
		static const int32_t kClientHeight_ = 720;

	private:
		HWND hwnd_ = nullptr;
		WNDCLASS wc_{};

		//所有Framework（終了要求通知用）
		Framework* framework_ = nullptr;
	};
}