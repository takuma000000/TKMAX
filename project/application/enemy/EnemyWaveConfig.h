#pragma once
#include <array>
#include <string>
#include <vector>
#include "MyMath.h"

class EnemyWaveConfig {
public:
	enum class Wave2Pattern {
		Triangle,
		Line,
		Column,
	};

	struct Wave1 {
		float spawnInterval = 1.5f;
		int   maxSimultaneous = 2;

		// SpawnPos(base): a(未使用), b=y, c=z
		float baseY = 5.0f;
		float baseZ = 100.0f;

		// RandX(range): b=min, c=max
		float randXMin = -20.0f;
		float randXMax = 20.0f;
	};

	struct Wave2SubWave {
		Wave2Pattern pattern = Wave2Pattern::Triangle;

		// Triangle: id, a=Triangle, b=段(=countPerSide), c=y, d=z, e=xCenter, f=xStep, g=zStep
		int   triCountPerSide = 1;
		float triY = 6.0f;
		float triZ = 80.0f;
		float triXCenter = 0.0f;
		float triXStep = 7.0f;
		float triZStep = 5.0f;

		// Line: b=count, c=y, d=z, e=xStart, f=xStep
		int   lineCount = 4;
		float lineY = 4.5f;
		float lineZ = 90.0f;
		float lineXStart = -12.0f;
		float lineXStep = 8.0f;

		// Column: b=count, c=x, d=zStart, e=zStep, f=yStart, g=yStep
		int   colCount = 3;
		float colX = 18.0f;
		float colZStart = 100.0f;
		float colZStep = 10.0f;
		float colYStart = 5.0f;
		float colYStep = 0.0f;
	};

	struct Wave3 {
		Vector3 midBossLeft = { -12.0f, 6.0f, 80.0f };
		Vector3 midBossRight = { 12.0f, 6.0f, 80.0f };

		// CoreRand(params): a=xRange, b=zMin, c=zMax, d=y
		float coreXRange = 18.0f;
		float coreZMin = 35.0f;
		float coreZMax = 75.0f;
		float coreY = 6.0f;
	};

public:
	bool Load(const char* path);

	const Wave1& GetWave1() const { return wave1_; }
	float GetWave2WaitDuration() const { return wave2WaitDuration_; }
	const Wave2SubWave& GetWave2SubWave(int id) const { return wave2SubWaves_[id]; }
	const Wave3& GetWave3() const { return wave3_; }

private:
	static bool StrEq(const std::string& a, const char* b);
	static float ToF(const std::string& s);
	static int   ToI(const std::string& s);

	Wave1 wave1_{};
	float wave2WaitDuration_ = 1.5f;
	std::array<Wave2SubWave, 3> wave2SubWaves_{};
	Wave3 wave3_{};
};