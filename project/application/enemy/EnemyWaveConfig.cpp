#include "EnemyWaveConfig.h"
#include <cstdlib>
#include <CsvReader.h>

bool EnemyWaveConfig::StrEq(const std::string& a, const char* b) {
	return a == b;
}

float EnemyWaveConfig::ToF(const std::string& s) {
	return std::stof(s);
}

int EnemyWaveConfig::ToI(const std::string& s) {
	return std::stoi(s);
}

bool EnemyWaveConfig::Load(const char* path) {
	std::vector<std::vector<std::string>> rows_;
	if (!TKM::CsvReader::ReadFile(path, rows_)) {
		return false;
	}

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
			if (StrEq(type_, "Settings")) {
				if (StrEq(id_, "spawnInterval")) { wave1_.spawnInterval_ = ToF(get_(3)); }
				if (StrEq(id_, "maxSimultaneous")) { wave1_.maxSimultaneous_ = ToI(get_(3)); }
			} else if (StrEq(type_, "SpawnPos") && StrEq(id_, "base")) {
				// Wave1,SpawnPos,base,a,b,c,...
				wave1_.baseY_ = ToF(get_(4));
				wave1_.baseZ_ = ToF(get_(5));
			} else if (StrEq(type_, "RandX") && StrEq(id_, "range")) {
				wave1_.randXMin_ = ToF(get_(4));
				wave1_.randXMax_ = ToF(get_(5));
			}
		}

		// ---- Wave2 ----
		if (StrEq(wave_, "Wave2")) {
			if (StrEq(type_, "Wait") && StrEq(id_, "base")) {
				wave2WaitDuration_ = ToF(get_(3));
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
		}

		// ---- Wave3 ----
		if (StrEq(wave_, "Wave3")) {
			if (StrEq(type_, "MidBossPos")) {
				// Wave3,MidBossPos,left,x,y,z
				Vector3 p{ ToF(get_(3)), ToF(get_(4)), ToF(get_(5)) };
				if (StrEq(id_, "left")) { wave3_.midBossLeft_ = p; }
				if (StrEq(id_, "right")) { wave3_.midBossRight_ = p; }
			} else if (StrEq(type_, "CoreRand") && StrEq(id_, "params")) {
				wave3_.coreXRange_ = ToF(get_(3));
				wave3_.coreZMin_ = ToF(get_(4));
				wave3_.coreZMax_ = ToF(get_(5));
				wave3_.coreY_ = ToF(get_(6));
			}
		}
	}
	return true;
}