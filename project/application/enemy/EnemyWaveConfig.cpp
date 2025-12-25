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
	std::vector<std::vector<std::string>> rows;
	if (!TKM::CsvReader::ReadFile(path, rows)) {
		return false;
	}

	for (const auto& c : rows) {
		// wave,type,id,a,b,c,d,e,f みたいな固定列
		if (c.size() < 3) { continue; }

		const std::string& wave = c[0];
		const std::string& type = c[1];
		const std::string& id = c[2];

		auto get = [&](size_t idx) -> std::string {
			if (idx < c.size()) { return c[idx]; }
			return "0";
			};

		// ---- Wave1 ----
		if (StrEq(wave, "Wave1")) {
			if (StrEq(type, "Settings")) {
				if (StrEq(id, "spawnInterval")) { wave1_.spawnInterval = ToF(get(3)); }
				if (StrEq(id, "maxSimultaneous")) { wave1_.maxSimultaneous = ToI(get(3)); }
			} else if (StrEq(type, "SpawnPos") && StrEq(id, "base")) {
				// Wave1,SpawnPos,base,a,b,c,...
				wave1_.baseY = ToF(get(4));
				wave1_.baseZ = ToF(get(5));
			} else if (StrEq(type, "RandX") && StrEq(id, "range")) {
				wave1_.randXMin = ToF(get(4));
				wave1_.randXMax = ToF(get(5));
			}
		}

		// ---- Wave2 ----
		if (StrEq(wave, "Wave2")) {
			if (StrEq(type, "Wait") && StrEq(id, "base")) {
				wave2WaitDuration_ = ToF(get(3));
			} else if (StrEq(type, "SubWave")) {
				const int subId = ToI(id);
				if (subId < 0 || subId >= 3) { continue; }

				Wave2SubWave& sw = wave2SubWaves_[subId];

				const std::string pat = get(3);
				if (pat == "Triangle") {
					sw.pattern = Wave2Pattern::Triangle;
					sw.triCountPerSide = ToI(get(4));
					sw.triY = ToF(get(5));
					sw.triZ = ToF(get(6));
					sw.triXCenter = ToF(get(7));
					sw.triXStep = ToF(get(8));
					sw.triZStep = ToF(get(9));
				} else if (pat == "Line") {
					sw.pattern = Wave2Pattern::Line;
					sw.lineCount = ToI(get(4));
					sw.lineY = ToF(get(5));
					sw.lineZ = ToF(get(6));
					sw.lineXStart = ToF(get(7));
					sw.lineXStep = ToF(get(8));
				} else if (pat == "Column") {
					sw.pattern = Wave2Pattern::Column;
					sw.colCount = ToI(get(4));
					sw.colX = ToF(get(5));
					sw.colZStart = ToF(get(6));
					sw.colZStep = ToF(get(7));
					sw.colYStart = ToF(get(8));
					sw.colYStep = ToF(get(9));
				}
			}
		}

		// ---- Wave3 ----
		if (StrEq(wave, "Wave3")) {
			if (StrEq(type, "MidBossPos")) {
				// Wave3,MidBossPos,left,x,y,z
				Vector3 p{ ToF(get(3)), ToF(get(4)), ToF(get(5)) };
				if (StrEq(id, "left")) { wave3_.midBossLeft = p; }
				if (StrEq(id, "right")) { wave3_.midBossRight = p; }
			} else if (StrEq(type, "CoreRand") && StrEq(id, "params")) {
				wave3_.coreXRange = ToF(get(3));
				wave3_.coreZMin = ToF(get(4));
				wave3_.coreZMax = ToF(get(5));
				wave3_.coreY = ToF(get(6));
			}
		}
	}

	return true;
}