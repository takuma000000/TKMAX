#include "AudioManager.h"
#include <cassert>
#include <fstream>
#include <algorithm>

namespace TKM {

	//============================
	// Initialize
	//============================
	void AudioManager::Initialize() {
		// すでに初期化済みなら何もしない
		if (initialized_) {
			return;
		}

		// XAudio2本体を生成する
		HRESULT hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
		assert(SUCCEEDED(hr));
		assert(xAudio2_);

		// 最終出力先になるマスターボイスを生成する
		hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
		assert(SUCCEEDED(hr));

		// 初期化完了フラグを立てる
		initialized_ = true;
	}

	//============================
	// Finalize（※deleteは絶対しない）
	//============================
	void AudioManager::Finalize() {
		// ============================
		// 1. 再生中のSourceVoiceを停止・破棄
		// ============================
		for (auto& pv : playingVoices_) {
			if (pv.voice_) {
				// 再生を止めて、積まれているバッファも破棄する
				pv.voice_->Stop();
				pv.voice_->FlushSourceBuffers();

				// SourceVoiceを破棄する
				pv.voice_->DestroyVoice();
				pv.voice_ = nullptr;
			}
		}

		// 管理配列からも削除する
		playingVoices_.clear();

		// ============================
		// 2. マスターボイスを破棄
		// ============================
		if (masterVoice_) {
			masterVoice_->DestroyVoice();
			masterVoice_ = nullptr;
		}

		// ============================
		// 3. 音声データを解放
		// ============================
		for (auto& [key, soundData] : soundMap_) {
			// 読み込んでいたWAVデータのバッファを解放する
			soundData.buffer_.clear();
			soundData.bufferSize_ = 0;
		}

		// 音声データ管理マップを空にする
		soundMap_.clear();

		// ============================
		// 4. XAudio2本体を解放
		// ============================
		xAudio2_.Reset();

		// 未初期化状態に戻す
		initialized_ = false;
	}

	//============================
	// DestroyInstance（必要ならアプリ終了時だけ呼ぶ）
	//============================
	void AudioManager::DestroyInstance() {
		AudioManager* instance = GetInstance();

		// 初期化済みなら終了処理を行う
		if (instance->initialized_) {
			instance->Finalize();
		}
	}

	//============================
	// LoadSound
	//============================
	bool AudioManager::LoadSound(const std::string& key, const std::string& filename) {
		// 初期化漏れ対策として、未初期化ならここで初期化する
		if (!initialized_) {
			Initialize();
		}

		// 同じキーの音声がすでに読み込まれている場合は読み込まない
		if (soundMap_.find(key) != soundMap_.end()) {
			return false;
		}

		// resources/audio配下のファイルとして扱う
		std::string fullPath = "resources/audio/" + filename;

		// WAVファイルを読み込んで管理マップへ登録する
		SoundData soundData = LoadWaveFile(fullPath);
		soundMap_[key] = soundData;

		return true;
	}

	//============================
	// PlaySound
	//============================
	void AudioManager::PlaySound(const std::string& key, float volume, bool loop) {
		// 初期化漏れ対策として、未初期化ならここで初期化する
		if (!initialized_ || !xAudio2_) {
			Initialize();
		}

		// 指定キーの音声データを探す
		auto it = soundMap_.find(key);
		if (it == soundMap_.end()) {
			return;
		}

		SoundData& soundData = it->second;

		// 再生用のSourceVoiceを作成する
		IXAudio2SourceVoice* sourceVoice = nullptr;
		HRESULT hr = xAudio2_->CreateSourceVoice(&sourceVoice, &soundData.wfex_);
		assert(SUCCEEDED(hr));

		// 個別音量を0.0f～1.0fに収める
		float v = volume;
		if (v < 0.0f) { v = 0.0f; }
		if (v > 1.0f) { v = 1.0f; }

		// 個別音量にゲーム全体音量を掛ける
		float finalVolume = v * gameVolume_;

		// 最終音量も0.0f～1.0fに収める
		if (finalVolume < 0.0f) { finalVolume = 0.0f; }
		if (finalVolume > 1.0f) { finalVolume = 1.0f; }

		// XAudio2のSourceVoiceへ音量を反映する
		hr = sourceVoice->SetVolume(finalVolume);
		assert(SUCCEEDED(hr));

		// 再生する音声バッファを設定する
		XAUDIO2_BUFFER buffer = {};
		buffer.pAudioData = soundData.buffer_.data();
		buffer.AudioBytes = soundData.bufferSize_;

		// ループ再生なら無限ループ、そうでなければ終端フラグを立てる
		if (loop) {
			buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
		} else {
			buffer.Flags = XAUDIO2_END_OF_STREAM;
		}

		// SourceVoiceへバッファを渡す
		hr = sourceVoice->SubmitSourceBuffer(&buffer);
		assert(SUCCEEDED(hr));

		// 再生を開始する
		hr = sourceVoice->Start();
		assert(SUCCEEDED(hr));

		// 再生中Voiceとして管理する
		playingVoices_.push_back({ key, sourceVoice, v });
	}

