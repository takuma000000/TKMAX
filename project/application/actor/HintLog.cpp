#include "HintLog.h"

void HintLog::Reset() {
	selectLogs_.clear();

	missCount_ = 0;
	correctCount_ = 0;
	totalSelectCount_ = 0;
}

void HintLog::AddSelectLog(int roadIndex, bool isCorrect) {
	SelectLog log;
	log.roadIndex = roadIndex;
	log.isCorrect = isCorrect;

	selectLogs_.push_back(log);

	totalSelectCount_++;

	if (isCorrect) {
		correctCount_++;
	} else {
		missCount_++;
	}
}

std::string HintLog::MakeJson() const {
	std::string json;

	json += "{\n";
	json += "  \"stageId\":\"" + stageId_ + "\",\n";
	json += "  \"correctIndex\":" + std::to_string(correctIndex_) + ",\n";
	json += "  \"missCount\":" + std::to_string(missCount_) + ",\n";
	json += "  \"correctCount\":" + std::to_string(correctCount_) + ",\n";
	json += "  \"totalSelectCount\":" + std::to_string(totalSelectCount_) + ",\n";
	json += "  \"logs\":[\n";

	for (size_t i = 0; i < selectLogs_.size(); i++) {
		json += "    {\n";
		json += "      \"roadIndex\":" + std::to_string(selectLogs_[i].roadIndex) + ",\n";
		json += "      \"isCorrect\":";
		json += selectLogs_[i].isCorrect ? "true\n" : "false\n";
		json += "    }";

		if (i + 1 < selectLogs_.size()) {
			json += ",";
		}

		json += "\n";
	}

	json += "  ]\n";
	json += "}";

	return json;
}