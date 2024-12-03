#pragma once

// GE3クラス化(MyClass)
#include "WindowsAPI.h"
#include "Input.h"
#include "DirectXCommon.h"
#include "SpriteCommon.h"
#include "engine/2d/Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Camera.h"
#include "SrvManager.h"
#include "ImGuiManager.h"
#include "AudioManager.h"


class MyGame
{
public://メンバ関数
	//初期化
	void Initialize();
	//終了
	void Finalize();
	//毎フレーム更新
	void Update();
	//描画
	void Draw();

	bool IsEndRequest() const { return endRequest_; } // 終了リクエスト確認

private://メンバ変数
	//ポインタ...WindowsAPI
	std::unique_ptr<WindowsAPI> windowsAPI = nullptr;
	//Audio
	std::unique_ptr<AudioManager> audio = nullptr;
	//ポインタ...Input
	std::unique_ptr<Input> input = nullptr;
	//ポインタ...DirectXCommon
	std::unique_ptr<DirectXCommon> dxCommon = nullptr;
	//ポインタ...srvManager
	std::unique_ptr<SrvManager> srvManager = nullptr;
	//ポインタ...SpriteCommon
	std::unique_ptr<SpriteCommon> spriteCommon = nullptr;
	//ポインタ...Sprite
	std::unique_ptr<Sprite> sprite = nullptr;
	//ポインタ...Object3dCommon
	std::unique_ptr<Object3dCommon> object3dCommon = nullptr;
	//ポインタ...Camera
	std::unique_ptr<Camera> camera = nullptr;
	//ポインタ...ImGuiManager
	std::unique_ptr<ImGuiManager>  imguiManager = nullptr;

	//Object3dの初期化
	Object3d* object3d = new Object3d();
	//AnotherObject3d ( もう一つのObject3d )
	Object3d* anotherObject3d = new Object3d();

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

private:
	//フラグ
	bool endRequest_ = false; // 終了フラグ

};

