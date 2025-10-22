#pragma once
#include "WindowsAPI.h"
#include "DirectXCommon.h"

//=============================================================
// ImGuiManagerクラス
// ImGuiの初期化・描画・終了処理およびテーマ設定を管理するクラス。
//=============================================================
class ImGuiManager
{

public:
	/// <summary>
	///	<para>ImGuiの初期化を行う関数</para>
	/// </summary>
	/// <param name="winApp"></param>
	/// <param name="dxCommon"></param>
	void Initialize(WindowsAPI* winApp, DirectXCommon* dxCommon);
	//終了
	///<summary>ImGuiの終了処理を行う関数</summary>
	void Finalize();

	//ImGui受付開始
	///<param name="dxCommon">DirectXCommonのポインタ</param>
	void Begin();
	//ImGui受付終了
	///<param name="dxCommon">DirectXCommonのポインタ</param>
	void End();
	//画面への描画
	///<param name="dxCommon">DirectXCommonのポインタ</param>
	void Draw();

	///ImGuiの色関数===========================================
	//イチゴ色
	///<summary>イチゴ色に設定する関数</summary>
	void SetColorStrawberry();
	//ホワイトタイガー
	///<summary>ホワイトタイガーに設定する関数</summary>
	void SetColorWhiteTiger();
	//レインボー キラキラ
	///<summary>レインボー キラキラに設定する関数</summary>
	void SetColorRainbow();
	///=======================================================


private:
	WindowsAPI* winApp_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;

	//SRV用デスクリプタ―ヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;

};

