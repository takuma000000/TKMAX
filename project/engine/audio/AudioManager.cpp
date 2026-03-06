#include "AudioManager.h"
#include <cassert>
#include <fstream>
#include <algorithm>

namespace TKM {
	AudioManager* AudioManager::instance = nullptr;

	//============================
	// Initialize
	//============================
	void AudioManager::Initialize() {
		if (initialized_) { return; }

		HRESULT hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
		assert(SUCCEEDED(hr));
		assert(xAudio2_);

		hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
		assert(SUCCEEDED(hr));

		initialized_ = true;
	}

	//============================
	// Finalize（※deleteは絶対しない）
	//============================
	void AudioManager::Finalize() {
		// 登録されている音声データを解放
		for (auto& [key, soundData] : soundMap_) {
			delete[] soundData.pBuffer_;
			soundData.pBuffer_ = nullptr;
			soundData.bufferSize_ = 0;
		}
		soundMap_.clear();

		// マスターボイス破棄
		if (masterVoice_) {
			masterVoice_->DestroyVoice();
			masterVoice_ = nullptr;
		}

		// 再生中の SourceVoice を停止・破棄
		for (auto& pv : playingVoices_) {
			if (pv.voice_) {
				pv.voice_->Stop();
				pv.voice_->FlushSourceBuffers();
				pv.voice_->DestroyVoice();
				pv.voice_ = nullptr;
			}
		}
		playingVoices_.clear();

		// XAudio2解放
		xAudio2_.Reset();

		initialized_ = false;
	}

	//============================
	// DestroyInstance（必要ならアプリ終了時だけ呼ぶ）
	//============================
	void AudioManager::DestroyInstance() {
		if (instance) {
			instance->Finalize();
			delete instance;
			instance = nullptr;
		}
	}

	//============================
	// LoadSound
	//============================
	bool AudioManager::LoadSound(const std::string& key, const std::string& filename) {
		// 初期化漏れ対策
		if (!initialized_) { Initialize(); }

		if (soundMap_.find(key) != soundMap_.end()) {
			return false;
		}

		std::string fullPath = "resources/audio/" + filename;

		SoundData soundData = LoadWaveFile(fullPath);
		soundMap_[key] = soundData;
		return true;
	}

	//============================
	// PlaySound
	//============================
	void AudioManager::PlaySound(const std::string& key, float volume, bool loop) {
		// 初期化漏れ対策（ここで落ちないようにする）
		if (!initialized_ || !xAudio2_) {
			Initialize();
		}

		auto it = soundMap_.find(key);
		if (it == soundMap_.end()) {
			return;
		}

		SoundData& soundData = it->second;

		IXAudio2SourceVoice* sourceVoice = nullptr;
		HRESULT hr = xAudio2_->CreateSourceVoice(&sourceVoice, &soundData.wfex_);
		assert(SUCCEEDED(hr));

		// 音量クランプ（0.0f～1.0f）
		float v = volume;
		if (v < 0.0f) { v = 0.0f; }
		if (v > 1.0f) { v = 1.0f; }
		hr = sourceVoice->SetVolume(v);
		assert(SUCCEEDED(hr));

		// バッファ設定
		XAUDIO2_BUFFER buffer = {};
		buffer.pAudioData = soundData.pBuffer_;
		buffer.AudioBytes = soundData.bufferSize_;

		if (loop) {
			buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		} else {
			buffer.Flags = XAUDIO2_END_OF_STREAM;
		}

		hr = sourceVoice->SubmitSourceBuffer(&buffer);
		assert(SUCCEEDED(hr));

		hr = sourceVoice->Start();
		assert(SUCCEEDED(hr));

		playingVoices_.push_back({ key, sourceVoice });
	}

	//============================
	// UnloadSound
	//============================
	void AudioManager::UnloadSound(const std::string& key) {
		
		StopSound(key); // 再生中の音声を停止

		auto it = soundMap_.find(key);
		if (it != soundMap_.end()) {
			delete[] it->second.pBuffer_;
			it->second.pBuffer_ = nullptr;
			it->second.bufferSize_ = 0;
			soundMap_.erase(it);
		}
	}

	//============================
	// GetInstance
	//============================
	AudioManager* AudioManager::GetInstance() {
		if (instance == nullptr) {
			instance = new AudioManager;
			instance->Initialize(); // 生成時に必ず初期化
		}
		return instance;
	}

	//============================
	// LoadWaveFile
	//============================
	SoundData AudioManager::LoadWaveFile(const std::string& filename) {
		std::ifstream file(filename, std::ios_base::binary);
		assert(file.is_open());

		RiffHeader riff;
		file.read(reinterpret_cast<char*>(&riff), sizeof(riff));
		assert(strncmp(riff.chunk_.id_, "RIFF", 4) == 0);
		assert(strncmp(riff.type_, "WAVE", 4) == 0);

		FormatChunk format = {};
		file.read(reinterpret_cast<char*>(&format.chunk_), sizeof(ChunkHeader));
		assert(strncmp(format.chunk_.id_, "fmt ", 4) == 0);
		file.read(reinterpret_cast<char*>(&format.fmt_), format.chunk_.size_);

		assert(format.fmt_.wFormatTag == WAVE_FORMAT_PCM);

		ChunkHeader data;
		while (true) {
			file.read(reinterpret_cast<char*>(&data), sizeof(data));
			if (file.eof() || !file) {
				assert(false);
			}
			if (strncmp(data.id_, "data", 4) == 0) {
				break;
			}
			file.seekg(data.size_, std::ios_base::cur);
		}

		char* pBuffer = new char[data.size_];
		file.read(pBuffer, data.size_);
		file.close();

		SoundData soundData;
		soundData.wfex_ = format.fmt_;
		soundData.pBuffer_ = reinterpret_cast<BYTE*>(pBuffer);
		soundData.bufferSize_ = data.size_;
		return soundData;
	}

	//============================
	// StopSound
	//============================
	void AudioManager::StopSound(const std::string& key) {
		for (auto it = playingVoices_.begin(); it != playingVoices_.end(); ) {
			if (it->key_ == key) {
				if (it->voice_) {
					it->voice_->Stop();
					it->voice_->FlushSourceBuffers();
					it->voice_->DestroyVoice();
					it->voice_ = nullptr;
				}
				it = playingVoices_.erase(it);
			} else {
				++it;
			}
		}
	}

	//============================
	// StopAllSounds
	//============================
	void AudioManager::StopAllSounds() {
		for (auto& pv : playingVoices_) {
			if (pv.voice_) {
				pv.voice_->Stop();
				pv.voice_->FlushSourceBuffers();
				pv.voice_->DestroyVoice();
				pv.voice_ = nullptr;
			}
		}
		playingVoices_.clear();
	}
}