#include "CsvSpawnLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

static inline std::string Trim(std::string s) {
	auto notSpace = [](unsigned char c) { return !std::isspace(c); };
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
	s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
	return s;
}

static inline bool ParseBool01(const std::string& s) {
	std::string t = Trim(s);
	return (t == "1" || t == "true" || t == "TRUE");
}

static inline float ParseFloatOr0(const std::string& s) {
	std::string t = Trim(s);
	if (t.empty()) { return 0.0f; }
	return std::stof(t);
}

static inline int ParseIntOr0(const std::string& s) {
	std::string t = Trim(s);
	if (t.empty()) { return 0; }
	return std::stoi(t);
}

static inline std::vector<std::string> SplitCsvLine(const std::string& line) {
	// ※今回の課題は「カンマ区切り、引用符なし」前提で十分
	std::vector<std::string> cols;
	std::stringstream ss(line);
	std::string cell;
	while (std::getline(ss, cell, ',')) {
		cols.push_back(cell);
	}
	return cols;
}

bool CsvSpawnLoader::Load(const std::string& filepath, std::vector<SpawnEvent>& out) {
	out.clear();

	std::ifstream ifs(filepath);
	if (!ifs.is_open()) {
		return false;
	}

	std::string line;
	bool firstLine = true;

	while (std::getline(ifs, line)) {
		line = Trim(line);
		if (line.empty()) { continue; }
		if (line.size() >= 2 && line[0] == '/' && line[1] == '/') { continue; } // コメント行

		if (firstLine) {
			firstLine = false; // ヘッダは読み飛ばす
			continue;
		}

		auto cols = SplitCsvLine(line);
		// 期待: wave,time,pattern,x,y,z,count,spacing,spawnFx,paramA,paramB,paramC
		if (cols.size() < 9) { continue; }

		SpawnEvent e{};
		e.wave = ParseIntOr0(cols[0]);
		e.time = ParseFloatOr0(cols[1]);
		e.pattern = Trim(cols[2]);

		e.x = ParseFloatOr0(cols[3]);
		e.y = ParseFloatOr0(cols[4]);
		e.z = ParseFloatOr0(cols[5]);

		e.count = ParseIntOr0(cols[6]);
		e.spacing = ParseFloatOr0(cols[7]);
		e.spawnFx = ParseBool01(cols[8]);

		if (cols.size() > 9) { e.paramA = ParseFloatOr0(cols[9]); }
		if (cols.size() > 10) { e.paramB = ParseFloatOr0(cols[10]); }
		if (cols.size() > 11) { e.paramC = ParseFloatOr0(cols[11]); }

		out.push_back(e);
	}

	// 念のため wave → time 順でソート（CSVの並びが崩れても動く）
	std::sort(out.begin(), out.end(), [](const SpawnEvent& a, const SpawnEvent& b) {
		if (a.wave != b.wave) { return a.wave < b.wave; }
		return a.time < b.time;
		});

	return true;
}