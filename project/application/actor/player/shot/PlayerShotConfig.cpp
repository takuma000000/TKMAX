#include "PlayerShotConfig.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include "json.hpp"

using json = nlohmann::json;

bool PlayerShotConfig::HasExtension(const std::string& path, const char* ext) {
	// 拡張子指定が無い場合は判定できないのでfalseを返す
	if (!ext) {
		return false;
	}

	// 大文字小文字を無視して比較するため、パスと拡張子をコピーする
	std::string lowerPath = path;
	std::string lowerExt = ext;

	// ファイルパス側を小文字に変換する
	std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	// 拡張子側も小文字に変換する
	std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	// パスの方が拡張子より短い場合は一致しない
	if (lowerPath.size() < lowerExt.size()) {
		return false;
	}

	// パス末尾が指定拡張子と一致するか確認する
	return lowerPath.compare(lowerPath.size() - lowerExt.size(), lowerExt.size(), lowerExt) == 0;
}

bool PlayerShotConfig::Load(const char* path) {
	// パスが無い場合は読み込めない
	if (!path) {
		return false;
	}

	// 拡張子判定に使うためstd::stringへ変換する
	const std::string path_ = path;

	// JSONファイルならJSON読み込み処理へ進む
	if (HasExtension(path_, ".json")) {
		return LoadJson(path);
	}

	// 対応していない拡張子なら読み込み失敗
	return false;
}

bool PlayerShotConfig::LoadJson(const char* path) {
	// パスが無い場合は読み込めない
	if (!path) {
		return false;
	}

	// JSONファイルを開く
	std::ifstream ifs(path);

	// ファイルが開けなければ読み込み失敗
	if (!ifs.is_open()) {
		return false;
	}

	// JSONとして読み込む
	json root;
	ifs >> root;

	//=========================================================
	// RB弾設定読み込み
	//=========================================================
	if (root.contains("rb")) {
		auto& rb = root["rb"];

		if (rb.contains("bulletSpeed")) {
			rb_.bulletSpeed_ = rb["bulletSpeed"].get<float>();
		}

		if (rb.contains("shotCooldownSec")) {
			rb_.shotCooldownSec_ = rb["shotCooldownSec"].get<float>();
		}
	}

	//=========================================================
	// LB弾設定読み込み
	//=========================================================
	if (root.contains("lb")) {
		// lb設定オブジェクトを取得する
		auto& lb = root["lb"];

		// LB弾の最大弾数を読み込む
		if (lb.contains("ammoMax")) {
			lb_.ammoMax_ = lb["ammoMax"].get<int>();
		}

		// LB弾の回復開始までの待ち時間を読み込む
		if (lb.contains("refillWaitSec")) {
			lb_.refillWaitSec_ = lb["refillWaitSec"].get<float>();
		}

		// LB弾の山なり弾道の高さを読み込む
		if (lb.contains("arcHeight")) {
			lb_.arcHeight_ = lb["arcHeight"].get<float>();
		}

		// LB弾の山なり弾道にかける時間を読み込む
		if (lb.contains("arcDuration")) {
			lb_.arcDuration_ = lb["arcDuration"].get<float>();
		}

		// LB弾の前方オフセット距離を読み込む
		if (lb.contains("forwardOffsetZ")) {
			lb_.forwardOffsetZ_ = lb["forwardOffsetZ"].get<float>();
		}
	}

	// 読み込み成功
	return true;
}