#pragma once
#include "Framework.h"

// GE3クラス化(MyClass)
#include "Input.h"
#include "SpriteCommon.h"
#include "engine/2d/Sprite.h"
#include "TextureManager.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "Model.h"
#include "ModelCommon.h"
#include "ModelManager.h"
#include "Camera.h"
#include "ImGuiManager.h"
#include "AudioManager.h"
#include "ParticleManager.h"
#include "ParticlerEmitter.h"


class MyGame : public Framework
{
public://メンバ関数
	//初期化
	void Initialize() override;
	//終了
	void Finalize() override;
	//毎フレーム更新
	void Update() override;
	//描画
	void Draw() override;

private://メンバ変数

	//Audio
	std::unique_ptr<AudioManager> audio = nullptr;
	//ポインタ...Input
	std::unique_ptr<Input> input = nullptr;
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
	//ポインタ...ParticleEmitter
	std::unique_ptr<ParticleEmitter>  particleEmitter = nullptr;

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

