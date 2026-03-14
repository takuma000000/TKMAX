#pragma once
#include "BossConfig.h"

//=============================================================
// BossConfigLoader
// boss_config.json を読み込むクラス
//=============================================================

class BossConfigLoader {
public:
	/// <summary>
	/// 指定されたパスから boss_config.json を読み込み、BossConfig 構造体に格納します。
	/// </summary>
	/// <param name="path">boss_config.json のファイルパス</param>
	/// <param name="outConfig">読み込んだ設定を格納する BossConfig 構造体への参照</param>
	/// <returns>読み込みに成功した場合 true、失敗した場合 false</returns>
	static bool Load(const char* path, BossConfig& outConfig);
};