#pragma once
#include "Framework.h"

// GE3クラス化(MyClass)
#include "Input.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "ImGuiManager.h"

//#include"GameScene.h"
//#include"TitleScene.h"
#include "SceneFactory.h"
#include "AbstractSceneFactory.h"

#include "SceneManager.h"

#include <memory>


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

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

private:
	//フラグ
	bool endRequest_ = false; // 終了フラグ

	//シーン
	//TitleScene* scene_ = nullptr;
	std::unique_ptr<SceneManager> sceneManager_ = nullptr;

};