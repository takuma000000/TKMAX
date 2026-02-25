#pragma once
#include <string>
#include <vector>

namespace TKM {

	//=============================================================
	// CsvReaderクラス
	// CSVファイルの読み込みと解析を行うクラス。
	//=============================================================
	class CsvReader {
	public:
		/// <summary>
		/// 1行分の文字列を解析し、カンマ区切りで分割して返します
		/// </summary>
		/// <param name="line"></param>
		/// <param name="out"></param>
		/// <returns></returns>
		static bool ParseLine(const std::string& line, std::vector<std::string>& out);
		/// <summary>
		/// CSVファイルを読み込み、行ごとに分割して返します
		/// </summary>
		/// <param name="path"></param>
		/// <param name="rows"></param>
		/// <returns></returns>
		static bool ReadFile(const char* path, std::vector<std::vector<std::string>>& rows);
	private:
		/// <summary>
		/// 文字列の前後の空白・タブを削除します
		/// </summary>
		/// <param name="s"></param>
		static void Trim(std::string& s);
		/// <summary>
		/// 文字列からコメント部分を削除します
		/// </summary>
		/// <param name="s"></param>
		static void StripComment(std::string& s);
		/// <summary>
		/// 文字列をカンマで分割します
		/// </summary>
		/// <param name="s"></param>
		/// <param name="out"></param>
		static void SplitComma(const std::string& s, std::vector<std::string>& out);
	};
}