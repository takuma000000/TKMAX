#pragma once
#include <string>

class HintClient {
public:
	bool Send(const std::string& json);

	const std::string& GetLatestHint() const { return latestHint_; }

private:
	std::string latestHint_;
};