#pragma once
#include <array>
#include <string>
#include <vector>
#include "MyMath.h"
#include "Enemy.h"

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
		int defeatTarget_ = 5;

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

		float coreLifetime_ = 5.0f;
		int   coreHP_ = 5;
		float angryDuration_ = 8.0f;
	};

	/// <summary>
	/// 設定ファイルを読み込みます。
	/// </summary>
	/// <param name="path">読み込む設定ファイルのパス</param>
	/// <returns>読み込みに成功した場合 true、それ以外は false</returns>
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
	/// Wave2のサブウェーブ数を取得します。
	/// </summary>
	/// <returns></returns>
	int GetWave2SubWaveCount() const { return wave2SubWaveCount_; }
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
	/// 文字列を等価比較します。
	/// </summary>
	/// <param name="a">比較対象となる文字列</param>
	/// <param name="b">比較対象となる C 文字列</param>
	/// <returns>等しい場合 true、それ以外は false</returns>
	static bool StrEq(const std::string& a, const char* b);
	/// <summary>
	/// 文字列を float 値に変換します。
	/// </summary>
	/// <param name="s">変換元となる文字列</param>
	/// <returns>変換後の float 値</returns>
	static float ToF(const std::string& s);
	/// <summary>
	/// 文字列を int 値に変換します。
	/// </summary>
	/// <param name="s">変換元となる文字列</param>
	/// <returns>変換後の int 値</returns>
	static int   ToI(const std::string& s);

	Wave1 wave1_{};
	float wave2WaitDuration_ = 1.5f;
	std::array<Wave2SubWave, 3> wave2SubWaves_{};
	Wave3 wave3_{};
	// 固定値
	int wave2SubWaveCount_ = 3;

public:
	// Enemy Params ==========================================
	struct Wave1EnemyParams {
		std::string model_ = "jerryfish.obj";
		int hp_ = 1;
		float startY_ = 20.0f;
		float targetForwardZ_ = 3.0f;
		float apexY_ = 30.0f;
		float pounceTime_ = 1.6f;
		EnemyBehavior behavior_ = EnemyBehavior::PounceFromAbove;
	};

	struct Wave2EnemyParamsTriangle {
		std::string model_ = "jerryfish.obj";
		int hp_ = 3;
		Vector3 vel_ = { 0.0f, 0.0f, -0.30f };
		float sineAmp_ = 4.0f;
		float sineFreq_ = 1.4f;
		float phaseStep_ = 0.6f;
		EnemyBehavior behavior_ = EnemyBehavior::SineX;
	};

	struct Wave2EnemyParamsLine {
		std::string model_ = "jerryfish.obj";
		int hp_ = 2;
		Vector3 vel_ = { 0.0f, 0.0f, -0.32f };
		float stopZ_ = 52.0f;
		EnemyBehavior behavior_ = EnemyBehavior::StraightStop;
	};

	struct Wave2EnemyParamsColumn {
		std::string model_ = "jerryfish.obj";
		int hp_ = 1;
		Vector3 vel_ = { -0.20f, 0.0f, -0.75f };
		float stopZ_ = -50.0f;
		EnemyBehavior behavior_ = EnemyBehavior::StraightStop;
	};

	struct Wave3MidBossParams {
		std::string model_ = "jerryfish.obj";
		int hp_ = 12;
		Vector3 areaMin_ = { -18.0f, 4.0f, 40.0f };
		Vector3 areaMax_ = { 18.0f, 10.0f, 62.0f };
		float normalSpeed_ = 0.10f;
		float rageSpeed_ = 0.24f;
		float scale_ = 1.5f;
		EnemyBehavior behavior_ = EnemyBehavior::FreeRoam;
	};

	struct Wave3ExtraMidBossParams {
		std::string model_ = "jerryfish.obj";
		int hp_ = 12;
		Vector3 vel_ = { 0.0f, 0.0f, -0.2f };
		float stopZ_ = 40.0f;
		float scale_ = 1.5f;
		EnemyBehavior behavior_ = EnemyBehavior::StraightStop;
	};

	/// <summary>
	/// Wave1 用の敵パラメータを取得します。
	/// </summary>
	/// <returns>Wave1 敵パラメータ</returns>
	const Wave1EnemyParams& GetWave1EnemyParams() const { return wave1EnemyParams_; }
	/// <summary>
	/// Wave2（三角形配置）用の敵パラメータを取得します。
	/// </summary>
	/// <returns>Wave2 三角形配置用敵パラメータ</returns>
	const Wave2EnemyParamsTriangle& GetWave2TriEnemyParams() const { return wave2TriEnemyParams_; }
	/// <summary>
	/// Wave2（直線配置）用の敵パラメータを取得します。
	/// </summary>
	/// <returns>Wave2 直線配置用敵パラメータ</returns>
	const Wave2EnemyParamsLine& GetWave2LineEnemyParams() const { return wave2LineEnemyParams_; }
	/// <summary>
	/// Wave2（縦列配置）用の敵パラメータを取得します。
	/// </summary>
	/// <returns>Wave2 縦列配置用敵パラメータ</returns>
	const Wave2EnemyParamsColumn& GetWave2ColEnemyParams() const { return wave2ColEnemyParams_; }
	/// <summary>
	/// Wave3（ミッドボス）用のパラメータを取得します。
	/// </summary>
	/// <returns>Wave3 ミッドボス用パラメータ</returns>
	const Wave3MidBossParams& GetWave3MidBossParams() const { return wave3MidBossParams_; }
	/// <summary>
	/// Wave3（追加ミッドボス）用のパラメータを取得します。
	/// </summary>
	/// <returns>Wave3 追加ミッドボス用パラメータ</returns>
	const Wave3ExtraMidBossParams& GetWave3ExtraMidBossParams() const { return wave3ExtraMidBossParams_; }
private:
	// Enemy Params 内部データ
	Wave1EnemyParams wave1EnemyParams_{};
	Wave2EnemyParamsTriangle wave2TriEnemyParams_{};
	Wave2EnemyParamsLine wave2LineEnemyParams_{};
	Wave2EnemyParamsColumn wave2ColEnemyParams_{};
	Wave3MidBossParams wave3MidBossParams_{};
	Wave3ExtraMidBossParams wave3ExtraMidBossParams_{};
};