	//============================
	// UnloadSound
	//============================
	void AudioManager::UnloadSound(const std::string& key) {
		// 指定音声が再生中なら先に停止する
		StopSound(key);

		// 指定キーの音声データを探す
		auto it = soundMap_.find(key);
		if (it != soundMap_.end()) {
			// 音声バッファを解放する
			it->second.buffer_.clear();
			it->second.bufferSize_ = 0;

			// 管理マップから削除する
			soundMap_.erase(it);
		}
	}

	//============================
	// GetInstance
	//============================
	AudioManager* AudioManager::GetInstance() {
		static AudioManager instance;

		// 未初期化なら取得時に初期化する
		if (!instance.initialized_) {
			instance.Initialize();
		}

		return &instance;
	}

	void AudioManager::SetGameVolume(float volume) {
		// ゲーム全体音量を設定する
		gameVolume_ = volume;

		// 0.0f～1.0fに収める
		if (gameVolume_ < 0.0f) { gameVolume_ = 0.0f; }
		if (gameVolume_ > 1.0f) { gameVolume_ = 1.0f; }

		// 再生中のすべての音声へ新しい全体音量を反映する
		for (auto& pv : playingVoices_) {
			if (pv.voice_) {
				// 個別音量を保ったまま、ゲーム全体音量だけを掛け直す
				float finalVolume = pv.baseVolume_ * gameVolume_;

				// 最終音量を0.0f～1.0fに収める
				if (finalVolume < 0.0f) { finalVolume = 0.0f; }
				if (finalVolume > 1.0f) { finalVolume = 1.0f; }

				pv.voice_->SetVolume(finalVolume);
			}
		}
	}

	float AudioManager::GetGameVolume() const {
		// 現在のゲーム全体音量を返す
		return gameVolume_;
	}

	//============================
	// LoadWaveFile
	//============================
	SoundData AudioManager::LoadWaveFile(const std::string& filename) {
		// WAVファイルをバイナリで開く
		std::ifstream file(filename, std::ios_base::binary);
		assert(file.is_open());

		// RIFFヘッダを読み込む
		RiffHeader riff;
		file.read(reinterpret_cast<char*>(&riff), sizeof(riff));

		// RIFF形式か確認する
		assert(strncmp(riff.chunk_.id_, "RIFF", 4) == 0);

		// WAVE形式か確認する
		assert(strncmp(riff.type_, "WAVE", 4) == 0);

		// fmtチャンクを読み込む
		FormatChunk format = {};
		file.read(reinterpret_cast<char*>(&format.chunk_), sizeof(ChunkHeader));

		// fmtチャンクであることを確認する
		assert(strncmp(format.chunk_.id_, "fmt ", 4) == 0);

		// WAVフォーマット情報を読み込む
		file.read(reinterpret_cast<char*>(&format.fmt_), format.chunk_.size_);

		// このAudioManagerではPCM形式のみ対応する
		assert(format.fmt_.wFormatTag == WAVE_FORMAT_PCM);

		// dataチャンクを探す
		ChunkHeader data;
		while (true) {
			file.read(reinterpret_cast<char*>(&data), sizeof(data));

			// dataチャンクが見つからないまま終端に来たら異常
			if (file.eof() || !file) {
				assert(false);
			}

			// dataチャンクを見つけたら探索終了
			if (strncmp(data.id_, "data", 4) == 0) {
				break;
			}

			// data以外のチャンクはサイズ分だけ読み飛ばす
			file.seekg(data.size_, std::ios_base::cur);
		}

		// 音声データ本体を読み込む
		std::vector<BYTE> buffer(data.size_);
		file.read(reinterpret_cast<char*>(buffer.data()), data.size_);
		file.close();

		// SoundDataにフォーマット情報とバッファを詰める
		SoundData soundData;
		soundData.wfex_ = format.fmt_;
		soundData.buffer_ = std::move(buffer);
		soundData.bufferSize_ = data.size_;

		return soundData;
	}

	//============================
	// StopSound
	//============================
	void AudioManager::StopSound(const std::string& key) {
		// 再生中Voiceから指定キーのものを探して停止する
		for (auto it = playingVoices_.begin(); it != playingVoices_.end(); ) {
			if (it->key_ == key) {
				if (it->voice_) {
					// 再生を止めてバッファを破棄する
					it->voice_->Stop();
					it->voice_->FlushSourceBuffers();

					// SourceVoiceを破棄する
					it->voice_->DestroyVoice();
					it->voice_ = nullptr;
				}

				// 管理配列から削除する
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
		// 再生中のすべてのSourceVoiceを停止・破棄する
		for (auto& pv : playingVoices_) {
			if (pv.voice_) {
				pv.voice_->Stop();
				pv.voice_->FlushSourceBuffers();
				pv.voice_->DestroyVoice();
				pv.voice_ = nullptr;
			}
		}

		// 管理配列を空にする
		playingVoices_.clear();
	}

	//============================
	// PauseSound
	//============================
	void AudioManager::PauseSound(const std::string& key) {
		// 指定キーの再生中Voiceを一時停止する
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
		// 指定キーの一時停止中Voiceを再開する
		for (auto& pv : playingVoices_) {
			if (pv.key_ == key && pv.voice_) {
				pv.voice_->Start();
			}
		}
	}

}