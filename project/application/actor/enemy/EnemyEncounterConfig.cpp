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

static Vector3 ParseVector3(const json& j, const Vector3& fallback) {
	Vector3 result = fallback; // JSONがオブジェクトでない場合は既存値を維持して返す

	// JSONがオブジェクトでない場合は既存値を維持して返す
	if (!j.is_object()) {
		return result;
	}
	// x, y, z の各要素が存在する場合のみ値を更新する
	if (j.contains("x")) {
		result.x = j["x"].get<float>();
	}
	if (j.contains("y")) {
		result.y = j["y"].get<float>();
	}
	if (j.contains("z")) {
		result.z = j["z"].get<float>();
	}

	return result; // JSONに必要な要素がない場合は既存値を維持して返す
}

static EnemyType ParseEnemyType(const std::string& s, EnemyType fallback) {
	// 空文字や"0"なら既存値をそのまま使う
	if (s.empty()) {
		return fallback;
	}

	// 小文字に統一して比較しやすくする
	std::string t = s;
	// 小文字に変換して比較しやすくする
	std::transform(t.begin(), t.end(), t.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (t == "mainsquad") { return EnemyType::MainSquad; } // デフォルト値と同じなので明示的に指定された場合も MainSquad を返す
	if (t == "boss") { return EnemyType::Boss; } // 該当しない場合は既存の値を維持

	return fallback; // 該当しない場合は既存の値を維持
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
			// Settings
			//-------------------------
			if (StrEq(type_, "Settings")) {
				// 撃破目標数
				if (StrEq(id_, "defeatTarget")) {
					smallEnemyPhase_.defeatTarget_ = ToI(get_(3));
				}

			}
			//-------------------------
			// 敵パラメータ
			//-------------------------
			if (StrEq(type_, "EnemyParams") && StrEq(id_, "default")) {

				mainEnemyParams_.model_ = get_(3);          // モデル名
				mainEnemyParams_.hp_ = ToI(get_(4));        // HP
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

			if (s.contains("defeatTarget")) {
				smallEnemyPhase_.defeatTarget_ = s["defeatTarget"].get<int>();
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
			if (e.contains("scale")) {
				mainEnemyParams_.scale_ =
					ParseVector3(e["scale"], mainEnemyParams_.scale_);
			}
			if (e.contains("type")) {
				mainEnemyParams_.type_ =
					ParseEnemyType(e["type"].get<std::string>(), mainEnemyParams_.type_);
			}
			if (e.contains("tentacle")) {
				auto& t = e["tentacle"];

				if (t.contains("enabled")) {
					mainEnemyParams_.useTentacle_ = t["enabled"].get<bool>();
				}
				if (t.contains("model")) {
					mainEnemyParams_.tentacleModel_ = t["model"].get<std::string>();
				}
				if (t.contains("localPosition")) {
					mainEnemyParams_.tentacleLocalPosition_ =
						ParseVector3(t["localPosition"], mainEnemyParams_.tentacleLocalPosition_);
				}
				if (t.contains("localRotation")) {
					mainEnemyParams_.tentacleLocalRotation_ =
						ParseVector3(t["localRotation"], mainEnemyParams_.tentacleLocalRotation_);
				}
				if (t.contains("localScale")) {
					mainEnemyParams_.tentacleLocalScale_ =
						ParseVector3(t["localScale"], mainEnemyParams_.tentacleLocalScale_);
				}
			}
			if (e.contains("formation")) {
				auto& f = e["formation"];

				if (f.contains("count")) {
					mainEnemyParams_.formationCount_ = f["count"].get<int>();
				}
				if (f.contains("center")) {
					mainEnemyParams_.formationCenter_ =
						ParseVector3(f["center"], mainEnemyParams_.formationCenter_);
				}
				if (f.contains("orbitRadius")) {
					mainEnemyParams_.formationOrbitRadius_ = f["orbitRadius"].get<float>();
				}
				if (f.contains("orbitAngularSpeed")) {
					mainEnemyParams_.formationOrbitAngularSpeed_ = f["orbitAngularSpeed"].get<float>();
				}
				if (f.contains("followSpeed")) {
					mainEnemyParams_.formationFollowSpeed_ = f["followSpeed"].get<float>();
				}
			}
		}
	}

	return true;
}