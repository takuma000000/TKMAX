#pragma once
#include <windows.h>
#include <stdint.h>

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

	public://静的メンバ関数
		/// <summary>
		/// <para>ウィンドウプロシージャ</para>
		/// </summary>
		/// <param name="hwnd"></param>
		/// <param name="msg"></param>
		/// <param name="wparam"></param>
		/// <param name="lparam"></param>
		/// <returns></returns>
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	public://メンバ関数
		//初期化
		/// <summary>ウィンドウを初期化します。</summary>
		void Initialize();
		//更新
		/// <summary>ウィンドウを更新します。</summary>
		void Update();
		//終了
		/// <summary>ウィンドウを終了します。</summary>
		void Finalize();

	public://定数
		//クライアント領域のサイズ
		static const int32_t kClientWidth_ = 1280;
		static const int32_t kClientHeight_ = 720;

	public:
		//メッセージの処理
		/// <summary>メッセージの処理を行います。</summary>
		bool ProcessMessage();

	private:
		//ウィンドウハンドル
		HWND hwnd_ = nullptr;
		//ウィンドウクラスの設定
		WNDCLASS wc_{};
	};
}