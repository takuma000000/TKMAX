#pragma once

#include "BaseScene.h"
#include <string>

/// <summary>
///シーン工場(概念)
/// </summary>

class AbstractSceneFactory
{
public:
	virtual ~AbstractSceneFactory() = default;
	/// <summary>
	/// シーンの生成
	/// </summary>
	/// <returns>生成したシーン</returns>
	virtual BaseScene* CreateScene(const std::string& sceneName) = 0;
	
};
