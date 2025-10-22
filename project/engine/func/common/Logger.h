#pragma once
#include <string>

//=============================================================
// Logger名前空間
// デバッグログの出力を行うユーティリティ。
//=============================================================

//ログ出力
///<summary>ログメッセージを出力します。</summary>
namespace Logger {
	void Log(const std::string& message);
};

