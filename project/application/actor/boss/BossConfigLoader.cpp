#include "BossConfigLoader.h"
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

//=============================================================
// ボス設定読み込み
//=============================================================
bool BossConfigLoader::Load(const char* path, BossConfig& outConfig) {

	//=========================================================
	// 引数チェック
	//=========================================================
	if (!path) {
		return false;
	}

	//=========================================================
	// ファイルオープン
	//=========================================================
	std::ifstream file(path);
	if (!file.is_open()) {
		return false;
	}

	//=========================================================
	// JSON読み込み
	//=========================================================
	json j;
	file >> j;

	//=========================================================
	// BossEnemy
	//=========================================================
	if (j.contains("bossEnemy")) {
		auto& je = j["bossEnemy"];

		outConfig.bossEnemy_.model_ =
			je.value("model", outConfig.bossEnemy_.model_);

		outConfig.bossEnemy_.tentacleModel_ =
			je.value("tentacleModel", outConfig.bossEnemy_.tentacleModel_);

		outConfig.bossEnemy_.hp_ =
			je.value("hp", outConfig.bossEnemy_.hp_);

		if (je.contains("scale")) {
			outConfig.bossEnemy_.scale_.x =
				je["scale"].value("x", outConfig.bossEnemy_.scale_.x);
			outConfig.bossEnemy_.scale_.y =
				je["scale"].value("y", outConfig.bossEnemy_.scale_.y);
			outConfig.bossEnemy_.scale_.z =
				je["scale"].value("z", outConfig.bossEnemy_.scale_.z);
		}

		if (je.contains("colliderScale")) {
			outConfig.bossEnemy_.colliderScale_.x =
				je["colliderScale"].value("x", outConfig.bossEnemy_.colliderScale_.x);
			outConfig.bossEnemy_.colliderScale_.y =
				je["colliderScale"].value("y", outConfig.bossEnemy_.colliderScale_.y);
			outConfig.bossEnemy_.colliderScale_.z =
				je["colliderScale"].value("z", outConfig.bossEnemy_.colliderScale_.z);
		}

		outConfig.bossEnemy_.normalScale_ =
			je.value("normalScale", outConfig.bossEnemy_.normalScale_);

		outConfig.bossEnemy_.lockBlinkSpeed_ =
			je.value("lockBlinkSpeed", outConfig.bossEnemy_.lockBlinkSpeed_);

		outConfig.bossEnemy_.lockBlinkAmount_ =
			je.value("lockBlinkAmount", outConfig.bossEnemy_.lockBlinkAmount_);
	}

	//=========================================================
	// BossBattle
	//=========================================================
	if (j.contains("bossBattle")) {
		auto& jb = j["bossBattle"];

		if (jb.contains("spawnPos")) {
			outConfig.bossBattle_.spawnPos_.x =
				jb["spawnPos"].value("x", outConfig.bossBattle_.spawnPos_.x);
			outConfig.bossBattle_.spawnPos_.y =
				jb["spawnPos"].value("y", outConfig.bossBattle_.spawnPos_.y);
			outConfig.bossBattle_.spawnPos_.z =
				jb["spawnPos"].value("z", outConfig.bossBattle_.spawnPos_.z);
		}

		if (jb.contains("arenaMin")) {
			outConfig.bossBattle_.arenaMin_.x =
				jb["arenaMin"].value("x", outConfig.bossBattle_.arenaMin_.x);
			outConfig.bossBattle_.arenaMin_.y =
				jb["arenaMin"].value("y", outConfig.bossBattle_.arenaMin_.y);
			outConfig.bossBattle_.arenaMin_.z =
				jb["arenaMin"].value("z", outConfig.bossBattle_.arenaMin_.z);
		}

		if (jb.contains("arenaMax")) {
			outConfig.bossBattle_.arenaMax_.x =
				jb["arenaMax"].value("x", outConfig.bossBattle_.arenaMax_.x);
			outConfig.bossBattle_.arenaMax_.y =
				jb["arenaMax"].value("y", outConfig.bossBattle_.arenaMax_.y);
			outConfig.bossBattle_.arenaMax_.z =
				jb["arenaMax"].value("z", outConfig.bossBattle_.arenaMax_.z);
		}

		if (jb.contains("killRipple")) {
			auto& kr = jb["killRipple"];

			outConfig.bossBattle_.killRipple_.duration_ =
				kr.value("duration", outConfig.bossBattle_.killRipple_.duration_);

			outConfig.bossBattle_.killRipple_.radiusMax_ =
				kr.value("radiusMax", outConfig.bossBattle_.killRipple_.radiusMax_);

			outConfig.bossBattle_.killRipple_.amplitude_ =
				kr.value("amplitude", outConfig.bossBattle_.killRipple_.amplitude_);

			outConfig.bossBattle_.killRipple_.frequency_ =
				kr.value("frequency", outConfig.bossBattle_.killRipple_.frequency_);

			outConfig.bossBattle_.killRipple_.width_ =
				kr.value("width", outConfig.bossBattle_.killRipple_.width_);
		}

		outConfig.bossBattle_.killSlowScale_ =
			jb.value("killSlowScale", outConfig.bossBattle_.killSlowScale_);

		outConfig.bossBattle_.killSlowDuration_ =
			jb.value("killSlowDuration", outConfig.bossBattle_.killSlowDuration_);
	}

	//=========================================================
	// BossController
	//=========================================================
	if (j.contains("bossController")) {
		auto& jc = j["bossController"];

		outConfig.bossController_.predictLeadTime_ =
			jc.value("predictLeadTime", outConfig.bossController_.predictLeadTime_);

		//=========================================================
		// Orbit
		//=========================================================
		if (jc.contains("orbit")) {
			auto& jo = jc["orbit"];

			outConfig.bossController_.orbit_.z_ =
				jo.value("z", outConfig.bossController_.orbit_.z_);

			outConfig.bossController_.orbit_.y_ =
				jo.value("y", outConfig.bossController_.orbit_.y_);

			outConfig.bossController_.orbit_.radiusX_ =
				jo.value("radiusX", outConfig.bossController_.orbit_.radiusX_);

			outConfig.bossController_.orbit_.radiusY_ =
				jo.value("radiusY", outConfig.bossController_.orbit_.radiusY_);

			outConfig.bossController_.orbit_.angularSpeed_ =
				jo.value("angularSpeed", outConfig.bossController_.orbit_.angularSpeed_);

			outConfig.bossController_.orbit_.playerInfluence_ =
				jo.value("playerInfluence", outConfig.bossController_.orbit_.playerInfluence_);

			outConfig.bossController_.orbit_.follow_ =
				jo.value("follow", outConfig.bossController_.orbit_.follow_);

			outConfig.bossController_.orbit_.duration_ =
				jo.value("duration", outConfig.bossController_.orbit_.duration_);
		}

		//=========================================================
		// Recover
		//=========================================================
		if (jc.contains("recover")) {
			auto& jr = jc["recover"];

			outConfig.bossController_.recover_.follow_ =
				jr.value("follow", outConfig.bossController_.recover_.follow_);

			outConfig.bossController_.recover_.duration_ =
				jr.value("duration", outConfig.bossController_.recover_.duration_);
		}

		//=========================================================
		// Rage
		//=========================================================
		if (jc.contains("rage")) {
			auto& jrage = jc["rage"];

			outConfig.bossController_.rage_.gainPerHp_ =
				jrage.value("gainPerHp", outConfig.bossController_.rage_.gainPerHp_);

			outConfig.bossController_.rage_.decayDelay_ =
				jrage.value("decayDelay", outConfig.bossController_.rage_.decayDelay_);

			outConfig.bossController_.rage_.decayPerSec_ =
				jrage.value("decayPerSec", outConfig.bossController_.rage_.decayPerSec_);

			outConfig.bossController_.rage_.onThreshold_ =
				jrage.value("onThreshold", outConfig.bossController_.rage_.onThreshold_);

			outConfig.bossController_.rage_.offThreshold_ =
				jrage.value("offThreshold", outConfig.bossController_.rage_.offThreshold_);

			outConfig.bossController_.rage_.maxGauge_ =
				jrage.value("maxGauge", outConfig.bossController_.rage_.maxGauge_);
		}

		//=========================================================
		// Missile
		//=========================================================
		if (jc.contains("missile")) {
			auto& jm = jc["missile"];

			outConfig.bossController_.missile_.muzzleYOffset_ =
				jm.value("muzzleYOffset", outConfig.bossController_.missile_.muzzleYOffset_);

			outConfig.bossController_.missile_.chargeTime_ =
				jm.value("chargeTime", outConfig.bossController_.missile_.chargeTime_);

			outConfig.bossController_.missile_.burstCount_ =
				jm.value("burstCount", outConfig.bossController_.missile_.burstCount_);

			outConfig.bossController_.missile_.burstInterval_ =
				jm.value("burstInterval", outConfig.bossController_.missile_.burstInterval_);

			outConfig.bossController_.missile_.speed_ =
				jm.value("speed", outConfig.bossController_.missile_.speed_);

			outConfig.bossController_.missile_.curveHeight_ =
				jm.value("curveHeight", outConfig.bossController_.missile_.curveHeight_);

			outConfig.bossController_.missile_.damage_ =
				jm.value("damage", outConfig.bossController_.missile_.damage_);

			outConfig.bossController_.missile_.lifeFrame_ =
				jm.value("lifeFrame", outConfig.bossController_.missile_.lifeFrame_);
		}

		//=========================================================
		// Slash
		//=========================================================
		if (jc.contains("slash")) {
			auto& js = jc["slash"];

			outConfig.bossController_.slash_.cooldown_ =
				js.value("cooldown", outConfig.bossController_.slash_.cooldown_);

			outConfig.bossController_.slash_.selectRate_ =
				js.value("selectRate", outConfig.bossController_.slash_.selectRate_);

			outConfig.bossController_.slash_.chargeTime_ =
				js.value("chargeTime", outConfig.bossController_.slash_.chargeTime_);

			outConfig.bossController_.slash_.speed_ =
				js.value("speed", outConfig.bossController_.slash_.speed_);

			outConfig.bossController_.slash_.damage_ =
				js.value("damage", outConfig.bossController_.slash_.damage_);

			outConfig.bossController_.slash_.lifeFrame_ =
				js.value("lifeFrame", outConfig.bossController_.slash_.lifeFrame_);
		}

		//=========================================================
		// Enter
		//=========================================================
		if (jc.contains("enter")) {
			auto& jenter = jc["enter"];

			outConfig.bossController_.enter_.approachSpeedZ_ =
				jenter.value("approachSpeedZ", outConfig.bossController_.enter_.approachSpeedZ_);

			outConfig.bossController_.enter_.approachSpeedX_ =
				jenter.value("approachSpeedX", outConfig.bossController_.enter_.approachSpeedX_);

			outConfig.bossController_.enter_.approachSpeedY_ =
				jenter.value("approachSpeedY", outConfig.bossController_.enter_.approachSpeedY_);

			outConfig.bossController_.enter_.completeEpsilonZ_ =
				jenter.value("completeEpsilonZ", outConfig.bossController_.enter_.completeEpsilonZ_);
		}
	}

	return true;
}