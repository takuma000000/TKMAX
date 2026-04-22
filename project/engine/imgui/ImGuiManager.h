#pragma once
#include "WindowsAPI.h"
#include "DirectXCommon.h"

//=============================================================
// ImGuiManagerクラス
// ImGuiの初期化・描画・終了処理およびテーマ設定を管理するクラス
//=============================================================
namespace TKM {
	class ImGuiManager {
	public:
		//=============================================================
		// 初期化・終了
		//=============================================================

		/// <summary>
		/// ImGuiを初期化します。
		/// </summary>
		/// <param name="winApp">Windows管理</param>
		/// <param name="dxCommon">DirectX共通管理</param>
		void Initialize(TKM::WindowsAPI* winApp, TKM::DirectXCommon* dxCommon);

		/// <summary>
		/// ImGuiを終了します。
		/// </summary>
		void Finalize();

		//=============================================================
		// フレーム制御・描画
		//=============================================================

		/// <summary>
		/// ImGuiの受付を開始します。
		/// </summary>
		void Begin();

		/// <summary>
		/// ImGuiの受付を終了します。
		/// </summary>
		void End();

		/// <summary>
		/// ImGuiを描画します。
		/// </summary>
		void Draw();

		//=============================================================
		// テーマ設定
		//=============================================================

		/// <summary>
		/// イチゴ色に設定します。
		/// </summary>
		void SetColorStrawberry();

		/// <summary>
		/// 白虎色に設定します。
		/// </summary>
		void SetColorWhiteTiger();

		/// <summary>
		/// 虹色に設定します。
		/// </summary>
		void SetColorRainbow();

	private:
		//=============================================================
		// 共通参照
		//=============================================================

		TKM::WindowsAPI* winApp_ = nullptr;          // Windows管理
		TKM::DirectXCommon* dxCommon_ = nullptr;     // DirectX共通管理

		//=============================================================
		// GPUリソース
		//=============================================================

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_; // SRV用デスクリプタヒープ
	};
}