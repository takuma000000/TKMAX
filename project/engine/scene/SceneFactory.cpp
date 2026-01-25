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
			newScene = new TitleScene(dxCommon_, srvManager_);	//TitleSceneを生成
		} else if (sceneName == "GAME") {
			newScene = new GameScene(dxCommon_, srvManager_);	//GameSceneを生成
		} else if (sceneName == "CLEAR") {
			newScene = new GameClearScene(dxCommon_, srvManager_); //GameClearSceneを生成
		} else if (sceneName == "GAMEOVER") {
			newScene = new GameOverScene(dxCommon_, srvManager_); //GameOverSceneを生成
		}

		return newScene;
	}
}