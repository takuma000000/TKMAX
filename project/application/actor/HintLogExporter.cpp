#include "HintLogExporter.h"
#include <fstream>

bool HintLogExporter::SaveJson(
	const HintLog& hintLog,
	const std::string& filePath
) {
	std::ofstream file(filePath);

	if (!file.is_open()) {
		return false;
	}

	file << hintLog.MakeJson();

	file.close();

	return true;
}