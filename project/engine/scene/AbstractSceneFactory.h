#pragma once

#include "BaseScene.h"
#include <string>

/// <summary>
///シーン工場(概念)
/// </summary>

//=============================================================
// AbstractSceneFactoryクラス
// シーン生成のインターフェースを定義する抽象クラス。
//=============================================================
namespace TKM {
	class AbstractSceneFactory {
	public:
		/// <summary>
		/// <span class="code-inline">AbstractSceneFactory</span>のデストラクタ
		/// </summary>
		virtual ~AbstractSceneFactory() = default;
		/// <summary>
		/// シーンの生成
		/// </summary>
		/// <returns>生成したシーン</returns>
		/// <param name="sceneName">シーン名</param>
		virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;

	};
}