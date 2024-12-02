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

	SoundData soundData = LoadWaveFile(filename);
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
}

void AudioManager::UnloadSound(const std::string& key) {
	auto it = soundMap.find(key);
	if (it != soundMap.end()) {
		delete[] it->second.pBuffer;
		soundMap.erase(it);
	}
}

SoundData AudioManager::LoadWaveFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::binary);
	assert(file.is_open());

	// RIFFヘッダー
	char riffHeader[12];
	file.read(riffHeader, 12);
	assert(std::memcmp(riffHeader, "RIFF", 4) == 0);
	assert(std::memcmp(riffHeader + 8, "WAVE", 4) == 0);

	// FMTチャンク
	char fmtHeader[8];
	file.read(fmtHeader, 8);
	assert(std::memcmp(fmtHeader, "fmt ", 4) == 0);

	FormatChunk format = {};
	file.read(reinterpret_cast<char*>(&format.fmt), sizeof(WAVEFORMATEX));

	// Dataチャンク
	ChunkHeader dataChunk;
	file.read(reinterpret_cast<char*>(&dataChunk), sizeof(dataChunk));
	while (std::memcmp(dataChunk.id, "data", 4) != 0) {

		file.read((char*)&dataChunk, sizeof(dataChunk));

		// Dataチャンクを検出した場合
		if (strncmp(dataChunk.id, "data", 4) == 0) {
			break;  // Dataチャンクが見つかったのでループを抜ける
		}

		file.seekg(dataChunk.size, std::ios::cur);
		file.read(reinterpret_cast<char*>(&dataChunk), sizeof(dataChunk));
	}

	char* pBuffer = new char[dataChunk.size];
	file.read(pBuffer, dataChunk.size);

	file.close();

	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = dataChunk.size;


	return soundData;
}
