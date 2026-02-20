#pragma once

namespace TKM {
	class DirectXCommon;
}

class ModelCatalog {
public:
	// 使うモデルをまとめてロード
	static void LoadModelCatalogs(TKM::DirectXCommon* dxCommon);
};