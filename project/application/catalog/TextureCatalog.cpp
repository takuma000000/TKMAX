#include "TextureCatalog.h"
#include "TextureManager.h"
#include <string>

void TextureCatalog::LoadTextureCatalogs() {
	// ベースディレクトリを定数化
	static constexpr const char* kBaseDir = "./resources/texture/";

	// ファイル名だけ並べる（ベースパスは共通化）
	static constexpr const char* kFiles[] = {
		"uvChecker.png",
		"circle.png",
		"circle2.png",
		"gradationLine.png",
		"rostock_laage_airport_4k.dds",
		"test.dds",
		"uvChecker.dds",
	};

	// TKM::TextureManager はシングルトン
	auto* tm = TKM::TextureManager::GetInstance();

	// ファイルリストのテクスチャをすべてロードする
	for (const char* file : kFiles) {
		// すでにロードされている場合はスキップする
		std::string path = std::string(kBaseDir) + file;
		tm->LoadTexture(path); // ロードする際はベースディレクトリを付加する
	}
}
