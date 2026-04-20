#include "PlayerShotConfig.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include "json.hpp"

using json = nlohmann::json;

bool PlayerShotConfig::HasExtension(const std::string& path, const char* ext) {
	if (!ext) {
		return false;
	}

	std::string lowerPath = path;
	std::string lowerExt = ext;

	std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (lowerPath.size() < lowerExt.size()) {
		return false;
	}

	return lowerPath.compare(lowerPath.size() - lowerExt.size(), lowerExt.size(), lowerExt) == 0;
}

bool PlayerShotConfig::Load(const char* path) {
	if (!path) {
		return false;
	}

	const std::string path_ = path;

	if (HasExtension(path_, ".json")) {
		return LoadJson(path);
	}

	return false;
}

bool PlayerShotConfig::LoadJson(const char* path) {
	if (!path) {
		return false;
	}

	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		return false;
	}

	json root;
	ifs >> root;

	if (root.contains("rb")) {
		auto& rb = root["rb"];

		if (rb.contains("bulletSpeed")) {
			rb_.bulletSpeed_ = rb["bulletSpeed"].get<float>();
		}
		if (rb.contains("ammoMax")) {
			rb_.ammoMax_ = rb["ammoMax"].get<int>();
		}
		if (rb.contains("refillWaitSec")) {
			rb_.refillWaitSec_ = rb["refillWaitSec"].get<float>();
		}
		if (rb.contains("refillSec")) {
			rb_.refillSec_ = rb["refillSec"].get<float>();
		}
		if (rb.contains("shotCooldownSec")) {
			rb_.shotCooldownSec_ = rb["shotCooldownSec"].get<float>();
		}
	}

	if (root.contains("lb")) {
		auto& lb = root["lb"];

		if (lb.contains("ammoMax")) {
			lb_.ammoMax_ = lb["ammoMax"].get<int>();
		}
		if (lb.contains("refillWaitSec")) {
			lb_.refillWaitSec_ = lb["refillWaitSec"].get<float>();
		}
		if (lb.contains("arcHeight")) {
			lb_.arcHeight_ = lb["arcHeight"].get<float>();
		}
		if (lb.contains("arcDuration")) {
			lb_.arcDuration_ = lb["arcDuration"].get<float>();
		}
		if (lb.contains("forwardOffsetZ")) {
			lb_.forwardOffsetZ_ = lb["forwardOffsetZ"].get<float>();
		}
	}

	return true;
}