#include "EnemyEncounterConfig.h"
#include <cstdlib>
#include <CsvReader.h>
#include <algorithm>
#include <cctype>
#include "json.hpp"
using json = nlohmann::json;

// 文字列比較（std::string と const char* の簡易比較用）
bool EnemyEncounterConfig::StrEq(const std::string& a, const char* b) {
	return a == b;
}

// 文字列をfloatに変換する
float EnemyEncounterConfig::ToF(const std::string& s) {
	return std::stof(s);
}

// 文字列をintに変換する
int EnemyEncounterConfig::ToI(const std::string& s) {
	return std::stoi(s);
}

// ファイルパスが指定拡張子を持っているか判定する（大文字小文字は無視）
bool EnemyEncounterConfig::HasExtension(const std::string& path, const char* ext) {
	if (!ext) {
		return false;
	}

	// 比較用に両方を小文字に変換する
	std::string lowerPath = path;
	std::string lowerExt = ext;

	std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	// パスの方が短い場合は一致しない
	if (lowerPath.size() < lowerExt.size()) {
		return false;
	}

	// 末尾一致で拡張子を判定する
	return lowerPath.compare(lowerPath.size() - lowerExt.size(), lowerExt.size(), lowerExt) == 0;
}

// 文字列からEnemyBehavior列挙型へ変換する
static EnemyBehavior ParseBehavior(const std::string& s, EnemyBehavior fallback) {
	// 空文字や"0"なら既存値をそのまま使う
	if (s.empty() || s == "0") { return fallback; }

	// 小文字に統一して比較しやすくする
	std::string t = s;
	std::transform(t.begin(), t.end(), t.begin(),
		[](unsigned char c) { return (char)std::tolower(c); });

	// 文字列に応じて行動パターンへ変換
	if (t == "pouncefromabove") { return EnemyBehavior::PounceFromAbove; }
	if (t == "movetotarget") { return EnemyBehavior::MoveToTarget; }

	// 該当しない場合は既存の値を維持
	return fallback;
}

// ファイル拡張子に応じてCSVかJSONかを振り分けて読み込む
bool EnemyEncounterConfig::Load(const char* path) {
	if (!path) {
		return false;
	}

	const std::string path_ = path;

	// CSVならCSVロードへ
	if (HasExtension(path_, ".csv")) {
		return LoadCsv(path);
	}

	// JSONならJSONロードへ
	if (HasExtension(path_, ".json")) {
		return LoadJson(path);
	}

	// 対応外拡張子
	return false;
}

//=========================================================
// CSV読み込み
//=========================================================
bool EnemyEncounterConfig::LoadCsv(const char* path) {

	std::vector<std::vector<std::string>> rows_;

	// CSVファイルを全行読み込む
	if (!TKM::CsvReader::ReadFile(path, rows_)) {
		return false;
	}

	// 行ごとにパースして設定へ反映する
	for (const auto& c : rows_) {

		// 最低限 wave,type,id がない行は無視
		if (c.size() < 3) { continue; }

		const std::string& wave_ = c[0];
		const std::string& type_ = c[1];
		const std::string& id_ = c[2];

		// 安全に列を取得するためのヘルパー（範囲外は"0"）
		auto get_ = [&](size_t idx) -> std::string {
			if (idx < c.size()) { return c[idx]; }
			return "0";
			};

		//=====================================================
		// Wave1設定
		//=====================================================
		if (StrEq(wave_, "Wave1")) {

			//-------------------------
			// Wave1 Settings
			//-------------------------
			if (StrEq(type_, "Settings")) {

				// 出現間隔
				if (StrEq(id_, "spawnInterval")) {
					smallEnemyPhase_.spawnInterval_ = ToF(get_(3));
				}

				// 同時出現数
				if (StrEq(id_, "maxSimultaneous")) {
					smallEnemyPhase_.maxSimultaneous_ = ToI(get_(3));
				}

				// 撃破目標数
				if (StrEq(id_, "defeatTarget")) {
					smallEnemyPhase_.defeatTarget_ = ToI(get_(3));
				}

			}
			//-------------------------
			// 出現位置ベース
			//-------------------------
			else if (StrEq(type_, "SpawnPos") && StrEq(id_, "base")) {

				// Y座標基準
				smallEnemyPhase_.baseY_ = ToF(get_(4));

				// Z座標基準
				smallEnemyPhase_.baseZ_ = ToF(get_(5));
			}
			//-------------------------
			// ランダムX範囲
			//-------------------------
			else if (StrEq(type_, "RandX") && StrEq(id_, "range")) {

				smallEnemyPhase_.randXMin_ = ToF(get_(4));
				smallEnemyPhase_.randXMax_ = ToF(get_(5));
			}

			//-------------------------
			// 敵パラメータ
			//-------------------------
			if (StrEq(type_, "EnemyParams") && StrEq(id_, "default")) {

				mainEnemyParams_.model_ = get_(3);          // モデル名
				mainEnemyParams_.hp_ = ToI(get_(4));        // HP
				mainEnemyParams_.startY_ = ToF(get_(5));    // 初期Y
				mainEnemyParams_.targetForwardZ_ = ToF(get_(6)); // 前進目標Z
				mainEnemyParams_.apexY_ = ToF(get_(7));     // 頂点Y
				mainEnemyParams_.pounceTime_ = ToF(get_(8)); // 突進時間

				// 行動パターンを文字列から変換
				mainEnemyParams_.behavior_ =
					ParseBehavior(get_(11), mainEnemyParams_.behavior_);
			}
		}
	}

	return true;
}

//=========================================================
// JSON読み込み
//=========================================================
bool EnemyEncounterConfig::LoadJson(const char* path) {

	if (!path) {
		return false;
	}

	// ファイルを開く
	std::ifstream ifs(path);

	if (!ifs.is_open()) {
		return false;
	}

	// JSONとしてパース
	json root;
	ifs >> root;

	//=====================================================
	// Wave1設定
	//=====================================================
	if (root.contains("smallEnemyPhase")) {

		auto& w = root["smallEnemyPhase"];

		//-------------------------
		// Settings
		//-------------------------
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

		//-------------------------
		// Spawn設定
		//-------------------------
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

		//-------------------------
		// 敵パラメータ
		//-------------------------
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