#include "SceneFactory.h"
#include "TitleScene.h"
#include "GameScene.h"
#include "GameClearScene.h"
#include "GameOverScene.h"

namespace TKM {
	std::unique_ptr<TKM::BaseScene> SceneFactory::CreateScene(const std::string& sceneName) {
		// シーン名に応じてシーンを生成して返す
		if (sceneName == "TITLE") {
			return std::make_unique<TitleScene>(dxCommon_, srvManager_);
		} else if (sceneName == "GAME") { // "GAME"というシーン名でゲーム本編シーンを生成
			return std::make_unique<GameScene>(dxCommon_, srvManager_);
		} else if (sceneName == "CLEAR") { // "CLEAR"というシーン名でゲームクリアシーンを生成
			return std::make_unique<GameClearScene>(dxCommon_, srvManager_);
		} else if (sceneName == "GAMEOVER") { // "GAMEOVER"というシーン名でゲームオーバーシーンを生成
			return std::make_unique<GameOverScene>(dxCommon_, srvManager_);
		}
		// シーン名が不明な場合はnullptrを返す
		return nullptr;
	}
}