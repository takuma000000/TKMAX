#pragma once
#include "WindowsAPI.h"
#include "DirectXCommon.h"

//=============================================================
// ImGuiManagerクラス
// ImGuiの初期化・描画・終了処理およびテーマ設定を管理するクラス。
//=============================================================
class ImGuiManager{
public:
	/// <summary>
	/// ImGuiの初期化を行う関数
	/// </summary>
	/// <param name="winApp"></param>
	/// <param name="dxCommon"></param>
	void Initialize(WindowsAPI* winApp, DirectXCommon* dxCommon);
	/// <summary>
	/// ImGuiの終了処理を行う関数
	/// </summary>
	void Finalize();

	/// <summary>
	/// ImGui受付開始
	/// </summary>
	void Begin();
	/// <summary>
	/// ImGui受付終了
	/// </summary>
	void End();
	/// <summary>
	/// ImGuiの描画を行う関数
	/// </summary>
	void Draw();

	//ImGuiの色関数===========================================
	/// <summary>
	/// イチゴ色に設定する関数
	/// </summary>
	void SetColorStrawberry();
	/// <summary>
	/// 白虎色に設定する関数
	/// </summary>
	void SetColorWhiteTiger();
	/// <summary>
	/// 虹色に設定する関数
	/// </summary>
	void SetColorRainbow();
	//=======================================================
private:
	WindowsAPI* winApp_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;

	//SRV用デスクリプタ―ヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
};