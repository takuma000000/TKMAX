#pragma once
#include <string>

//=============================================================
// StringUtility名前空間
// 文字列の変換（string ⇄ wstring）を行うユーティリティ。
//=============================================================
//文字コードユーティリティ
namespace StringUtility {
	//stringをwstringに変換する
	/// <summary>stringをwstringに変換します。</summary>
	std::string ConvertString(const std::wstring& str);
	//wstringをstringに変換する
	/// <summary>wstringをstringに変換します。</summary>
	std::wstring ConvertString(const std::string& str);
}

