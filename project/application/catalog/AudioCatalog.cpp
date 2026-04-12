#include "AudioCatalog.h"
#include "AudioManager.h"

namespace {
	void LoadAudioIfNeeded_(const char* key, const char* fileName) {
		TKM::AudioManager::GetInstance()->LoadSound(key, fileName);
	}
}

//=============================================================
// 共通音声
//=============================================================
void AudioCatalog::LoadCommonAudios() {
	LoadAudioIfNeeded_("cursor", "cursor.wav"); // カーソル
	LoadAudioIfNeeded_("decision", "decision.wav"); // 決定
}

//=============================================================
// タイトル音声
//=============================================================
void AudioCatalog::LoadTitleAudios() {
	//////////////////////////////////////
	LoadCommonAudios(); // 共通音声もロード
	//////////////////////////////////////
	LoadAudioIfNeeded_("title", "title.wav"); // タイトルBGM
}

//=============================================================
// ゲーム本編音声
//=============================================================
void AudioCatalog::LoadGameAudios() {
	//////////////////////////////////////
	LoadCommonAudios(); // 共通音声もロード
	//////////////////////////////////////
	LoadAudioIfNeeded_("pause", "pause.wav"); // ポーズ
	LoadAudioIfNeeded_("playBGM", "playBGM.wav"); // ゲーム本編BGM
	LoadAudioIfNeeded_("surprise", "surprise.wav"); // !マーク
	LoadAudioIfNeeded_("avoid", "avoid.wav"); // 回避
	LoadAudioIfNeeded_("bossPhaseBGM", "bossPhaseBGM.wav"); // ボスフェーズBGM
}

//=============================================================
// リザルト音声
//=============================================================
void AudioCatalog::LoadResultAudios() {
	//////////////////////////////////////
	LoadCommonAudios(); // 共通音声もロード
	//////////////////////////////////////
}