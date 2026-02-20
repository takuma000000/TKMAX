#include "TextureCatalog.h"
#include "TextureManager.h"
#include <string>

void TextureCatalog::LoadTextureCatalogs() {
	// ここだけ見れば「何を読むか」が全部分かる状態にする
	static constexpr const char* kBaseDir = "./resources/texture/";

	// ファイル名だけ並べる（ベースパスは共通化）
	static constexpr const char* kFiles[] = {
		"uvChecker.png",
		"circle.png",
		"circle2.png",
		"gradationLine.png",
		"rostock_laage_airport_4k.dds",
		"test.dds",
		"kloofendal_48d_partly_cloudy_puresky_1k.dds",
		"Ground.png",
		"start.png",
		"damageSpark.png",
		"firework_star.png",
		"RB_ui.png",
		"LB_ui.png",
		"X_ui.png",
		"LS_ui.png",
		"resume_pause.png",
		"restart_pause.png",
		"title_pause.png",
		"gauge_fill_grad.png",
		"gauge_frame_glass.png",
		"RB_gauge_ui.png",
		"gauge_shard.jpeg",
		"blue.dds",
		"gray.jpg",
		"gauge_green.jpg",
		"player_hp.jpg",
		"player_hp_frame.jpg",
		"player_hp.png",
		"uvChecker.dds",
	};

	// TKM::TextureManager はシングルトン
	auto* tm = TKM::TextureManager::GetInstance();

	for (const char* file : kFiles) {
		std::string path = std::string(kBaseDir) + file;
		tm->LoadTexture(path);
	}
}
