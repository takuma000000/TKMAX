#include "CsvReader.h"
#include <fstream>

namespace TKM {
	void CsvReader::Trim(std::string& s) {
		while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) { s.erase(s.begin()); }
		while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) { s.pop_back(); }
	}

	void CsvReader::StripComment(std::string& s) {
		// # 以降はコメント（CSV内の説明用）
		const size_t p = s.find('#');
		if (p != std::string::npos) {
			s = s.substr(0, p);
		}
	}

	void CsvReader::SplitComma(const std::string& s, std::vector<std::string>& out) {
		out.clear();
		std::string cur;
		bool inQuote = false;

		for (size_t i = 0; i < s.size(); ++i) {
			char c = s[i];
			if (c == '"') { inQuote = !inQuote; continue; }

			if (!inQuote && c == ',') {
				Trim(cur);
				out.push_back(cur);
				cur.clear();
			} else {
				cur.push_back(c);
			}
		}
		Trim(cur);
		if (!cur.empty()) {
			out.push_back(cur);
		}
	}

	bool CsvReader::ParseLine(const std::string& line, std::vector<std::string>& out) {
		std::string s = line;
		StripComment(s);
		Trim(s);

		if (s.empty()) { return false; }
		if (!s.empty() && s[0] == '#') { return false; }

		SplitComma(s, out);
		return !out.empty();
	}

	bool CsvReader::ReadFile(const char* path, std::vector<std::vector<std::string>>& rows) {
		rows.clear();

		std::ifstream ifs(path);
		if (!ifs.is_open()) {
			return false;
		}

		std::string line;
		std::vector<std::string> cols;
		while (std::getline(ifs, line)) {
			if (!ParseLine(line, cols)) { continue; }
			rows.push_back(cols);
		}

		return true;
	}
}