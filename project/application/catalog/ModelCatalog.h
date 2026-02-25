#pragma once

namespace TKM {
	class DirectXCommon;
}

//=============================================================
// ModelCatalogクラス
// モデルのカタログ（目録）クラス。
// 使うモデルをまとめてロードするためのクラスです。
//=============================================================
class ModelCatalog {
public:
	// 使うモデルをまとめてロード
	static void LoadModelCatalogs(TKM::DirectXCommon* dxCommon);
};