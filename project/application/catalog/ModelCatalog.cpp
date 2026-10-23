#include "ModelCatalog.h"
#include "ModelManager.h"
#include "DirectXCommon.h"
#include <string>

void ModelCatalog::LoadModelCatalogs(TKM::DirectXCommon* dxCommon) {
	// ロードするモデルファイルのリスト
	static constexpr const char* kFiles[] = {
		"sphere.obj",
	};

	auto* mm = TKM::ModelManager::GetInstance();

	// ファイルリストのモデルをすべてロードする
	for (const char* file : kFiles) {
		// すでにロードされている場合はスキップする
		mm->LoadModel(file, dxCommon);
	}
}