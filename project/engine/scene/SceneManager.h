#pragma once
#include "BaseScene.h"
#include "DirectXCommon.h"
#include "AbstractSceneFactory.h"

class SceneManager
{
public:
	//次シーン予約
	void SetNextScene(BaseScene* nextScene) {
		nextScene_ = nextScene;
	}

	//メンバ関数
	//シーンファクトリー
	void SetSceneFactory(AbstractSceneFactory* sceneFactory) {
		sceneFactory_ = sceneFactory;
	}

private:

	//今のシーン( 実行中 )
	BaseScene* scene_ = nullptr;

	//次のシーン( 次フレームから実行 )
	BaseScene* nextScene_ = nullptr;

	DirectXCommon* dxCommon = nullptr;

	//シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;

public://メンバ関数
	void Update();
	void Draw();

	//デストラクタ
	~SceneManager();
};

