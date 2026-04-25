#include "ModelCatalog.h"
#include "ModelManager.h"
#include "DirectXCommon.h"
#include <string>

void ModelCatalog::LoadModelCatalogs(TKM::DirectXCommon* dxCommon) {
	// ロードするモデルファイルのリスト
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
		"normalBullet.obj",
		"barrierCore.obj",
	};

	auto* mm = TKM::ModelManager::GetInstance();

	// ファイルリストのモデルをすべてロードする
	for (const char* file : kFiles) {
		// すでにロードされている場合はスキップする
		mm->LoadModel(file, dxCommon);
	}
}