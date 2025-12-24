#pragma once
#include <vector>
#include <string>

struct SpawnEvent {
	int wave = 0;
	float time = 0.0f;
	std::string pattern;

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	int count = 1;
	float spacing = 0.0f;
	bool spawnFx = false;

	// 余白（未使用なら空でOK）
	float paramA = 0.0f;
	float paramB = 0.0f;
	float paramC = 0.0f;
};

class CsvSpawnLoader {
public:
	static bool Load(const std::string& filepath, std::vector<SpawnEvent>& out);
};