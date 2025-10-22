#pragma once
#include <string>

//=============================================================
// StringUtility名前空間
// 文字列の変換（string ⇄ wstring）を行うユーティリティ。
//=============================================================
//文字コードユーティリティ
namespace StringUtility {
	//stringをwstringに変換する
	std::string ConvertString(const std::wstring& str);
	//wstringをstringに変換する
	std::wstring ConvertString(const std::string& str);
}

