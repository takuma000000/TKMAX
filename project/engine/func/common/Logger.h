#pragma once
#include <string>

//=============================================================
// Logger名前空間
// デバッグログの出力を行うユーティリティ。
//=============================================================
namespace Logger {
	/// <summary>
	/// ログを出力します。
	/// </summary>
	/// <param name="message"></param>
	void Log(const std::string& message);
};

