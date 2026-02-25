#pragma once
#include <string>
#include <initializer_list>

//=============================================================
// TextureCatalogクラス
// テクスチャのカタログ（目録）クラス。
// 使うテクスチャをまとめてロードするためのクラスです。
//=============================================================
class TextureCatalog {
public:
	// 使うテクスチャをまとめてロード
	static void LoadTextureCatalogs();
};
