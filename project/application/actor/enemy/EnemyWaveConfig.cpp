#include "EnemyWaveConfig.h"
#include <cstdlib>
#include <CsvReader.h>
#include <algorithm>
#include <cctype>
#include "json.hpp"
using json = nlohmann::json;

bool EnemyWaveConfig::StrEq(const std::string& a, const char* b) {
	return a == b;
}

float EnemyWaveConfig::ToF(const std::string& s) {
	return std::stof(s);
}

int EnemyWaveConfig::ToI(const std::string& s) {
	return std::stoi(s);
}

bool EnemyWaveConfig::HasExtension(const std::string& path, const char* ext) {
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

static EnemyBehavior ParseBehavior(const std::string& s, EnemyBehavior fallback) {
	if (s.empty() || s == "0") { return fallback; }

	std::string t = s;
	std::transform(t.begin(), t.end(), t.begin(),
		[](unsigned char c) { return (char)std::tolower(c); });

	// 敵の行動パターンを表す文字列を EnemyBehavior 列挙型に変換
	if (t == "pouncefromabove") { return EnemyBehavior::PounceFromAbove; }
	if (t == "sinex") { return EnemyBehavior::SineX; }
	if (t == "straightstop") { return EnemyBehavior::StraightStop; }
	if (t == "freeroam") { return EnemyBehavior::FreeRoam; }

	return fallback;
}

bool EnemyWaveConfig::Load(const char* path) {
	if (!path) {
		return false;
	}

	const std::string path_ = path;

	if (HasExtension(path_, ".csv")) {
		return LoadCsv(path);
	}

	if (HasExtension(path_, ".json")) {
		return LoadJson(path);
	}

	return false;
}

bool EnemyWaveConfig::LoadCsv(const char* path) {
	std::vector<std::vector<std::string>> rows_;
	// CSVファイルを読み込む
	if (!TKM::CsvReader::ReadFile(path, rows_)) {
		return false;
	}
	// 読み込んだ行を1行ずつ処理
	for (const auto& c : rows_) {
		// wave,type,id,a,b,c,d,e,f みたいな固定列
		if (c.size() < 3) { continue; }

		const std::string& wave_ = c[0];
		const std::string& type_ = c[1];
		const std::string& id_ = c[2];

		auto get_ = [&](size_t idx) -> std::string {
			if (idx < c.size()) { return c[idx]; }
			return "0";
			};

		// ---- Wave1 ----
		if (StrEq(wave_, "Wave1")) {
			// Wave1の設定行を処理
			if (StrEq(type_, "Settings")) {
				// Wave1,Settings,spawnInterval,a,b,c,...
				if (StrEq(id_, "spawnInterval")) { wave1_.spawnInterval_ = ToF(get_(3)); }
				// Wave1,Settings,maxSimultaneous,a,b,c,...
				if (StrEq(id_, "maxSimultaneous")) { wave1_.maxSimultaneous_ = ToI(get_(3)); }
				// Wave1,Settings,defeatTarget,a,b,c,...
				if (StrEq(id_, "defeatTarget")) { wave1_.defeatTarget_ = ToI(get_(3)); }
			} else if (StrEq(type_, "SpawnPos") && StrEq(id_, "base")) {
				// Wave1,SpawnPos,base,a,b,c,...
				wave1_.baseY_ = ToF(get_(4));
				wave1_.baseZ_ = ToF(get_(5));
			} else if (StrEq(type_, "RandX") && StrEq(id_, "range")) {
				wave1_.randXMin_ = ToF(get_(4));
				wave1_.randXMax_ = ToF(get_(5));
			}
			// 敵のパラメータ行（type=EnemyParams, id=default）を処理
			if (StrEq(type_, "EnemyParams") && StrEq(id_, "default")) {
				wave1EnemyParams_.model_ = get_(3);
				wave1EnemyParams_.hp_ = ToI(get_(4));
				wave1EnemyParams_.startY_ = ToF(get_(5));
				wave1EnemyParams_.targetForwardZ_ = ToF(get_(6));
				wave1EnemyParams_.apexY_ = ToF(get_(7));
				wave1EnemyParams_.pounceTime_ = ToF(get_(8));
				wave1EnemyParams_.behavior_ = ParseBehavior(get_(11), wave1EnemyParams_.behavior_);
			}
		}

		// ---- Wave2 ----
		if (StrEq(wave_, "Wave2")) {
			if (StrEq(type_, "Wait") && StrEq(id_, "base")) {
				wave2WaitDuration_ = ToF(get_(3));
			} else if (StrEq(type_, "Settings") && StrEq(id_, "subWaveCount")) {
				wave2SubWaveCount_ = ToI(get_(3));
			} else if (StrEq(type_, "SubWave")) {
				const int subId_ = ToI(id_);
				if (subId_ < 0 || subId_ >= 3) { continue; }

				Wave2SubWave& sw_ = wave2SubWaves_[subId_];

				const std::string pat = get_(3);
				if (pat == "Triangle") {
					sw_.pattern_ = Wave2Pattern::Triangle;
					sw_.triCountPerSide_ = ToI(get_(4));
					sw_.triY_ = ToF(get_(5));
					sw_.triZ_ = ToF(get_(6));
					sw_.triXCenter_ = ToF(get_(7));
					sw_.triXStep_ = ToF(get_(8));
					sw_.triZStep_ = ToF(get_(9));
				} else if (pat == "Line") {
					sw_.pattern_ = Wave2Pattern::Line;
					sw_.lineCount_ = ToI(get_(4));
					sw_.lineY_ = ToF(get_(5));
					sw_.lineZ_ = ToF(get_(6));
					sw_.lineXStart_ = ToF(get_(7));
					sw_.lineXStep_ = ToF(get_(8));
				} else if (pat == "Column") {
					sw_.pattern_ = Wave2Pattern::Column;
					sw_.colCount_ = ToI(get_(4));
					sw_.colX_ = ToF(get_(5));
					sw_.colZStart_ = ToF(get_(6));
					sw_.colZStep_ = ToF(get_(7));
					sw_.colYStart_ = ToF(get_(8));
					sw_.colYStep_ = ToF(get_(9));
				}
			}

			if (StrEq(type_, "EnemyParams")) {
				if (StrEq(id_, "Triangle")) {
					wave2TriEnemyParams_.model_ = get_(3);
					wave2TriEnemyParams_.hp_ = ToI(get_(4));
					wave2TriEnemyParams_.vel_ = { ToF(get_(5)), ToF(get_(6)), ToF(get_(7)) };
					wave2TriEnemyParams_.sineAmp_ = ToF(get_(8));
					wave2TriEnemyParams_.sineFreq_ = ToF(get_(9));
					wave2TriEnemyParams_.phaseStep_ = ToF(get_(10)); // 11列目
					wave2TriEnemyParams_.behavior_ = ParseBehavior(get_(11), wave2TriEnemyParams_.behavior_);
				} else if (StrEq(id_, "Line")) {
					wave2LineEnemyParams_.model_ = get_(3);
					wave2LineEnemyParams_.hp_ = ToI(get_(4));
					wave2LineEnemyParams_.vel_ = { ToF(get_(5)), ToF(get_(6)), ToF(get_(7)) };
					wave2LineEnemyParams_.stopZ_ = ToF(get_(8));
					wave2LineEnemyParams_.behavior_ = ParseBehavior(get_(11), wave2LineEnemyParams_.behavior_);
				} else if (StrEq(id_, "Column")) {
					wave2ColEnemyParams_.model_ = get_(3);
					wave2ColEnemyParams_.hp_ = ToI(get_(4));
					wave2ColEnemyParams_.vel_ = { ToF(get_(5)), ToF(get_(6)), ToF(get_(7)) };
					wave2ColEnemyParams_.stopZ_ = ToF(get_(8));
					wave2ColEnemyParams_.behavior_ = ParseBehavior(get_(11), wave2ColEnemyParams_.behavior_);
				}
			}
		}

		// ---- Wave3 ----
		if (StrEq(wave_, "Wave3")) {
			if (StrEq(type_, "MidBossPos")) {
				// Wave3,MidBossPos,left,x,y,z
				Vector3 p{ ToF(get_(3)), ToF(get_(4)), ToF(get_(5)) };
				if (StrEq(id_, "left")) { wave3_.midBossLeft_ = p; }
				if (StrEq(id_, "right")) { wave3_.midBossRight_ = p; }
			} if (StrEq(type_, "Settings")) {
				if (StrEq(id_, "coreLifetime")) { wave3_.coreLifetime_ = ToF(get_(3)); }
				if (StrEq(id_, "coreHP")) { wave3_.coreHP_ = ToI(get_(3)); }
				if (StrEq(id_, "angryDuration")) { wave3_.angryDuration_ = ToF(get_(3)); }
			} else if (StrEq(type_, "CoreRand") && StrEq(id_, "params")) {
				wave3_.coreXRange_ = ToF(get_(3));
				wave3_.coreZMin_ = ToF(get_(4));
				wave3_.coreZMax_ = ToF(get_(5));
				wave3_.coreY_ = ToF(get_(6));
			}

			if (StrEq(type_, "EnemyParams")) {
				if (StrEq(id_, "MidBoss")) {
					wave3MidBossParams_.model_ = get_(3);
					wave3MidBossParams_.hp_ = ToI(get_(4));
					wave3MidBossParams_.areaMin_ = { ToF(get_(5)), ToF(get_(6)), ToF(get_(7)) };
					wave3MidBossParams_.areaMax_ = { ToF(get_(8)), ToF(get_(9)), ToF(get_(10)) };
					wave3MidBossParams_.normalSpeed_ = ToF(get_(11));
					wave3MidBossParams_.rageSpeed_ = ToF(get_(12));
					wave3MidBossParams_.scale_ = ToF(get_(13));
					wave3MidBossParams_.behavior_ = ParseBehavior(get_(14), wave3MidBossParams_.behavior_);
				} else if (StrEq(id_, "ExtraMidBoss")) {
					wave3ExtraMidBossParams_.model_ = get_(3);
					wave3ExtraMidBossParams_.hp_ = ToI(get_(4));
					wave3ExtraMidBossParams_.vel_ = { ToF(get_(5)), ToF(get_(6)), ToF(get_(7)) };
					wave3ExtraMidBossParams_.stopZ_ = ToF(get_(8));
					wave3ExtraMidBossParams_.scale_ = ToF(get_(11));
					wave3ExtraMidBossParams_.behavior_ = ParseBehavior(get_(12), wave3ExtraMidBossParams_.behavior_);
				}
			}
		}
	}
	return true;
}

bool EnemyWaveConfig::LoadJson(const char* path) {
	(void)path;
	return false;
}