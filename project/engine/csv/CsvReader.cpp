#include "CsvReader.h"
#include <fstream>

namespace TKM {

	void CsvReader::Trim(std::string& s) {
		// 先頭の空白・タブを削除する
		while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
			s.erase(s.begin());
		}

		// 末尾の空白・タブ・改行コードを削除する
		while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) {
			s.pop_back();
		}
	}

	void CsvReader::StripComment(std::string& s) {
		// 「#」以降をコメントとして削除する（CSV内説明用）
		const size_t p = s.find('#');
		if (p != std::string::npos) {
			s = s.substr(0, p);
		}
	}

	void CsvReader::SplitComma(const std::string& s, std::vector<std::string>& out) {
		// 出力配列をクリアする
		out.clear();

		std::string cur;
		bool inQuote = false;

		// 1文字ずつ走査してカンマ区切りを分解する
		for (size_t i = 0; i < s.size(); ++i) {
			char c = s[i];

			// ダブルクォーテーションの開始・終了をトグル管理
			if (c == '"') {
				inQuote = !inQuote;
				continue;
			}

			// クォート外でカンマが来たら区切りとして扱う
			if (!inQuote && c == ',') {
				Trim(cur);              // 要素の前後空白を削除
				out.push_back(cur);     // 配列へ追加
				cur.clear();            // 次の要素のためにリセット
			} else {
				// 通常文字は現在の要素へ追加
				cur.push_back(c);
			}
		}

		// 最後の要素を処理
		Trim(cur);
		if (!cur.empty()) {
			out.push_back(cur);
		}
	}

	bool CsvReader::ParseLine(const std::string& line, std::vector<std::string>& out) {
		// 元の行をコピーして加工用に使う
		std::string s = line;

		// コメント部分を削除する
		StripComment(s);

		// 前後の空白を削除する
		Trim(s);

		// 空行は無視する
		if (s.empty()) {
			return false;
		}

		// 行頭がコメントなら無視する
		if (!s.empty() && s[0] == '#') {
			return false;
		}

		// カンマ区切りで分解する
		SplitComma(s, out);

		// 要素が1つ以上あれば有効行とみなす
		return !out.empty();
	}

	bool CsvReader::ReadFile(const char* path, std::vector<std::vector<std::string>>& rows) {
		// 出力行配列を初期化する
		rows.clear();

		// ファイルを開く
		std::ifstream ifs(path);
		if (!ifs.is_open()) {
			return false; // 開けなかったら失敗
		}

		std::string line;
		std::vector<std::string> cols;

		// 1行ずつ読み込む
		while (std::getline(ifs, line)) {

			// 行を解析して、有効なCSV行なら格納する
			if (!ParseLine(line, cols)) {
				continue;
			}

			rows.push_back(cols);
		}

		return true;
	}

} // namespace TKM