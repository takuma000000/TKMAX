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
	if (!path) {
		return false;
	}

	std::ifstream ifs(path);
	if (!ifs.is_open()) {
		return false;
	}

	json root;
	ifs >> root;

	// -------------------------
	// Wave1
	// -------------------------
	if (root.contains("wave1")) {
		auto& w = root["wave1"];

		if (w.contains("settings")) {
			auto& s = w["settings"];

			if (s.contains("spawnInterval")) {
				wave1_.spawnInterval_ = s["spawnInterval"].get<float>();
			}
			if (s.contains("maxSimultaneous")) {
				wave1_.maxSimultaneous_ = s["maxSimultaneous"].get<int>();
			}
			if (s.contains("defeatTarget")) {
				wave1_.defeatTarget_ = s["defeatTarget"].get<int>();
			}
		}

		if (w.contains("spawn")) {
			auto& s = w["spawn"];

			if (s.contains("baseY")) {
				wave1_.baseY_ = s["baseY"].get<float>();
			}
			if (s.contains("baseZ")) {
				wave1_.baseZ_ = s["baseZ"].get<float>();
			}
			if (s.contains("randXMin")) {
				wave1_.randXMin_ = s["randXMin"].get<float>();
			}
			if (s.contains("randXMax")) {
				wave1_.randXMax_ = s["randXMax"].get<float>();
			}
		}

		if (w.contains("enemyParams")) {
			auto& e = w["enemyParams"];

			if (e.contains("model")) {
				wave1EnemyParams_.model_ = e["model"].get<std::string>();
			}
			if (e.contains("hp")) {
				wave1EnemyParams_.hp_ = e["hp"].get<int>();
			}
			if (e.contains("startY")) {
				wave1EnemyParams_.startY_ = e["startY"].get<float>();
			}
			if (e.contains("targetForwardZ")) {
				wave1EnemyParams_.targetForwardZ_ = e["targetForwardZ"].get<float>();
			}
			if (e.contains("apexY")) {
				wave1EnemyParams_.apexY_ = e["apexY"].get<float>();
			}
			if (e.contains("pounceTime")) {
				wave1EnemyParams_.pounceTime_ = e["pounceTime"].get<float>();
			}
		}
	}

	// -------------------------
	// Wave2
	// -------------------------
	if (root.contains("wave2")) {
		auto& w = root["wave2"];

		if (w.contains("waitDuration")) {
			wave2WaitDuration_ = w["waitDuration"].get<float>();
		}
		if (w.contains("subWaveCount")) {
			wave2SubWaveCount_ = w["subWaveCount"].get<int>();
		}

		if (w.contains("subWaves") && w["subWaves"].is_array()) {
			const auto& arr = w["subWaves"];
			const size_t count = (std::min)(arr.size(), wave2SubWaves_.size());

			for (size_t i = 0; i < count; ++i) {
				const auto& jsw = arr[i];
				Wave2SubWave& sw = wave2SubWaves_[i];

				if (jsw.contains("pattern")) {
					const std::string pattern = jsw["pattern"].get<std::string>();

					if (pattern == "Triangle") {
						sw.pattern_ = Wave2Pattern::Triangle;
						if (jsw.contains("triCountPerSide")) { sw.triCountPerSide_ = jsw["triCountPerSide"].get<int>(); }
						if (jsw.contains("triY")) { sw.triY_ = jsw["triY"].get<float>(); }
						if (jsw.contains("triZ")) { sw.triZ_ = jsw["triZ"].get<float>(); }
						if (jsw.contains("triXCenter")) { sw.triXCenter_ = jsw["triXCenter"].get<float>(); }
						if (jsw.contains("triXStep")) { sw.triXStep_ = jsw["triXStep"].get<float>(); }
						if (jsw.contains("triZStep")) { sw.triZStep_ = jsw["triZStep"].get<float>(); }
					} else if (pattern == "Line") {
						sw.pattern_ = Wave2Pattern::Line;
						if (jsw.contains("lineCount")) { sw.lineCount_ = jsw["lineCount"].get<int>(); }
						if (jsw.contains("lineY")) { sw.lineY_ = jsw["lineY"].get<float>(); }
						if (jsw.contains("lineZ")) { sw.lineZ_ = jsw["lineZ"].get<float>(); }
						if (jsw.contains("lineXStart")) { sw.lineXStart_ = jsw["lineXStart"].get<float>(); }
						if (jsw.contains("lineXStep")) { sw.lineXStep_ = jsw["lineXStep"].get<float>(); }
					} else if (pattern == "Column") {
						sw.pattern_ = Wave2Pattern::Column;
						if (jsw.contains("colCount")) { sw.colCount_ = jsw["colCount"].get<int>(); }
						if (jsw.contains("colX")) { sw.colX_ = jsw["colX"].get<float>(); }
						if (jsw.contains("colZStart")) { sw.colZStart_ = jsw["colZStart"].get<float>(); }
						if (jsw.contains("colZStep")) { sw.colZStep_ = jsw["colZStep"].get<float>(); }
						if (jsw.contains("colYStart")) { sw.colYStart_ = jsw["colYStart"].get<float>(); }
						if (jsw.contains("colYStep")) { sw.colYStep_ = jsw["colYStep"].get<float>(); }
					}
				}
			}
		}

		if (w.contains("enemyParams")) {
			auto& ep = w["enemyParams"];

			if (ep.contains("triangle")) {
				auto& e = ep["triangle"];
				if (e.contains("model")) { wave2TriEnemyParams_.model_ = e["model"].get<std::string>(); }
				if (e.contains("hp")) { wave2TriEnemyParams_.hp_ = e["hp"].get<int>(); }

				if (e.contains("vel")) {
					auto& v = e["vel"];
					if (v.contains("x")) { wave2TriEnemyParams_.vel_.x = v["x"].get<float>(); }
					if (v.contains("y")) { wave2TriEnemyParams_.vel_.y = v["y"].get<float>(); }
					if (v.contains("z")) { wave2TriEnemyParams_.vel_.z = v["z"].get<float>(); }
				}

				if (e.contains("sineAmp")) { wave2TriEnemyParams_.sineAmp_ = e["sineAmp"].get<float>(); }
				if (e.contains("sineFreq")) { wave2TriEnemyParams_.sineFreq_ = e["sineFreq"].get<float>(); }
				if (e.contains("phaseStep")) { wave2TriEnemyParams_.phaseStep_ = e["phaseStep"].get<float>(); }
				if (e.contains("behavior")) {
					wave2TriEnemyParams_.behavior_ =
						ParseBehavior(e["behavior"].get<std::string>(), wave2TriEnemyParams_.behavior_);
				}
			}

			if (ep.contains("line")) {
				auto& e = ep["line"];
				if (e.contains("model")) { wave2LineEnemyParams_.model_ = e["model"].get<std::string>(); }
				if (e.contains("hp")) { wave2LineEnemyParams_.hp_ = e["hp"].get<int>(); }

				if (e.contains("vel")) {
					auto& v = e["vel"];
					if (v.contains("x")) { wave2LineEnemyParams_.vel_.x = v["x"].get<float>(); }
					if (v.contains("y")) { wave2LineEnemyParams_.vel_.y = v["y"].get<float>(); }
					if (v.contains("z")) { wave2LineEnemyParams_.vel_.z = v["z"].get<float>(); }
				}

				if (e.contains("stopZ")) { wave2LineEnemyParams_.stopZ_ = e["stopZ"].get<float>(); }
				if (e.contains("behavior")) {
					wave2LineEnemyParams_.behavior_ =
						ParseBehavior(e["behavior"].get<std::string>(), wave2LineEnemyParams_.behavior_);
				}
			}

			if (ep.contains("column")) {
				auto& e = ep["column"];
				if (e.contains("model")) { wave2ColEnemyParams_.model_ = e["model"].get<std::string>(); }
				if (e.contains("hp")) { wave2ColEnemyParams_.hp_ = e["hp"].get<int>(); }

				if (e.contains("vel")) {
					auto& v = e["vel"];
					if (v.contains("x")) { wave2ColEnemyParams_.vel_.x = v["x"].get<float>(); }
					if (v.contains("y")) { wave2ColEnemyParams_.vel_.y = v["y"].get<float>(); }
					if (v.contains("z")) { wave2ColEnemyParams_.vel_.z = v["z"].get<float>(); }
				}

				if (e.contains("stopZ")) { wave2ColEnemyParams_.stopZ_ = e["stopZ"].get<float>(); }
				if (e.contains("behavior")) {
					wave2ColEnemyParams_.behavior_ =
						ParseBehavior(e["behavior"].get<std::string>(), wave2ColEnemyParams_.behavior_);
				}
			}
		}
	}

	// -------------------------
	// Wave3
	// -------------------------
	if (root.contains("wave3")) {
		auto& w = root["wave3"];

		if (w.contains("midBossLeft")) {
			auto& p = w["midBossLeft"];
			wave3_.midBossLeft_.x = p["x"].get<float>();
			wave3_.midBossLeft_.y = p["y"].get<float>();
			wave3_.midBossLeft_.z = p["z"].get<float>();
		}

		if (w.contains("midBossRight")) {
			auto& p = w["midBossRight"];
			wave3_.midBossRight_.x = p["x"].get<float>();
			wave3_.midBossRight_.y = p["y"].get<float>();
			wave3_.midBossRight_.z = p["z"].get<float>();
		}

		if (w.contains("core")) {
			auto& c = w["core"];

			if (c.contains("xRange")) {
				wave3_.coreXRange_ = c["xRange"].get<float>();
			}
			if (c.contains("zMin")) {
				wave3_.coreZMin_ = c["zMin"].get<float>();
			}
			if (c.contains("zMax")) {
				wave3_.coreZMax_ = c["zMax"].get<float>();
			}
			if (c.contains("y")) {
				wave3_.coreY_ = c["y"].get<float>();
			}
			if (c.contains("lifetime")) {
				wave3_.coreLifetime_ = c["lifetime"].get<float>();
			}
			if (c.contains("hp")) {
				wave3_.coreHP_ = c["hp"].get<int>();
			}
		}

		if (w.contains("angryDuration")) {
			wave3_.angryDuration_ = w["angryDuration"].get<float>();
		}

		if (w.contains("enemyParams")) {
			auto& ep = w["enemyParams"];

			if (ep.contains("midBoss")) {
				auto& e = ep["midBoss"];

				if (e.contains("model")) {
					wave3MidBossParams_.model_ = e["model"].get<std::string>();
				}
				if (e.contains("hp")) {
					wave3MidBossParams_.hp_ = e["hp"].get<int>();
				}
				if (e.contains("scale")) {
					wave3MidBossParams_.scale_ = e["scale"].get<float>();
				}
				if (e.contains("behavior")) {
					wave3MidBossParams_.behavior_ =
						ParseBehavior(e["behavior"].get<std::string>(), wave3MidBossParams_.behavior_);
				}
			}
		}
	}

	return true;
}