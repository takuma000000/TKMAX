#include "SceneFactory.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "GameClearScene.h"
#include "GameOverScene.h"

namespace TKM {
	TKM::BaseScene* SceneFactory::CreateScene(const std::string& sceneName) {
		//次のシーンを生成
		TKM::BaseScene* newScene = nullptr;

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
}