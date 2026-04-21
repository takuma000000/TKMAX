#include "EnemyEncounterConfig.h"
#include <cstdlib>
#include <CsvReader.h>
#include <algorithm>
#include <cctype>
#include "json.hpp"
using json = nlohmann::json;

bool EnemyEncounterConfig::StrEq(const std::string& a, const char* b) {
	return a == b;
}

float EnemyEncounterConfig::ToF(const std::string& s) {
	return std::stof(s);
}

int EnemyEncounterConfig::ToI(const std::string& s) {
	return std::stoi(s);
}

bool EnemyEncounterConfig::HasExtension(const std::string& path, const char* ext) {
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
	if (t == "movetotarget") { return EnemyBehavior::MoveToTarget; }

	return fallback;
}

bool EnemyEncounterConfig::Load(const char* path) {
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

bool EnemyEncounterConfig::LoadCsv(const char* path) {
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
				if (StrEq(id_, "spawnInterval")) { smallEnemyPhase_.spawnInterval_ = ToF(get_(3)); }
				// Wave1,Settings,maxSimultaneous,a,b,c,...
				if (StrEq(id_, "maxSimultaneous")) { smallEnemyPhase_.maxSimultaneous_ = ToI(get_(3)); }
				// Wave1,Settings,defeatTarget,a,b,c,...
				if (StrEq(id_, "defeatTarget")) { smallEnemyPhase_.defeatTarget_ = ToI(get_(3)); }
			} else if (StrEq(type_, "SpawnPos") && StrEq(id_, "base")) {
				// Wave1,SpawnPos,base,a,b,c,...
				smallEnemyPhase_.baseY_ = ToF(get_(4));
				smallEnemyPhase_.baseZ_ = ToF(get_(5));
			} else if (StrEq(type_, "RandX") && StrEq(id_, "range")) {
				smallEnemyPhase_.randXMin_ = ToF(get_(4));
				smallEnemyPhase_.randXMax_ = ToF(get_(5));
			}
			// 敵のパラメータ行（type=EnemyParams, id=default）を処理
			if (StrEq(type_, "EnemyParams") && StrEq(id_, "default")) {
				mainEnemyParams_.model_ = get_(3);
				mainEnemyParams_.hp_ = ToI(get_(4));
				mainEnemyParams_.startY_ = ToF(get_(5));
				mainEnemyParams_.targetForwardZ_ = ToF(get_(6));
				mainEnemyParams_.apexY_ = ToF(get_(7));
				mainEnemyParams_.pounceTime_ = ToF(get_(8));
				mainEnemyParams_.behavior_ = ParseBehavior(get_(11), mainEnemyParams_.behavior_);
			}
		}

	}
	return true;
}

bool EnemyEncounterConfig::LoadJson(const char* path) {
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
	if (root.contains("smallEnemyPhase")) {
		auto& w = root["smallEnemyPhase"];

		if (w.contains("settings")) {
			auto& s = w["settings"];

			if (s.contains("spawnInterval")) {
				smallEnemyPhase_.spawnInterval_ = s["spawnInterval"].get<float>();
			}
			if (s.contains("maxSimultaneous")) {
				smallEnemyPhase_.maxSimultaneous_ = s["maxSimultaneous"].get<int>();
			}
			if (s.contains("defeatTarget")) {
				smallEnemyPhase_.defeatTarget_ = s["defeatTarget"].get<int>();
			}
		}

		if (w.contains("spawn")) {
			auto& s = w["spawn"];

			if (s.contains("baseY")) {
				smallEnemyPhase_.baseY_ = s["baseY"].get<float>();
			}
			if (s.contains("baseZ")) {
				smallEnemyPhase_.baseZ_ = s["baseZ"].get<float>();
			}
			if (s.contains("randXMin")) {
				smallEnemyPhase_.randXMin_ = s["randXMin"].get<float>();
			}
			if (s.contains("randXMax")) {
				smallEnemyPhase_.randXMax_ = s["randXMax"].get<float>();
			}
		}

		if (w.contains("enemyParams")) {
			auto& e = w["enemyParams"];

			if (e.contains("model")) {
				mainEnemyParams_.model_ = e["model"].get<std::string>();
			}
			if (e.contains("hp")) {
				mainEnemyParams_.hp_ = e["hp"].get<int>();
			}
			if (e.contains("startY")) {
				mainEnemyParams_.startY_ = e["startY"].get<float>();
			}
			if (e.contains("targetForwardZ")) {
				mainEnemyParams_.targetForwardZ_ = e["targetForwardZ"].get<float>();
			}
			if (e.contains("apexY")) {
				mainEnemyParams_.apexY_ = e["apexY"].get<float>();
			}
			if (e.contains("pounceTime")) {
				mainEnemyParams_.pounceTime_ = e["pounceTime"].get<float>();
			}
			if (e.contains("behavior")) {
				mainEnemyParams_.behavior_ =
					ParseBehavior(e["behavior"].get<std::string>(), mainEnemyParams_.behavior_);
			}
		}
	}

	return true;
}