#include "ModelCatalog.h"
#include "ModelManager.h"
#include "DirectXCommon.h"
#include <string>

void ModelCatalog::LoadModelCatalogs(TKM::DirectXCommon* dxCommon) {
	// ここだけ見れば「何を読むか」が全部分かる状態にする

	static constexpr const char* kFiles[] = {
		"sphere.obj",
		"turtle.obj",
		"turtle_flipper.obj",
		"jerryfish.obj",
		"jerryfish_boss.obj",
		"tentacle.obj",
		"tentacle_boss.obj",
		"reticle_big.obj",
		"reticle_normal.obj",
		"reticle_small.obj",
	};

	auto* mm = TKM::ModelManager::GetInstance();

	for (const char* file : kFiles) {
		mm->LoadModel(file, dxCommon);
	}
}