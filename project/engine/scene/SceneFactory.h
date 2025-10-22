#pragma once
#include "AbstractSceneFactory.h"
#include "Framework.h"

//=============================================================
// SceneFactoryクラス
// シーン生成を行うクラス。
//=============================================================
class SceneFactory : public AbstractSceneFactory
{
public:
	/// <summary>
	/// <span class="code-inline">SceneFactory</span>のコンストラクタ
	/// </summary>
	/// <param name="dxCommon"></param>
	/// <param name="srvManager"></param>
	SceneFactory(DirectXCommon* dxCommon, SrvManager* srvManager)
		: dxCommon(dxCommon), srvManager(srvManager) {
	}

	/// <summary>
	///	</span class="code-inline">CreateScene</span>シーンの生成
	/// </summary>
	/// <param name="sceneName"></param>
	/// <returns></returns>
	BaseScene* CreateScene(const std::string& sceneName) override;

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

};

