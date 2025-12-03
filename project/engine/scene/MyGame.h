#pragma once
#include "Framework.h"

// GE3クラス化(MyClass)
#include "Input.h"
#include "Object3dCommon.h"
#include "engine/3d/camera/Camera.h"
#include "ImGuiManager.h"

//#include"GameScene.h"
//#include"TitleScene.h"
#include "SceneFactory.h"
#include "AbstractSceneFactory.h"

#include "SceneManager.h"

#include <memory>

//=============================================================
// MyGameクラス
// ゲーム全体を管理するクラス。
//=============================================================
class MyGame : public Framework{
public://メンバ関数
	/// <summary>
	/// シーンを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// シーンを終了します。
	/// </summary>
	void Finalize() override;
	/// <summary>
	/// 毎フレーム更新を行う関数。
	/// </summary>
	void Update() override;
	/// <summary>
	/// 毎フレーム描画を行う関数。
	/// </summary>
	void Draw() override;

private:
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	bool endRequest_ = false; // 終了フラグ
	std::unique_ptr<SceneManager> sceneManager_ = nullptr; // シーンマネージャー

};