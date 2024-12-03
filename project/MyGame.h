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

};

