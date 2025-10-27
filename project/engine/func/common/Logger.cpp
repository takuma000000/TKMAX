#include "Logger.h"
#include <windows.h>

namespace Logger {
	// デバッグ出力にメッセージを送る関数
	void Log(const std::string& message) {
		OutputDebugStringA(message.c_str()); // デバッグ出力にメッセージを送る
	}
}
