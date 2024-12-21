#pragma once
#include "BaseScene.h"

class SceneManager
{
public:
	//次シーン予約
	void SetNextScene(BaseScene* nextScene) {
		nextScene_ = nextScene;
	}

private:

	//今のシーン( 実行中 )
	BaseScene* scene_ = nullptr;

	//次のシーン( 次フレームから実行 )
	BaseScene* nextScene_ = nullptr;

public://メンバ関数
	void Update();
	void Draw();

	//デストラクタ
	~SceneManager();
};

