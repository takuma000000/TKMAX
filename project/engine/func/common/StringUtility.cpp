#include "StringUtility.h"
#include <windows.h>

namespace StringUtility {
	std::wstring ConvertString(const std::string& str) { // UTF-8 -> UTF-16
		if (str.empty()) { // 空文字チェック
			return std::wstring(); // 空のワイド文字列を返す
		}

		auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0); // 必要なサイズを取得
		if (sizeNeeded == 0) { // エラーチェック
			return std::wstring(); // 空のワイド文字列を返す
		}
		std::wstring result(sizeNeeded, 0); // 結果用のワイド文字列を確保
		// 変換実行
		MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
		return result;
	}

	std::string ConvertString(const std::wstring& str) { // UTF-16 -> UTF-8
		if (str.empty()) { // 空文字チェック
			return std::string(); // 空の文字列を返す
		}

		auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL); // 必要なサイズを取得
		if (sizeNeeded == 0) { // エラーチェック
			return std::string(); // 空の文字列を返す
		}
		std::string result(sizeNeeded, 0); // 結果用の文字列を確保
		// 変換実行
		WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
		return result;
	}
}