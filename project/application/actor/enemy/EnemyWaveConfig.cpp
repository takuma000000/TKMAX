#include "EnemyWaveConfig.h"
#include <cstdlib>
#include <CsvReader.h>
#include <algorithm>
#include <cctype>
#include "json.hpp"
using json = nlohmann::json;

bool EnemyWaveConfig::StrEq(const std::string& a, const char* b) {
	return a == b;
}

float EnemyWaveConfig::ToF(const std::string& s) {
	return std::stof(s);
}

int EnemyWaveConfig::ToI(const std::string& s) {
	return std::stoi(s);
}

bool EnemyWaveConfig::HasExtension(const std::string& path, const char* ext) {
	if (!ext) {
		return false;
	}

	std::string lowerPath = path;
	std::string lowerExt = ext;

	std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (lowerPath.size() < lowerExt.size()) {
		return false;
	}

	return lowerPath.compare(lowerPath.size() - lowerExt.size(), lowerExt.size(), lowerExt) == 0;
}

static EnemyBehavior ParseBehavior(const std::string& s, EnemyBehavior fallback) {
	if (s.empty() || s == "0") { return fallback; }

	std::string t = s;
	std::transform(t.begin(), t.end(), t.begin(),
		[](unsigned char c) { return (char)std::tolower(c); });

	// 敵の行動パターンを表す文字列を EnemyBehavior 列挙型に変換
	if (t == "pouncefromabove") { return EnemyBehavior::PounceFromAbove; }
	if (t == "sinex") { return EnemyBehavior::SineX; }
	if (t == "straightstop") { return EnemyBehavior::StraightStop; }
	if (t == "freeroam") { return EnemyBehavior::FreeRoam; }

	return fallback;
}

bool EnemyWaveConfig::Load(const char* path) {
	if (!path) {
		return false;
	}

	const std::string path_ = path;

	if (HasExtension(path_, ".csv")) {
		return LoadCsv(path);
	}

	if (HasExtension(path_, ".json")) {
		return LoadJson(path);
	}

	return false;
}

bool EnemyWaveConfig::LoadCsv(const char* path) {
	std::vector<std::vector<std::string>> rows_;
	// CSVファイルを読み込む
	if (!TKM::CsvReader::ReadFile(path, rows_)) {
		return false;
	}
	// 読み込んだ行を1行ずつ処理
	for (const auto& c : rows_) {
		// wave,type,id,a,b,c,d,e,f みたいな固定列
		if (c.size() < 3) { continue; }

		const std::string& wave_ = c[0];
		const std::string& type_ = c[1];
		const std::string& id_ = c[2];

		auto get_ = [&](size_t idx) -> std::string {
			if (idx < c.size()) { return c[idx]; }
			return "0";
			};

		// ---- Wave1 ----
		if (StrEq(wave_, "Wave1")) {
			// Wave1の設定行を処理
			if (StrEq(type_, "Settings")) {
				// Wave1,Settings,spawnInterval,a,b,c,...
				if (StrEq(id_, "spawnInterval")) { wave1_.spawnInterval_ = ToF(get_(3)); }
				// Wave1,Settings,maxSimultaneous,a,b,c,...
				if (StrEq(id_, "maxSimultaneous")) { wave1_.maxSimultaneous_ = ToI(get_(3)); }
				// Wave1,Settings,defeatTarget,a,b,c,...
				if (StrEq(id_, "defeatTarget")) { wave1_.defeatTarget_ = ToI(get_(3)); }
			} else if (StrEq(type_, "SpawnPos") && StrEq(id_, "base")) {
				// Wave1,SpawnPos,base,a,b,c,...
				wave1_.baseY_ = ToF(get_(4));
				wave1_.baseZ_ = ToF(get_(5));
			} else if (StrEq(type_, "RandX") && StrEq(id_, "range")) {
				wave1_.randXMin_ = ToF(get_(4));
				wave1_.randXMax_ = ToF(get_(5));
			}
			// 敵のパラメータ行（type=EnemyParams, id=default）を処理
			if (StrEq(type_, "EnemyParams") && StrEq(id_, "default")) {
				wave1EnemyParams_.model_ = get_(3);
				wave1EnemyParams_.hp_ = ToI(get_(4));
				wave1EnemyParams_.startY_ = ToF(get_(5));
				wave1EnemyParams_.targetForwardZ_ = ToF(get_(6));
				wave1EnemyParams_.apexY_ = ToF(get_(7));
				wave1EnemyParams_.pounceTime_ = ToF(get_(8));
				wave1EnemyParams_.behavior_ = ParseBehavior(get_(11), wave1EnemyParams_.behavior_);
			}
		}

	}
	return true;
}

bool EnemyWaveConfig::LoadJson(const char* path) {
	if (!path) {
		return false;
	}

	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		return false;
	}

	json root;
	ifs >> root;

	// -------------------------
	// Wave1
	// -------------------------
	if (root.contains("wave1")) {
		auto& w = root["wave1"];

		if (w.contains("settings")) {
			auto& s = w["settings"];

			if (s.contains("spawnInterval")) {
				wave1_.spawnInterval_ = s["spawnInterval"].get<float>();
			}
			if (s.contains("maxSimultaneous")) {
				wave1_.maxSimultaneous_ = s["maxSimultaneous"].get<int>();
			}
			if (s.contains("defeatTarget")) {
				wave1_.defeatTarget_ = s["defeatTarget"].get<int>();
			}
		}

		if (w.contains("spawn")) {
			auto& s = w["spawn"];

			if (s.contains("baseY")) {
				wave1_.baseY_ = s["baseY"].get<float>();
			}
			if (s.contains("baseZ")) {
				wave1_.baseZ_ = s["baseZ"].get<float>();
			}
			if (s.contains("randXMin")) {
				wave1_.randXMin_ = s["randXMin"].get<float>();
			}
			if (s.contains("randXMax")) {
				wave1_.randXMax_ = s["randXMax"].get<float>();
			}
		}

		if (w.contains("enemyParams")) {
			auto& e = w["enemyParams"];

			if (e.contains("model")) {
				wave1EnemyParams_.model_ = e["model"].get<std::string>();
			}
			if (e.contains("hp")) {
				wave1EnemyParams_.hp_ = e["hp"].get<int>();
			}
			if (e.contains("startY")) {
				wave1EnemyParams_.startY_ = e["startY"].get<float>();
			}
			if (e.contains("targetForwardZ")) {
				wave1EnemyParams_.targetForwardZ_ = e["targetForwardZ"].get<float>();
			}
			if (e.contains("apexY")) {
				wave1EnemyParams_.apexY_ = e["apexY"].get<float>();
			}
			if (e.contains("pounceTime")) {
				wave1EnemyParams_.pounceTime_ = e["pounceTime"].get<float>();
			}
			if (e.contains("behavior")) {
				wave1EnemyParams_.behavior_ =
					ParseBehavior(e["behavior"].get<std::string>(), wave1EnemyParams_.behavior_);
			}
		}
	}

	return true;
}