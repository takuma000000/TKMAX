#pragma once
#include "WindowsAPI.h"
#include "DirectXCommon.h"

class ImGuiManager
{

public:
	void Initialize(WindowsAPI* winApp, DirectXCommon* dxCommon);

private:
	WindowsAPI* winApp = nullptr;
	DirectXCommon* dxCommon = nullptr;

};

