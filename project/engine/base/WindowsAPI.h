#pragma once
#include <windows.h>
#include <stdint.h>

// 前方宣言
namespace TKM {
	class Framework;
}

namespace TKM {

	//=============================================================
	// WindowsAPIクラス
	// ウィンドウ管理を行うクラス
	//=============================================================
	class WindowsAPI {
	public:
		//=============================================================
		// Getter

		/// <summary>
		/// HWNDを取得します。
		/// </summary>
		HWND GetHwnd() const { return hwnd_; }

		/// <summary>
		/// HINSTANCEを取得します。
		/// </summary>
		HINSTANCE GetHInstance() const { return wc_.hInstance; }

		/// <summary>
		/// クライアント幅を取得します。
		/// </summary>
		static int32_t GetClientWidth() { return kClientWidth_; }

		/// <summary>
		/// クライアント高さを取得します。
		/// </summary>
		static int32_t GetClientHeight() { return kClientHeight_; }

		//=============================================================
		// Setter

		/// <summary>
		/// Frameworkを設定します。
		/// </summary>
		void SetFramework(Framework* framework) { framework_ = framework; }

		//=============================================================

		//=============================================================
		// ウィンドウ処理
		//=============================================================

		/// <summary>
		/// ウィンドウプロシージャです。
		/// </summary>
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
		/// ウィンドウを終了します。
		/// </summary>
		void Finalize();

		/// <summary>
		/// メッセージを処理します。
		/// </summary>
		bool ProcessMessage();

	private:
		//=============================================================
		// ウィンドウ情報
		//=============================================================

		HWND hwnd_ = nullptr; // ウィンドウハンドル
		WNDCLASS wc_{};       // ウィンドウクラス

		//=============================================================
		// 参照
		//=============================================================

		Framework* framework_ = nullptr; // Framework参照

		//=============================================================
		// 定数
		//=============================================================

		static const int32_t kClientWidth_ = 1280;
		static const int32_t kClientHeight_ = 720;
	};
}