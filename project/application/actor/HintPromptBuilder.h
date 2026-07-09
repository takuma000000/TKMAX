#pragma once
#include <string>
#include "HintLog.h"

class HintPromptBuilder {
public:
	static std::string BuildPrompt(const HintLog& hintLog);
};