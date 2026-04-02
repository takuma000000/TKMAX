#pragma once

//=============================================================
// AudioCatalogクラス
// 音声のカタログ（目録）クラス。
// 使う音声をまとめてロードするためのクラスです。
//=============================================================
class AudioCatalog {
public:
	/// <summary>
	/// 共通で使う音声をまとめてロードします。
	/// </summary>
	static void LoadCommonAudios();

	/// <summary>
	/// タイトルシーンで使う音声をまとめてロードします。
	/// </summary>
	static void LoadTitleAudios();

	/// <summary>
	/// ゲーム本編で使う音声をまとめてロードします。
	/// </summary>
	static void LoadGameAudios();

	/// <summary>
	/// リザルト系シーンで使う音声をまとめてロードします。
	/// </summary>
	static void LoadResultAudios();
};