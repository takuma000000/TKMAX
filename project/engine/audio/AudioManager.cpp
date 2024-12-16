#include "AudioManager.h"
#include <cassert>
#include <fstream>

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {
	Finalize();
}

void AudioManager::Initialize() {
	HRESULT hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));

	hr = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(hr));
}

void AudioManager::Finalize() {
	for (auto& [key, soundData] : soundMap) {
		delete[] soundData.pBuffer;
	}
	soundMap.clear();

	if (masterVoice) {
		masterVoice->DestroyVoice();
		masterVoice = nullptr;
	}
	xAudio2.Reset();
}

bool AudioManager::LoadSound(const std::string& key, const std::string& filename) {
    if (soundMap.find(key) != soundMap.end()) {
        return false; // 既にロード済み
    }

    // ファイル名に "resources/" を追加
    std::string fullPath = "resources/" + filename;

    SoundData soundData = LoadWaveFile(fullPath);
    soundMap[key] = soundData;
    return true;
}


void AudioManager::PlaySound(const std::string& key) {
	auto it = soundMap.find(key);
	if (it == soundMap.end()) {
		return; // 存在しない音声キー
	}

	SoundData& soundData = it->second;

	IXAudio2SourceVoice* sourceVoice = nullptr;
	HRESULT hr = xAudio2->CreateSourceVoice(&sourceVoice, &soundData.wfex);
	assert(SUCCEEDED(hr));

	XAUDIO2_BUFFER buffer = {};
	buffer.pAudioData = soundData.pBuffer;
	buffer.AudioBytes = soundData.bufferSize;
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	hr = sourceVoice->SubmitSourceBuffer(&buffer);
	assert(SUCCEEDED(hr));

	hr = sourceVoice->Start();
	assert(SUCCEEDED(hr));

	// 音声の終了を待つ
	XAUDIO2_VOICE_STATE state;
	do {
		sourceVoice->GetState(&state);
	} while (state.BuffersQueued > 0);

	sourceVoice->DestroyVoice();
}


void AudioManager::UnloadSound(const std::string& key) {
	auto it = soundMap.find(key);
	if (it != soundMap.end()) {
		delete[] it->second.pBuffer;
		soundMap.erase(it);
	}
}

SoundData AudioManager::LoadWaveFile(const std::string& filename) {
	std::ifstream file(filename, std::ios_base::binary);
	assert(file.is_open());

	RiffHeader riff;
	file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
	assert(strncmp(riff.chunk.id, "RIFF", 4) == 0);
	assert(strncmp(riff.type, "WAVE", 4) == 0);

	FormatChunk format = {};
	file.read(reinterpret_cast<char*>(&format.chunk), sizeof(ChunkHeader));
	assert(strncmp(format.chunk.id, "fmt ", 4) == 0);
	file.read(reinterpret_cast<char*>(&format.fmt), format.chunk.size);

	// PCM形式か確認
	assert(format.fmt.wFormatTag == WAVE_FORMAT_PCM);

	ChunkHeader data;
	while (true) {
		file.read(reinterpret_cast<char*>(&data), sizeof(data));
		if (file.eof() || !file) {
			assert(false); // Dataチャンクが見つからなかった
		}
		if (strncmp(data.id, "data", 4) == 0) {
			break;
		}
		file.seekg(data.size, std::ios_base::cur);
	}

	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);
	file.close();

	SoundData soundData;
	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}


