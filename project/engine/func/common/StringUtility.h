#pragma once
#include <string>

//=============================================================
// StringUtility名前空間
// 文字列の変換（string ⇄ wstring）を行うユーティリティ。
//=============================================================
//文字コードユーティリティ
namespace StringUtility {
	/// <summary>
	/// stringをwstringに変換します。
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	std::string ConvertString(const std::wstring& str);
	/// <summary>
	/// wstringをstringに変換します。
	/// </summary>
	/// <param name="str"></param>
	/// <returns></returns>
	std::wstring ConvertString(const std::string& str);
}

