#include "SceneFactory.h"
#include "application/scene/TitleScene.h"
#include "application/scene/GameScene.h"
#include "application/scene/GameClearScene.h"
#include "application/scene/GameOverScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName){
	//次のシーンを生成
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE") {
		newScene = new TitleScene(dxCommon, srvManager);	//TitleSceneを生成
	} else if (sceneName == "GAME") {
		newScene = new GameScene(dxCommon, srvManager);	//GameSceneを生成
	} else if (sceneName == "CLEAR") {
		newScene = new GameClearScene(dxCommon, srvManager); //GameClearSceneを生成
	} else if (sceneName == "GAMEOVER") {
		newScene = new GameOverScene(dxCommon, srvManager); //GameOverSceneを生成
	}

	return newScene;
}