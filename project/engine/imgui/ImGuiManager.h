#pragma once
#include "WindowsAPI.h"
#include "DirectXCommon.h"

class ImGuiManager
{

public:
	void Initialize(WindowsAPI* winApp, DirectXCommon* dxCommon);
	//終了
	void Finalize();

	//ImGui受付開始
	void Begin();
	//ImGui受付終了
	void End();
	//画面への描画
	void Draw();

	///ImGuiの色関数===========================================
	//イチゴ色
	void SetColorStrawberry();
	//ホワイトタイガー
	void SetColorWhiteTiger();

	///=======================================================


private:
	WindowsAPI* winApp_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;

	//SRV用デスクリプタ―ヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;

};

