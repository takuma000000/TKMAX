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
	LoadAudioIfNeeded_("cursor", "cursor.wav");
	LoadAudioIfNeeded_("decision", "decision.wav");
}

//=============================================================
// タイトル音声
//=============================================================
void AudioCatalog::LoadTitleAudios() {
	LoadCommonAudios();
	LoadAudioIfNeeded_("title", "kuraran.wav");
}

//=============================================================
// ゲーム本編音声
//=============================================================
void AudioCatalog::LoadGameAudios() {
	LoadCommonAudios();
	LoadAudioIfNeeded_("pause", "pause.wav");
}

//=============================================================
// リザルト音声
//=============================================================
void AudioCatalog::LoadResultAudios() {
	LoadCommonAudios();
}