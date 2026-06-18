#pragma once
#include <string>
#include <vector>

struct SelectLog {
	int roadIndex = -1;
	bool isCorrect = false;
};

class HintLog {
public:
	void Reset();

	void AddSelectLog(int roadIndex, bool isCorrect);

	int GetMissCount() const { return missCount_; }
	int GetCorrectCount() const { return correctCount_; }
	int GetTotalSelectCount() const { return totalSelectCount_; }

	const std::vector<SelectLog>& GetLogs() const { return selectLogs_; }

	std::string MakeJson() const;

private:
	std::string stageId_ = "five_road_001";
	int correctIndex_ = 2;

	std::vector<SelectLog> selectLogs_;

	int missCount_ = 0;
	int correctCount_ = 0;
	int totalSelectCount_ = 0;
};