#pragma once
#include <string>
#include "HintLog.h"

class HintLogExporter {
public:
	static bool SaveJson(
		const HintLog& hintLog,
		const std::string& filePath
	);
};