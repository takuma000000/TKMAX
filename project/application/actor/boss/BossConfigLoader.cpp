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
	auto& je = j["bossEnemy"];

	// 基本パラメータ
	outConfig.bossEnemy_.model_ = je.value("model", outConfig.bossEnemy_.model_);
	outConfig.bossEnemy_.tentacleModel_ = je.value("tentacleModel", outConfig.bossEnemy_.tentacleModel_);
	outConfig.bossEnemy_.hp_ = je.value("hp", outConfig.bossEnemy_.hp_);

	// スケール
	if (je.contains("scale")) {
		outConfig.bossEnemy_.scale_.x = je["scale"].value("x", outConfig.bossEnemy_.scale_.x);
		outConfig.bossEnemy_.scale_.y = je["scale"].value("y", outConfig.bossEnemy_.scale_.y);
		outConfig.bossEnemy_.scale_.z = je["scale"].value("z", outConfig.bossEnemy_.scale_.z);
	}

	// 当たり判定サイズ
	if (je.contains("colliderScale")) {
		outConfig.bossEnemy_.colliderScale_.x = je["colliderScale"].value("x", outConfig.bossEnemy_.colliderScale_.x);
		outConfig.bossEnemy_.colliderScale_.y = je["colliderScale"].value("y", outConfig.bossEnemy_.colliderScale_.y);
		outConfig.bossEnemy_.colliderScale_.z = je["colliderScale"].value("z", outConfig.bossEnemy_.colliderScale_.z);
	}

	// ロック演出系パラメータ
	outConfig.bossEnemy_.normalScale_ =
		je.value("normalScale", outConfig.bossEnemy_.normalScale_);

	outConfig.bossEnemy_.lockBlinkSpeed_ =
		je.value("lockBlinkSpeed", outConfig.bossEnemy_.lockBlinkSpeed_);

	outConfig.bossEnemy_.lockBlinkAmount_ =
		je.value("lockBlinkAmount", outConfig.bossEnemy_.lockBlinkAmount_);

	//=========================================================
	// BossBattle
	//=========================================================
	auto& jb = j["bossBattle"];

	// スポーン位置
	if (jb.contains("spawnPos")) {
		outConfig.bossBattle_.spawnPos_.x = jb["spawnPos"].value("x", outConfig.bossBattle_.spawnPos_.x);
		outConfig.bossBattle_.spawnPos_.y = jb["spawnPos"].value("y", outConfig.bossBattle_.spawnPos_.y);
		outConfig.bossBattle_.spawnPos_.z = jb["spawnPos"].value("z", outConfig.bossBattle_.spawnPos_.z);
	}

	// 戦闘エリア最小座標
	if (jb.contains("arenaMin")) {
		outConfig.bossBattle_.arenaMin_.x = jb["arenaMin"].value("x", outConfig.bossBattle_.arenaMin_.x);
		outConfig.bossBattle_.arenaMin_.y = jb["arenaMin"].value("y", outConfig.bossBattle_.arenaMin_.y);
		outConfig.bossBattle_.arenaMin_.z = jb["arenaMin"].value("z", outConfig.bossBattle_.arenaMin_.z);
	}

	// 戦闘エリア最大座標
	if (jb.contains("arenaMax")) {
		outConfig.bossBattle_.arenaMax_.x = jb["arenaMax"].value("x", outConfig.bossBattle_.arenaMax_.x);
		outConfig.bossBattle_.arenaMax_.y = jb["arenaMax"].value("y", outConfig.bossBattle_.arenaMax_.y);
		outConfig.bossBattle_.arenaMax_.z = jb["arenaMax"].value("z", outConfig.bossBattle_.arenaMax_.z);
	}

	// 撃破時リップル演出設定
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

	// 撃破時スロー演出設定
	outConfig.bossBattle_.killSlowScale_ =
		jb.value("killSlowScale", outConfig.bossBattle_.killSlowScale_);

	outConfig.bossBattle_.killSlowDuration_ =
		jb.value("killSlowDuration", outConfig.bossBattle_.killSlowDuration_);

	return true;
}