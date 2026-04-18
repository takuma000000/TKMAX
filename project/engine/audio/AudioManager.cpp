#include "AudioManager.h"
#include <cassert>
#include <fstream>
#include <algorithm>

namespace TKM {

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
		// ============================
		// 1. まず再生中の SourceVoice を停止・破棄
		// ============================
		for (auto& pv : playingVoices_) {
			if (pv.voice_) {
				pv.voice_->Stop();
				pv.voice_->FlushSourceBuffers();
				pv.voice_->DestroyVoice();
				pv.voice_ = nullptr;
			}
		}
		playingVoices_.clear();

		// ============================
		// 2. 次にマスターボイスを破棄
		// ============================
		if (masterVoice_) {
			masterVoice_->DestroyVoice();
			masterVoice_ = nullptr;
		}

		// ============================
		// 3. そのあと音声データを解放
		// ============================
		for (auto& [key, soundData] : soundMap_) {
			soundData.buffer_.clear();
			soundData.bufferSize_ = 0;
		}
		soundMap_.clear();

		// ============================
		// 4. 最後に XAudio2 を解放
		// ============================
		xAudio2_.Reset();

		initialized_ = false;
	}

	//============================
	// DestroyInstance（必要ならアプリ終了時だけ呼ぶ）
	//============================
	void AudioManager::DestroyInstance() {
		AudioManager* instance = GetInstance();
		if (instance->initialized_) {
			instance->Finalize();
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

		// 音量設定
		float v = volume;
		if (v < 0.0f) { v = 0.0f; }
		if (v > 1.0f) { v = 1.0f; }
		// ゲーム全体の音量を掛ける
		float finalVolume = v * gameVolume_;
		if (finalVolume < 0.0f) { finalVolume = 0.0f; }
		if (finalVolume > 1.0f) { finalVolume = 1.0f; }
		// XAudio2の音量は0.0f（無音）から1.0f（最大音量）までの範囲で指定
		hr = sourceVoice->SetVolume(finalVolume);
		assert(SUCCEEDED(hr));

		// バッファ設定
		XAUDIO2_BUFFER buffer = {};
		buffer.pAudioData = soundData.buffer_.data();
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

		playingVoices_.push_back({ key, sourceVoice, v });
	}

	//============================
	// UnloadSound
	//============================
	void AudioManager::UnloadSound(const std::string& key) {
		
		StopSound(key); // 再生中の音声を停止

		auto it = soundMap_.find(key);
		if (it != soundMap_.end()) {
			it->second.buffer_.clear();
			it->second.bufferSize_ = 0;
			soundMap_.erase(it);
		}
	}

	//============================
	// GetInstance
	//============================
	AudioManager* AudioManager::GetInstance() {
		static AudioManager instance;
		if (!instance.initialized_) {
			instance.Initialize();
		}
		return &instance;
	}

	void AudioManager::SetGameVolume(float volume) {
		gameVolume_ = volume;
		if (gameVolume_ < 0.0f) { gameVolume_ = 0.0f; }
		if (gameVolume_ > 1.0f) { gameVolume_ = 1.0f; }

		for (auto& pv : playingVoices_) {
			if (pv.voice_) {
				float finalVolume = pv.baseVolume_ * gameVolume_;
				if (finalVolume < 0.0f) { finalVolume = 0.0f; }
				if (finalVolume > 1.0f) { finalVolume = 1.0f; }
				pv.voice_->SetVolume(finalVolume);
			}
		}
	}

	float AudioManager::GetGameVolume() const {
		return gameVolume_;
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

		std::vector<BYTE> buffer(data.size_);
		file.read(reinterpret_cast<char*>(buffer.data()), data.size_);
		file.close();

		SoundData soundData;
		soundData.wfex_ = format.fmt_;
		soundData.buffer_ = std::move(buffer);
		soundData.bufferSize_ = data.size_;;
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

	//============================
	// PauseSound
	//============================
	void AudioManager::PauseSound(const std::string& key) {
		for (auto& pv : playingVoices_) {
			if (pv.key_ == key && pv.voice_) {
				pv.voice_->Stop();
			}
		}
	}

	//============================
	// ResumeSound
	//============================
	void AudioManager::ResumeSound(const std::string& key) {
		for (auto& pv : playingVoices_) {
			if (pv.key_ == key && pv.voice_) {
				pv.voice_->Start();
			}
		}
	}
}