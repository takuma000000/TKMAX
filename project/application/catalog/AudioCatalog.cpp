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
	LoadAudioIfNeeded_("title", "title.wav");
}

//=============================================================
// ゲーム本編音声
//=============================================================
void AudioCatalog::LoadGameAudios() {
	LoadCommonAudios();
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
	LoadCommonAudios();
}