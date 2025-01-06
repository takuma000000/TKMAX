#pragma once
#include "AbstractSceneFactory.h"
#include "Framework.h"

class SceneFactory : public AbstractSceneFactory
{
public:
	SceneFactory(DirectXCommon* dxCommon, SrvManager* srvManager)
		: dxCommon(dxCommon), srvManager(srvManager) {
	}

	BaseScene* CreateScene(const std::string& sceneName) override;

private:
	DirectXCommon* dxCommon = nullptr;
	SrvManager* srvManager = nullptr;

};

