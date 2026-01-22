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
		float spawnInterval_ = 1.5f;
		int   maxSimultaneous_ = 2;

		// SpawnPos(base): a(未使用), b=y, c=z
		float baseY_ = 5.0f;
		float baseZ_ = 100.0f;

		// RandX(range): b=min, c=max
		float randXMin_ = -20.0f;
		float randXMax_ = 20.0f;
	};

	struct Wave2SubWave {
		Wave2Pattern pattern_ = Wave2Pattern::Triangle;

		// Triangle: id, a=Triangle, b=段(=countPerSide), c=y, d=z, e=xCenter, f=xStep, g=zStep
		int   triCountPerSide_ = 1;
		float triY_ = 6.0f;
		float triZ_ = 80.0f;
		float triXCenter_ = 0.0f;
		float triXStep_ = 7.0f;
		float triZStep_ = 5.0f;

		// Line: b=count, c=y, d=z, e=xStart, f=xStep
		int   lineCount_ = 4;
		float lineY_ = 4.5f;
		float lineZ_ = 90.0f;
		float lineXStart_ = -12.0f;
		float lineXStep_ = 8.0f;

		// Column: b=count, c=x, d=zStart, e=zStep, f=yStart, g=yStep
		int   colCount_ = 3;
		float colX_ = 18.0f;
		float colZStart_ = 100.0f;
		float colZStep_ = 10.0f;
		float colYStart_ = 5.0f;
		float colYStep_ = 0.0f;
	};

	struct Wave3 {
		Vector3 midBossLeft_ = { -12.0f, 6.0f, 80.0f };
		Vector3 midBossRight_ = { 12.0f, 6.0f, 80.0f };

		// CoreRand(params): a=xRange, b=zMin, c=zMax, d=y
		float coreXRange_ = 18.0f;
		float coreZMin_ = 35.0f;
		float coreZMax_ = 75.0f;
		float coreY_ = 6.0f;
	};

	/// <summary>
	/// 設定ファイルを読み込みます。
	/// </summary>
	/// <param name="path"></param>
	/// <returns></returns>
	bool Load(const char* path);

	// Getter==========================================
	/// <summary>
	/// Wave1設定を取得します。
	/// </summary>
	/// <returns></returns>
	const Wave1& GetWave1() const { return wave1_; }
	/// <summary>
	/// Wave2の待機時間を取得します。
	/// </summary>
	/// <returns></returns>
	float GetWave2WaitDuration() const { return wave2WaitDuration_; }
	/// <summary>
	/// Wave2のサブウェーブ設定を取得します。
	/// </summary>
	/// <param name="id"></param>
	/// <returns></returns>
	const Wave2SubWave& GetWave2SubWave(int id) const { return wave2SubWaves_[id]; }
	/// <summary>
	/// Wave3設定を取得します。
	/// </summary>
	/// <returns></returns>
	const Wave3& GetWave3() const { return wave3_; }
	// ================================================

private:
	/// <summary>
	/// 文字列比較（等価）
	/// </summary>
	/// <param name="a"></param>
	/// <param name="b"></param>
	/// <returns></returns>
	static bool StrEq(const std::string& a, const char* b);
	/// <summary>
	/// 文字列を float に変換します。
	/// </summary>
	/// <param name="s"></param>
	/// <returns></returns>
	static float ToF(const std::string& s);
	/// <summary>
	/// 文字列を int に変換します。
	/// </summary>
	/// <param name="s"></param>
	/// <returns></returns>
	static int   ToI(const std::string& s);

	Wave1 wave1_{};
	float wave2WaitDuration_ = 1.5f;
	std::array<Wave2SubWave, 3> wave2SubWaves_{};
	Wave3 wave3_{};
};