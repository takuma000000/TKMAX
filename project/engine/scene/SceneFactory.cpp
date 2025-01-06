#include "SceneFactory.h"
#include "TitleScene.h"
#include "GameScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
	//次のシーンを生成
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene(dxCommon, srvManager);	//TitleSceneを生成
	} else if (sceneName == "GAME") {
		newScene = new GameScene(dxCommon, srvManager);	//GameSceneを生成
	}

	return newScene;
}
